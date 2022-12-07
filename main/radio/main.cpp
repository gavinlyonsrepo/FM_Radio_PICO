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
#include <vector> // used in DisplaySelectMenu
#include <time.h> // for settings function (time hold of the button)

// Custom libraries, all here https://github.com/gavinlyonsrepo/RPI_PICO_projects_list
#include "ch1115/ER_OLEDM1_CH1115.hpp" // OLED
#include "ahtxx/ahtxx.hpp"   // Temperature Sensor
#include "pushbutton/push_button.hpp" // Push button
#include "tea5767/tea5767.hpp" // FM  radio module
#include "bitmapdata/bitmap_data.hpp" // Bitmap Data

// === Setup ===

// Timing display intervals
const uint16_t intervalAHT10 = 9000;  // mS , AHT10, Recommended polling frequency: 8s-30s
const uint16_t intervalRadioSignalLevel = 10000; // mS
const uint16_t intervalVolDisplay = 5000; // mS

// Misc
#define I2C_CONNECTION_ATTEMPTS 3 // No of attempts to establish I2C connect at start
bool bDebugPrint = true; // If true debug information printed(in this main.cpp only)
const uint StatusLEDPin = 25; // PICO on_board Status LED

// Push Button Setup 
PushButton MuteBtn(7 , 10); 
PushButton SearchUpBtn(6 , 10); 
PushButton SearchDownBtn(5, 10);

// OLED  
uint8_t screenBuffer[128  * (64 / 8)]; // 1024 bytes = w by h/8 , 128 * 64/8 
ERMCH1115  myOLED(2, 3, 4, 18, 19); 

LIB_AHTXX myAHT10(AHT10_ADDRESS_0X38, AHT10_SENSOR); // AHT10

//radio
typedef enum 
{
	RadioScanSearch = 2, // tunes automatically to signal
	RadioFineTune = 3 // manually fine tunes in increments +/- 50kHz
}RadioScanMode_e; // sets the scan mode

TEA5767N radio;
RadioScanMode_e RadioScanMode = RadioScanSearch;

// === Function Prototypes ===
void Setup(void);

void SplashScreen(void); //Splash screen shown once at start-up

void SelectStation(float &); // Seleect station screen shown at startup once
float DisplaySelectMenu(int8_t);

void Settings(void); // Seetings menu displayed if mute button held done > 3 seconds.
void DisplaySettingsMenu(int8_t);

// Check, read and display radio module
void RadioIsConnect(float &);
bool ReadRadioSignalLevel(uint8_t);
void DisplayRadioInfo(uint8_t , float );
// read and display volume info
bool ReadVolLevel(uint16_t &);
void DisplayVolInfo(uint16_t );
// Check, read and display aht10 sensor
void AHT10IsConnect(void); 
bool ReadAHT10(float *);
void DisplayAHT10Info(float *);

// used for mapping bar graphs :: volume and signal level
uint16_t map(uint32_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max);
clock_t clock(void); // used for timing button press to see if held down.

bool CheckMuteButton(void); 
bool CheckSearchUp(uint8_t &, float &);
bool CheckSearchDown(uint8_t &, float &);

// === Main ====
int main()
{
  float AHT10_Data[2];
  uint8_t signalLevel = 10;
  float freqRadio = 98.4;  // Classic hits FM 98.4
  uint16_t VolumeLevel = 125;

  Setup(); 
  
  SplashScreen();
  RadioIsConnect(freqRadio);
  AHT10IsConnect(); 
  SelectStation(freqRadio);
  signalLevel = radio.getSignalLevel();
  radio.selectFrequency(freqRadio);

  // Display the screen on first pass
  myOLED.OLEDFillScreen(0x00, 0);
  DisplayRadioInfo(signalLevel , freqRadio);
  DisplayVolInfo(VolumeLevel);
  DisplayAHT10Info(AHT10_Data);

  while(true)
  {
    if (ReadAHT10(AHT10_Data)) DisplayAHT10Info(AHT10_Data);
    if (ReadRadioSignalLevel(signalLevel)) DisplayRadioInfo(signalLevel , freqRadio);
    if (ReadVolLevel(VolumeLevel)) DisplayVolInfo(VolumeLevel);

    if (CheckMuteButton()) DisplayVolInfo(VolumeLevel);
    if (CheckSearchUp(signalLevel, freqRadio)) DisplayRadioInfo(signalLevel , freqRadio);
    if (CheckSearchDown(signalLevel, freqRadio)) DisplayRadioInfo(signalLevel , freqRadio);
  } // loop here forever, main loop
} 

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

  if (bDebugPrint) stdio_init_all(); // Init default  serial port, baud 38400
  
  adc_init();  // ADC Init 
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
  myOLED.OLEDclearBuffer();  

  // radio Init 
  radio.begin(TEA5767_I2C_ADDRESS, i2c1, 14, 15, 100);
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
  busy_wait_ms(2000);
  myOLED.OLEDclearBuffer();   
  myOLED.OLEDupdate();
  myOLED.setFontNum(OLEDFontType_Default);
  if (bDebugPrint) printf("FM RADIO : Start!\r\n");
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

// Function used to read AHT10 display sensor on timing define "intervalAHT10"
// Param 1 Pointer to a Array of two floats
// One to hold temperature, One to hold Humidity data.
// Returns: bool true = Data was read , false Data was not read
bool ReadAHT10(float* AHT10_data)
{
    static uint32_t prevMillAHT10 = to_ms_since_boot(get_absolute_time());
    if (myAHT10.AHT10_GetIsConnected() == false) return false;

    if ((to_ms_since_boot(get_absolute_time()) - prevMillAHT10) >= intervalAHT10){ 
        prevMillAHT10 = to_ms_since_boot(get_absolute_time()); 
        AHT10_data[0] = myAHT10.AHT10_readTemperature(true);
        AHT10_data[1] = myAHT10.AHT10_readHumidity(true);
        if (bDebugPrint) printf("AHT10 Read %f \r\n", AHT10_data[0]);
        return true;
    }
  
  return false;
}

//  Function used to read radio signal Level on define "intervalRadioSignalLevel"
//  Param1 :: uint8_t :: Signal Level
//  Returns: bool true = Data was read, false Data was not read
bool ReadRadioSignalLevel(uint8_t SigLevel)
{
    static uint32_t prevMillSignalLevel = to_ms_since_boot(get_absolute_time());
    if ((to_ms_since_boot(get_absolute_time()) - prevMillSignalLevel) >= intervalRadioSignalLevel){ 
        prevMillSignalLevel = to_ms_since_boot(get_absolute_time()); 
        SigLevel = radio.getSignalLevel();
        if (bDebugPrint) printf("Signal Level Read %u\r\n", SigLevel);
        return true;
  }
  return false;
}

// Function to Display Radio information
// Param1 Signal Level
// Param2 Freq of radio station
void DisplayRadioInfo(uint8_t SigLevel, float freqRadio){

        // Clear the radio area buffer
        myOLED.fillRect(0, 0, 128, 32, BACKGROUND);
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

        myOLED.OLEDupdate();
}

// Function to display Volume information
// Param 1 uint16_t  ADC result
void DisplayVolInfo(uint16_t ADCResult)
{
    // Clear the radio area sector
    myOLED.fillRect(0, 32, 128, 16, BACKGROUND);

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
      myOLED.OLEDupdate();
}

// Function to display the AHT10 information
// Param 1 Pointer to array of floats with AHT10 data
void DisplayAHT10Info(float * AHT10_Data)
{
  myOLED.fillRect(0, 48, 128, 16, BACKGROUND);
  if (myAHT10.AHT10_GetIsConnected() == false) // Offline, failed init
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
}


//  Function used to read update OLED on define "intervalVolume"
//  Also read ADC for volume
//  Param1 :: volume level passed by reference
//  Note reads ADC
//  Returns: bool true = ADC was read, false ADC was not read
bool ReadVolLevel(uint16_t &VolLevel)
{
    static uint32_t prevMillDisplay = to_ms_since_boot(get_absolute_time());
    if ((to_ms_since_boot(get_absolute_time()) - prevMillDisplay) >= intervalVolDisplay)
    { 
        prevMillDisplay = to_ms_since_boot(get_absolute_time());
        adc_select_input(2);
        VolLevel = adc_read();
        if (bDebugPrint) printf("ADC Level Read %u \r\n", VolLevel);
        return true;
   } 
   return false;
}

// Function to check for a button press on the SearchUpBtn
// This button will search up the radio band on press
//  Param1 :: uint8_t Signal Level passed by reference
//  Param2 :: (float) freq of radio passed by reference
//  Returns: bool true = button pressed, false button not pressed
bool CheckSearchUp(uint8_t &sigLevel, float &freqRadio)
{
    if (SearchUpBtn.IsPressed())
    {
        if (bDebugPrint) printf("Button up pressed , scan mode %d\r\n", RadioScanMode);
        if (RadioScanMode == RadioScanSearch)
        {
          radio.setSearchUp();
          radio.setSearchLowStopLevel(); // try radio.setSearchMidStopLevel() if too sensitive
          radio.searchNextMuting();
          busy_wait_ms(700); // give time to let radio module tune 
          freqRadio = radio.readFrequencyInMHz();
        }else if (RadioScanMode == RadioFineTune)
        {
          freqRadio = freqRadio + 0.05;
          radio.selectFrequency(freqRadio);
          busy_wait_ms(50);
        }
        sigLevel = radio.getSignalLevel();
        return true;
    }
  return false;
}

// Function to check for a button press on the SearchDownBtn
// This button will search down the radio band on press
//  Param1 :: uint8_t Signal Level passed by reference
//  Param2 :: (float) freq of radio passed by reference
//  Returns: bool true = button pressed, false button not pressed
bool CheckSearchDown(uint8_t &sigLevel, float &freqRadio)
{
    if (SearchDownBtn.IsPressed())
    {
        if (bDebugPrint) printf("Button down pressed, scan mode %d\r\n",RadioScanMode);
        if (RadioScanMode == RadioScanSearch)
        {
          radio.setSearchDown();
          radio.setSearchLowStopLevel(); // try radio.setSearchMidStopLevel() if too sensitive
          radio.searchNextMuting();
          busy_wait_ms(700); // give time to let radio module tune 
          freqRadio = radio.readFrequencyInMHz();
         
        }else if (RadioScanMode == RadioFineTune)
        {
          freqRadio = freqRadio - 0.05;
          radio.selectFrequency(freqRadio);
          busy_wait_ms(50);
        }
      sigLevel = radio.getSignalLevel();
      return true;
    }
  return false;
}

// Function check if mute button pressed.
// Returns: bool true = Data was read, false Data was not read
// NOTE: if held for down for more than 3 seconds enters "settings Mode".
bool CheckMuteButton(void)
{
  if (MuteBtn.IsPressed())
  {
    clock_t startTime = clock();  // start clock

    if (bDebugPrint) printf("Mute button pressed \r\n");
    if (radio.isMuted())
      radio.turnTheSoundBackOn();
    else
      radio.mute();

    while (MuteBtn.IsReleased() == false) // wait for release
      {busy_wait_ms(1);}

    clock_t endTime = clock(); // stop clock
    double executionTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    if (bDebugPrint) printf("%.4f sec  %d\n", executionTime, int(executionTime));
    if ((int)executionTime >= 3) Settings();
    return true;
  }
  return false;
}

// Function to check if Radio connected at Start
// Param1 :: freq of radio passed by reference(float)
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
      if (bDebugPrint) printf("Radio Not connected %d , %u\r\n", returnValue, connectionAttempts);
      busy_wait_ms(2000);
      if (connectionAttempts++ == I2C_CONNECTION_ATTEMPTS){
        radio.SetIsConnected(false);
        freqRadio = 0.0;
        return;
      } 
    }
  } //while
}

// Function handles control of station selection screen
// Param1 freq of radio,
// Note shown at start-ip after splash  user selects a station here at startup
// or just selects default( by selecting start), In this screen mute button selects option.
// search buttons scan up and down menu options
void SelectStation(float &freqRadio)
{
  myOLED.OLEDfadeEffect(); // turn on fade effect
  int8_t menuChoice = 0; // hold menu row index 0-5
  float stationSelected = 0.0; // user menu choice
  DisplaySelectMenu(menuChoice); // display menu first pass

  while(1)
  {
    if (MuteBtn.IsPressed()) // mute press, leave 
    {
      if (menuChoice != 0)
        freqRadio = stationSelected;  
      break;
    }

    if (SearchDownBtn.IsPressed()) // Scan down menu
    {
      menuChoice ++;
      if  (menuChoice == 6) menuChoice = 0;
      stationSelected = DisplaySelectMenu(menuChoice);
    }

    if (SearchUpBtn.IsPressed()) // scan up menu
    {
      menuChoice --;
      if  (menuChoice == -1) menuChoice = 5;
      stationSelected = DisplaySelectMenu(menuChoice);
    }

  }//end of menu display.
   myOLED.OLEDfadeEffect(0x00); // turn off fade effect
   myOLED.OLEDclearBuffer();
   gpio_put(StatusLEDPin, false);
}

// Function displays/draws the menu shown at station selection screen
// Param1: the position in the menu 0-5
// returns: A vector elements data, float with the freq of selected station
// returns 0.0 if user is on menu position zero i.e "start" label
float DisplaySelectMenu(int8_t menuChoice)
{
   uint8_t rowNo = 1;
   std::vector<float> stationList = {91.00, 92.23, 96.34, 102.64, 106.15};
   myOLED.OLEDclearBuffer();
   myOLED.drawBitmap(0, 0, pRadioMastImage, 16, 16, BACKGROUND, FOREGROUND);
   myOLED.drawRoundRect(20, menuChoice * 10, 60, 10,5, FOREGROUND);
   myOLED.setCursor(30, 1);
   myOLED.print("Start");
   for (float i : stationList) {
    myOLED.setCursor(30, (rowNo++ * 10)+1);
    myOLED.print(i);
  }

   myOLED.OLEDupdate();
   if (menuChoice != 0)
      return stationList[menuChoice-1];
    else 
      return 0.0;
    
}

// Function displays/draws the menu shown at settings screen
// Param1: the position in the menu 0 or 1
void DisplaySettingsMenu(int8_t menuChoice)
{
  myOLED.OLEDclearBuffer();
  myOLED.drawRoundRect(10, menuChoice * 10, 100, 10,5, FOREGROUND);
  myOLED.setCursor(20, 1);
  myOLED.print("Scan Search");
  myOLED.setCursor(20, 12);
  myOLED.print("Fine Tune");
  myOLED.OLEDupdate();
}

// Function handles control of settings screen
// Param1 freq of radio,
// Note shown if mute button held down >3 seconds. 
// In this screen mute button selects option.
// search buttons scan up and down menu options
void Settings(void)
{
  int8_t menuChoice = 0;
  uint8_t SelectedMode = 2;
  myOLED.OLEDclearBuffer();
  myOLED.drawBitmap(36, 0, pSettingsImage, 64, 64, FOREGROUND, BACKGROUND);
  myOLED.OLEDupdate();
  busy_wait_ms(2000); // Pause to show user bitmap screen
  myOLED.OLEDfadeEffect(); // turn on fade effect
  DisplaySettingsMenu(menuChoice);

  while(true)
  {
    if (MuteBtn.IsPressed()) // mute press, leave 
    {
      if (menuChoice == 0)
        RadioScanMode = RadioScanSearch;  
      else if(menuChoice == 1)
        RadioScanMode = RadioFineTune; 
      radio.turnTheSoundBackOn();
      break;
    }

    if (SearchDownBtn.IsPressed()) // Scan down menu
    {
      menuChoice ++;
      if  (menuChoice == 2) menuChoice = 0;
        DisplaySettingsMenu(menuChoice);
    }

    if (SearchUpBtn.IsPressed()) // scan up menu
    {
      menuChoice --;
      if  (menuChoice == -1) menuChoice = 1;
        DisplaySettingsMenu(menuChoice);
    }
  }; //end of while

  myOLED.OLEDfadeEffect(0x00); // turn off fade effect
  myOLED.OLEDclearBuffer();
  myOLED.OLEDupdate();
}

clock_t clock()
{
    return (clock_t) time_us_64() / 10000;
}
// === EOF ===