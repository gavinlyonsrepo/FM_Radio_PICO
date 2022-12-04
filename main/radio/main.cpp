// ******************************
// File name :: Radio/main.cpp 
// Description :: FM Radio Project , RPI PICO RP2040  SDK C++
// Author :: Gavin Lyons
// Date :: December 2022
// URL :: https://github.com/gavinlyonsrepo/FM_Radio_PICO
// *****************************

// === Libraries ===

// Standard
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/adc.h"

// Custom
#include "ch1115/ER_OLEDM1_CH1115.hpp" // OLED
#include "ahtxx/ahtxx.hpp"   // Temperature Sensor
#include "pushbutton/push_button.hpp" 
#include "tea5767/tea5767.hpp" // FM  radio module
#include "bitmapdata/bitmap_data.hpp" // Bitmap Data

// === Setup ===

// Timing 
// interval read sensor AHT10, Recommended polling frequency: 8s-30s
const uint16_t intervalAHT10 = 9000;   // mS
const uint16_t intervalRadioSignalLevel = 10000; // mS
const uint16_t intervalDisplay = 2000; // mS

// Misc
#define I2C_CONNECTION_ATTEMPTS 3 // No of attempts to establish I2C connect at start
bool bDebugPrint = false; // If true debug information printed(in this main.cpp only)
const uint StatusLEDPin = 25; // PICO on_board Status LED

// Push Button Setup 
PushButton MuteBtn(7 , 10); 
PushButton SearchUpBtn(6 , 10); 
PushButton SearchDownBtn(5, 10);

// OLED  
uint8_t screenBuffer[128  * (64 / 8)]; // 1024 bytes = w by h/8 , 128 * 64/8 
ERMCH1115  myOLED(2, 3, 4, 18, 19); 

LIB_AHTXX myAHT10(AHT10_ADDRESS_0X38, AHT10_SENSOR); // AHT10

TEA5767N radio; // Radio 

// === Function Prototypes ===
void Setup(void);
void SplashScreen(void);
void AHT10IsConnect(void);
void RadioIsConnect(float &);
uint16_t map(uint32_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max);

void ReadAHT10(float *);
void ReadRadioSignalLevel(uint8_t);
void DisplayScreen(float *, uint8_t , float &);

void CheckMuteButton(void);
uint8_t CheckSearchUp(uint8_t , float &);
uint8_t CheckSearchDown(uint8_t , float &);

// === Main ====
int main()
{
  float AHT10_Data[2];
  uint8_t signalLevel = 10;
  float freqRadio = 98.4;  // Classic hits FM 98.4

  Setup();
  
  SplashScreen();
  RadioIsConnect(freqRadio);
  AHT10IsConnect(); 
      
  signalLevel = radio.getSignalLevel();
  radio.selectFrequency(freqRadio);
  gpio_put(StatusLEDPin, false);

  while(true)
  {
    if (myAHT10.AHT10_GetIsConnected()) ReadAHT10(AHT10_Data);
    DisplayScreen(AHT10_Data, signalLevel, freqRadio);
    ReadRadioSignalLevel(signalLevel);
    
    CheckMuteButton();
    signalLevel = CheckSearchUp(signalLevel, freqRadio);
    signalLevel = CheckSearchDown(signalLevel, freqRadio);
  } // end of while forever
} // end of  main

// === End  of main ===


// === Function Space ===

// Function to setup Radio
void Setup(void)
{
  busy_wait_ms(100);

  // Initialize Status LED pin
  gpio_init(StatusLEDPin);
  gpio_set_dir(StatusLEDPin, GPIO_OUT);
  gpio_put(StatusLEDPin, true);

  // Initialize chosen serial port , default baud 38400
  if (bDebugPrint) stdio_init_all();
  
  // Adc Init 
  adc_init();
  adc_gpio_init(28);

  // Init Push Buttons
  MuteBtn.Init();
  SearchUpBtn.Init(); 
  SearchDownBtn.Init();
  
  // aht10 Sensor setup 
  // (port, data pin, clock pin, clock speed(KHZ));             
  myAHT10.AHT10_InitI2C(i2c0, 16, 17, 100);
  
  // Screen Setup :
  // initialize the OLED , contrast , Spi interface , spi Baud rate in Khz
  // Contrast 00 to FF , 0x80 is default. 
  myOLED.OLEDbegin(0x80, spi0, 8000); 
  myOLED.setTextColor(FOREGROUND);
  myOLED.setFontNum(OLEDFontType_Default);
  myOLED.OLEDFillScreen(0x00, 0);
    
  // Screen Buffer setup  
  myOLED.OLEDbuffer = (uint8_t*) &screenBuffer;  // Assign the pointer to the buffer
  myOLED.OLEDclearBuffer();   // Clear  buffer

  // radio Init 
  radio.begin(TEA5767_I2C_ADDRESS, i2c1, 14, 15, 100);
  if (bDebugPrint) printf("FM RADIO : Start!\r\n");
}


// Function to Display Splash Screen at startup
void SplashScreen (void)
{
  myOLED.setDrawBitmapAddr(true); 
  myOLED.drawBitmap(20, 1, pLightingImage, 84, 24, FOREGROUND, BACKGROUND);
  myOLED.setCursor(10, 28);
  myOLED.setFontNum(OLEDFontType_Homespun);
  myOLED.print("PICO FM Radio");
  myOLED.setCursor(10, 38);
  myOLED.print(" Gavin Lyons");
  myOLED.setCursor(10, 48);
  myOLED.print("  V 1.0.0");
  myOLED.OLEDupdate();  
  busy_wait_ms(3500);
  myOLED.OLEDclearBuffer();   
  myOLED.OLEDupdate();
  myOLED.setFontNum(OLEDFontType_Default);
}

// Function to check if AHT10 sensor is connected at start
// Number of connection attempts defined by I2C_CONNECTION_ATTEMPTS
// Changes member "isConnected" in AHT10 library.
void AHT10IsConnect(void)
{
  uint8_t connectionAttempts = 0;
  // Start the sensor comms
  while (myAHT10.AHT10_begin() != true) 
  {
    if (bDebugPrint) printf("AHT10 not connect or fail load calib coeff \r\n");
    busy_wait_ms(2000);
    connectionAttempts++;
    if (connectionAttempts == I2C_CONNECTION_ATTEMPTS){return;} 
  }
  if (bDebugPrint) printf("AHT10 connected \r\n");
}

// Function used to map signal level and ADC to values displayed in Bargraph's on OLED
uint16_t map(uint32_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Function used to read AHT10 display sensor on define "intervalAHT10"
// Param 1 Pointer to a Array of two floats
// One to hold temperature One to hold Humidity data.
void ReadAHT10(float* AHT10_data)
{
    static uint32_t prevMillAHT10 = to_ms_since_boot(get_absolute_time());

    if ((to_ms_since_boot(get_absolute_time()) - prevMillAHT10) >= intervalAHT10){ 
        prevMillAHT10 = to_ms_since_boot(get_absolute_time()); 
        AHT10_data[0] = myAHT10.AHT10_readTemperature(true);
        AHT10_data[1] = myAHT10.AHT10_readHumidity(true);
        if (bDebugPrint) printf("AHT10 Read %f \r\n", AHT10_data[0]);
    }
  
  return;
}

//  Function used to read radio signal Level on define "intervalRadioSignalLevel"
//  Param1 :: uint8_t Signal Level
void ReadRadioSignalLevel(uint8_t SigLevel)
{
    static uint32_t prevMillSignalLevel = to_ms_since_boot(get_absolute_time());
    if ((to_ms_since_boot(get_absolute_time()) - prevMillSignalLevel) >= intervalRadioSignalLevel){ 
        prevMillSignalLevel = to_ms_since_boot(get_absolute_time()); 
        SigLevel = radio.getSignalLevel();
        if (bDebugPrint) printf("Signal Level Read %u\r\n", SigLevel);
  }
}

//  Function used to read update OLED on define "intervalDisplay"
//  Also read ADC for volume
//  Param1 Pointer to a Array of two floats to hold AHT10 data
//  Param2 :: uint8_t Signal Level
//  Param3 :: freq of radio passed by reference(float)
void DisplayScreen(float * AHT10_Data, uint8_t SigLevel, float &freqRadio)
{
    // 1. read ADC
    // 2. Display Radio station + Signal 
    // 3. Display Volume
    // 4. Display Sensor Data 
    // 5. debug message
    static uint32_t prevMillDisplay = to_ms_since_boot(get_absolute_time());
    static uint16_t ADCResult = 125;

    if ((to_ms_since_boot(get_absolute_time()) - prevMillDisplay) >= intervalDisplay)
    { 
        prevMillDisplay = to_ms_since_boot(get_absolute_time()); 
        // 1 read ADC
        adc_select_input(2);
        ADCResult = adc_read();
        // Clear display
        myOLED.OLEDclearBuffer(); 
        
        // 2 === Radio station + Signal ===
        SigLevel = map(SigLevel, 0 , 20 , 5 , 36);
        if (SigLevel  > 36) SigLevel  = 36;

        // Draw bitmaps
        myOLED.drawBitmap(0, 0, pRadioMastImage, 16, 16, BACKGROUND, FOREGROUND);
        myOLED.drawBitmap(1, 17, pSignalImage, 16, 8, FOREGROUND, BACKGROUND);
        
        // Draw the round rect for Signal level
        myOLED.drawRoundRect(40, 17, 80, 10, 5, FOREGROUND);
        myOLED.fillRoundRect(40, 17, SigLevel *2, 10, 5, FOREGROUND);
        
        // Write the text
        myOLED.setCursor(22,0);
        myOLED.setTextSize(2);
        myOLED.setFontNum(OLEDFontType_Homespun);
        myOLED.print(freqRadio,2);
        myOLED.setTextSize(1);
        myOLED.setFontNum(OLEDFontType_Tiny);
        myOLED.setCursor(110,8);
        myOLED.print(" MHz");

        myOLED.setFontNum(OLEDFontType_Tiny);
        myOLED.setCursor(20,20);
        myOLED.print(SigLevel);
        
        // 3. === Volume ===
        if (radio.isMuted() == false)
        {
            // 12-bit conversion, assume max value == ADC_VREF == 3.3 V
            const float conversionFactor = 3.3f / (1 << 12); // 93.3 / 4096)
            uint16_t BarGraphLength = 1;
            BarGraphLength = map(ADCResult * conversionFactor * 100 , 0 , 330 , 5 , 75);
            
            // bitmap icon
            myOLED.drawBitmap(1, 30, pVolumeImage, 16, 16, FOREGROUND, BACKGROUND);
            // Bargraph for volume
            myOLED.drawRoundRect(40, 32, 80, 10, 5, FOREGROUND);
            myOLED.fillRoundRect(40, 32, BarGraphLength, 10, 5, FOREGROUND);
            // Text
            myOLED.setCursor(20,36);
            myOLED.setFontNum(OLEDFontType_Tiny);
            myOLED.print(BarGraphLength);
        } else { // Muted print mute icon 
            myOLED.drawBitmap(1, 30, pMuteImage, 16, 16, BACKGROUND, FOREGROUND);
        }

        // 4. === Sensor Data === 
        if (myAHT10.AHT10_GetIsConnected() == false) // Offline failed init
        {
          myOLED.setCursor(20,52);
          myOLED.setFontNum(OLEDFontType_Tiny);
          myOLED.print("AHT10 sensor not connected");
        }else{
            //  Temperature
            myOLED.setFontNum(OLEDFontType_Default);
            myOLED.setCursor(20,52);
            myOLED.drawBitmap(0, 48, pTemperatureImage, 16, 16, BACKGROUND, FOREGROUND);
            if (AHT10_Data[0] != AHT10_ERROR) {
            myOLED.print("T");
            myOLED.print(AHT10_Data[0] , 2);
            myOLED.print("C");
            } else {
            myOLED.print("Error 2");
            }
            
            // Humidity
            myOLED.setCursor(84,52);
            myOLED.drawBitmap(64, 48, pHumidityImage, 16, 16, BACKGROUND, FOREGROUND);
            if (AHT10_Data[1] != AHT10_ERROR) {
                myOLED.print("H");
                myOLED.print(AHT10_Data[1], 2);
                myOLED.print("%");
            } else {
                myOLED.print("Error 2");
            }
          }
      // write to buffer
      myOLED.OLEDupdate();

      if (bDebugPrint)  // 5 debug message
      {
        printf("ADC Level Read %u \r\n", ADCResult);
        printf("Display refreshed  \r\n");
      }
   } //end of big if
}

// Function to check for a button press on the SearchUpBtn
// This button will search up the radio band on press
//  Param1 :: uint8_t Signal Level
//  Param2 :: freq of radio passed by reference(float)
//  Returns :: uint8_t Signal Level
uint8_t CheckSearchUp(uint8_t sigLevel, float &freqRadio)
{
    if (SearchUpBtn.IsPressed())
    {
        if (bDebugPrint) printf("Button up pressed \r\n");
        radio.setSearchUp();
        radio.setSearchLowStopLevel(); // try radio.setSearchMidStopLevel() if too sensitive
        radio.searchNextMuting();
        busy_wait_ms(700);
        freqRadio = radio.readFrequencyInMHz();
        sigLevel = radio.getSignalLevel();
    }
    return sigLevel;
}

// Function to check for a button press on the SearchDownBtn
// This button will search down the radio band on press
//  Param1 :: uint8_t Signal Level
//  Param2 :: freq of radio passed by reference(float)
//  Returns :: uint8_t Signal Level
uint8_t CheckSearchDown(uint8_t sigLevel, float &freqRadio)
{
    if (SearchDownBtn.IsPressed())
    {
        if (bDebugPrint) printf("Button down pressed \r\n");
        radio.setSearchDown();
        radio.setSearchLowStopLevel(); // try radio.setSearchMidStopLevel() if too sensitive
        radio.searchNextMuting();
        busy_wait_ms(700);
        freqRadio = radio.readFrequencyInMHz();
        sigLevel = radio.getSignalLevel();
       
    }
    return sigLevel;
}

// Function check if mute button pressed.
void CheckMuteButton(void)
{
  if (MuteBtn.IsPressed())
  {
    if (bDebugPrint) printf("Mute button pressed \r\n");
    if (radio.isMuted())
      radio.turnTheSoundBackOn();
    else
      radio.mute();
  }
}

// Function to check if Radio connected at Start
//  Param1 :: freq of radio passed by reference(float)
// Number of connection attempts defined by I2C_CONNECTION_ATTEMPTS
// Changes member "isConnected" in radio library.
void RadioIsConnect(float &freqRadio)
{
  uint8_t connectionAttempts = 0;
  int16_t returnValue;

  while (true) 
  {
    returnValue =radio.CheckConnection();
    if (returnValue > 0) //connected
    {
        radio.SetIsConnected(true);
        if (bDebugPrint) printf("Radio connected \r\n");
        break;
    }else{ // not connected 
      if (bDebugPrint) printf("Radio Not connected %d\r\n", returnValue);
      busy_wait_ms(2000);
      if (connectionAttempts++ == I2C_CONNECTION_ATTEMPTS){
        radio.SetIsConnected(false);
        freqRadio = 0.0;
        return;
      } 
    }
  } //while
}

// === EOF ===