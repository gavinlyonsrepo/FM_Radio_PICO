/*
* Project Name: ERMCH1115
* File: ERMCH1115.h
* Description: ER_OLEDM1 OLED driven by CH1115 controller source file
* Author: Gavin Lyons.
* URL: https://github.com/gavinlyonsrepo/ER_OLEDM1_CH1115_PICO
*/
#include <stdio.h>
#include "pico/stdlib.h"
#include "../include/ch1115/ER_OLEDM1_CH1115.hpp"
#include "../include/ch1115/ER_OLEDM1_CH1115_graphics.hpp"

// Class Constructor
ERMCH1115  :: ERMCH1115(int8_t cd, int8_t rst, int8_t cs, int8_t sclk, int8_t din) :  ERMCH1115_graphics(OLED_WIDTH, OLED_HEIGHT) 
{
  _OLED_CD = cd;
  _OLED_RST= rst;
  _OLED_CS = cs;
  _OLED_DIN = din;  
  _OLED_SCLK = sclk;
}


// Desc: begin Method initialise OLED 
// Sets pinmodes and SPI setup
// Param1: OLEDcontrast default = 0x80 , range 0x00 to 0xFE
// Param2: SPi instance spi1 or spi0 
// Param3: SPI baudrate in Khz , 1000 = 1 Mhz
// Max SPI speed on the PICO is 62.5Mhz
void ERMCH1115::OLEDbegin (uint8_t OLEDcontrast, spi_inst_t *spiType, uint32_t spiSpeedKhz) 
{
	_OLEDcontrast  = OLEDcontrast ;

	gpio_init(_OLED_CD);
	gpio_init(_OLED_RST);
	gpio_init(_OLED_CS);
	gpio_init(_OLED_SCLK);
	gpio_init(_OLED_DIN);

	gpio_set_dir(_OLED_CD, GPIO_OUT);
	gpio_set_dir(_OLED_RST, GPIO_OUT);
	gpio_set_dir(_OLED_CS, GPIO_OUT);
	gpio_put(_OLED_CS, true);
	
	spi_inst_t *spi = spiType;
	// Initialize SPI port 
	spi_init(spi, spiSpeedKhz * 1000);
	// Initialize SPI pins
	gpio_set_function(_OLED_SCLK, GPIO_FUNC_SPI);
	gpio_set_function(_OLED_DIN, GPIO_FUNC_SPI);

    // Set SPI format
    spi_set_format( spi,   // SPI instance
                    8,      // Number of bits per transfer
                    SPI_CPOL_0,      // Polarity (CPOL)
                    SPI_CPHA_0,      // Phase (CPHA)
                    SPI_MSB_FIRST);

  
  	OLEDinit();
}


// Desc: Called from OLEDbegin carries out Power on sequence and register init
// Can be used to reset OLED to default values.
void ERMCH1115::OLEDinit()
 {
	ERMCH1115_CS_SetLow;
	
	OLEDReset();
	
	send_command(ERMCH1115_DISPLAY_OFF, 0); 
	
	send_command(ERMCH1115_SET_COLADD_LSB, 0); 
	send_command(ERMCH1115_SET_COLADD_MSB, 0); 
	send_command(ERMCH1115_SET_PAGEADD, 0); 
	send_command(ERMCH115_SET_DISPLAY_START_LINE, 0); 
   
	send_command(ERMCH115_CONTRAST_CONTROL  ,0); 
	send_command(_OLEDcontrast, 0);  
	
	send_command(ERMCH1115_IREF_REG, 0); 
	send_command(ERMCH1115_IREF_SET, 0); 
	
	send_command(ERMCH1115_SEG_SET_REMAP, 0); 
	send_command(ERMCH1115_SEG_SET_PADS, 0); 
	send_command(ERMCH1115_ENTIRE_DISPLAY_ON, 0); 
	send_command(ERMCH1115_DISPLAY_NORMAL, 0);
	
	send_command(ERMCH1115_MULTIPLEX_MODE_SET, 0); 
	send_command(ERMCH1115_MULTIPLEX_DATA_SET , 0); 
	
	send_command(ERMCH1115_COMMON_SCAN_DIR, 0); 
	
	send_command(ERMCH1115_OFFSET_MODE_SET, 0); 
	send_command(ERMCH1115_OFFSET_DATA_SET, 0); 
	
	send_command(ERMCH1115_OSC_FREQ_MODE_SET, 0); 
	send_command(ERMCH1115_OSC_FREQ_DATA_SET, 0); 
	
	send_command(ERMCH1115_PRECHARGE_MODE_SET, 0); 
	send_command(ERMCH1115_PRECHARGE_DATA_SET, 0); 
	
	send_command(ERMCH1115_COM_LEVEL_MODE_SET, 0); 
	send_command(ERMCH1115_COM_LEVEL_DATA_SET, 0); 
	
	send_command(ERMCH1115_SET_PUMP_REG, ERMCH115_SET_PUMP_SET); 
	
	send_command(ERMCH1115_DC_MODE_SET, 0); 
	send_command(ERMCH1115_DC_ONOFF_SET, 0);
	
	send_command(ERMCH1115_DISPLAY_ON, 0);
	_sleep= false;
	ERMCH1115_CS_SetHigh;  
	busy_wait_ms(ERMCH1115_INITDELAY);
}

// Desc: Sends a command to the display
// Param1: the command
// Param2: the values to change
void ERMCH1115::send_command (uint8_t command,uint8_t value) 
{
  ERMCH1115_CD_SetLow; 
  send_data(command | value);
  ERMCH1115_CD_SetHigh;
}

// Desc: Sends data to the display via hardware SPI
// Param1: Data byte
void ERMCH1115::send_data(uint8_t data)
{
	spi_write_blocking(spi, &data, 1);

}

// Desc: Resets OLED in a four wire setup called at start 
void ERMCH1115::OLEDReset () 
{
	ERMCH1115_RST_SetHigh; 
	busy_wait_ms(ERMCH1115_RST_DELAY1);
	ERMCH1115_RST_SetLow;
	busy_wait_ms(ERMCH1115_RST_DELAY1);
	ERMCH1115_RST_SetHigh ;
	busy_wait_ms(ERMCH1115_RST_DELAY2);
}

// Desc: Turns On Display
// Param1: bits,  1  on , 0 off
void ERMCH1115::OLEDEnable (uint8_t bits) 
{
 
 ERMCH1115_CS_SetLow;
 if (bits)
 {
	  _sleep= false;
	  send_command(ERMCH1115_DISPLAY_ON, 0);
 }else
 {
	 _sleep= true;
	 send_command(ERMCH1115_DISPLAY_OFF, 0);
 }
 ERMCH1115_CS_SetHigh;

}

// Desc: OLEDIsOff
// Returns: bool  value of _sleep if true OLED is off and in sleep mode, 500uA.
bool ERMCH1115::OLEDIssleeping() { return  _sleep ;}

// Desc: Sets up Horionztal Scroll
// Param1: TimeInterval 0x00 -> 0x07 , 0x00 = 6 frames
// Param2: Direction 0x26 right 0x27 left (A2 – A0)
// Param3: Mode. Set Scroll Mode: (28H – 2BH)  0x28 = continuous
void ERMCH1115::OLEDscrollSetup(uint8_t Timeinterval, uint8_t Direction, uint8_t mode) 
{

 ERMCH1115_CS_SetLow;

 send_command(ERMCH1115_HORIZONTAL_A_SCROLL_SETUP, 0);
 send_command(ERMCH1115_HORIZONTAL_A_SCROLL_SET_SCOL, 0);
 send_command(ERMCH1115_HORIZONTAL_A_SCROLL_SET_ECOL, 0);

 send_command(Direction , 0);
 send_command(ERMCH1115_SPAGE_ADR_SET , 0);
 send_command(Timeinterval , 0);
 send_command(ERMCH1115_EPAGE_ADR_SET , 0);

 send_command(mode, 0);
  
 ERMCH1115_CS_SetHigh;
 
}

// Desc: Turns on Horizontal scroll 
// Param1: bits 1  on , 0 off
// Note OLEDscrollSetup must be called before it 
void ERMCH1115::OLEDscroll(uint8_t bits) 
{

	ERMCH1115_CS_SetLow;
	bits ? send_command(ERMCH1115_ACTIVATE_SCROLL , 0) :   send_command(ERMCH1115_DEACTIVATE_SCROLL, 0);
	ERMCH1115_CS_SetHigh;

}

// Desc: Adjusts contrast 
// Param1: Contrast 0x00 to 0xFF , default 0x80
// Note: Setup during init. 
void ERMCH1115::OLEDContrast(uint8_t contrast)
{

	ERMCH1115_CS_SetLow;
	send_command(ERMCH115_CONTRAST_CONTROL  ,0); 
	send_command(contrast, 0);  
	ERMCH1115_CS_SetHigh;

}

// Desc: Rotates the display vertically 
// Param1: bits 1  on , 0 off
void ERMCH1115::OLEDFlip(uint8_t  bits) 
{

  ERMCH1115_CS_SetLow;
  bits ? send_command(ERMCH1115_COMMON_SCAN_DIR, 0x08):send_command(ERMCH1115_COMMON_SCAN_DIR, 0x00)  ; // C0H - C8H 
  bits ? send_command(ERMCH1115_SEG_SET_REMAP, 0x01):   send_command(ERMCH1115_SEG_SET_REMAP, 0x00); //(A0H - A1H)
  ERMCH1115_CS_SetHigh;

}

// Desc: Turns on fade effect 
// Param1: bits  
// bits = 0x00 to stop  
// bits values: (see datasheet breatheffect P25 for more details)
// 		ON/OFF * * A4 A3 A2 A1 A0
//      When ON/OFF =”H”, Breathing Light ON.
//	 	Breathing Display Effect Maximum Brightness Adjust Set: (A4 – A3)
//		Breathing Display Effect Time Interval Set: (A2 – A0)
// 		Default on is 0x81
void ERMCH1115::OLEDfadeEffect(uint8_t bits) 
{

 ERMCH1115_CS_SetLow;
 send_command(ERMCCH1115_BREATHEFFECT_SET,0);
 send_command(bits,0);
 ERMCH1115_CS_SetHigh;

}

// Call when powering down
void ERMCH1115::OLEDPowerDown(void)
{
	OLEDEnable(0);
	ERMCH1115_CD_SetLow ;
	ERMCH1115_RST_SetLow ;
	ERMCH1115_CS_SetLow;
	ERMCH1115_SCLK_SetLow;
	ERMCH1115_SDA_SetLow ;
	_sleep= true;
}

// Desc: invert the display
// Param1: bits, 1 invert , 0 normal
void ERMCH1115::OLEDInvert(uint8_t bits) 
{
 
 ERMCH1115_CS_SetLow;
 bits ? send_command(ERMCH1115_DISPLAY_INVERT, 0) :   send_command(ERMCH1115_DISPLAY_NORMAL, 0);
 ERMCH1115_CS_SetHigh;

}

// Desc: Fill the screen NOT the buffer with a datapattern 
// Param1: datapattern can be set to zero to clear screen (not buffer) range 0x00 to 0ff
// Param2: optional delay in milliseconds can be set to zero normally.
void ERMCH1115::OLEDFillScreen(uint8_t dataPattern, uint8_t delay) 
{
	for (uint8_t row = 0; row < OLED_PAGE_NUM; row++) 
	{
		OLEDFillPage(row,dataPattern,delay);
	}
}

// Desc: Fill the chosen page(1-8)  with a datapattern 
// Param1: datapattern can be set to 0 to FF (not buffer)
// Param2: optional delay in milliseconds can be set to zero normally.
void ERMCH1115::OLEDFillPage(uint8_t page_num, uint8_t dataPattern,uint8_t mydelay) 
{

	ERMCH1115_CS_SetLow;
	send_command(ERMCH1115_SET_COLADD_LSB, 0); 
	send_command(ERMCH1115_SET_COLADD_MSB, 0);
	send_command(ERMCH1115_SET_PAGEADD, page_num); 
	ERMCH1115_CS_SetHigh;
	busy_wait_us(2);
	ERMCH1115_CS_SetLow;
	uint8_t numofbytes = OLED_WIDTH; // 128 bytes
	for (uint8_t i = 0; i < numofbytes; i++) 
	{
	  send_data(dataPattern);
	  busy_wait_ms(mydelay);
	}
	ERMCH1115_CS_SetHigh;

}

//Desc: Draw a bitmap to the screen
//Param1: x offset 0-128
//Param2: y offset 0-64
//Param3: width 0-128
//Param4 height 0-64
//Param5 the bitmap
void ERMCH1115::OLEDBitmap(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint8_t* data) 
{
 ERMCH1115_CS_SetLow;

  uint8_t tx, ty; 
  uint16_t offset = 0; 
  uint8_t column = (x < 0) ? 0 : x;
  uint8_t page = (y < 0) ? 0 : y >>3;

  for (ty = 0; ty < h; ty = ty + 8) 
  {
		if (y + ty < 0 || y + ty >= OLED_HEIGHT) {continue;}
		send_command(ERMCH1115_SET_COLADD_LSB, (column & 0x0F)); 
		send_command(ERMCH1115_SET_COLADD_MSB, (column & 0xF0) >> 4);
		send_command(ERMCH1115_SET_PAGEADD, page++); 

		for (tx = 0; tx < w; tx++) 
		{
			  if (x + tx < 0 || x + tx >= OLED_WIDTH) {continue;}
			  offset = (w * (ty >> 3)) + tx; 
			  send_data(data[offset]);
		}
  }
ERMCH1115_CS_SetHigh;
}

//Desc: updates the buffer i.e. writes it to the screen
void ERMCH1115::OLEDupdate() 
{
  uint8_t x = 0; uint8_t y = 0; uint8_t w = this->bufferWidth; uint8_t h = this->bufferHeight;
  OLEDBuffer( x,  y,  w,  h, (uint8_t*) this->OLEDbuffer);
}

//Desc: clears the buffer i.e. does NOT write to the screen
void ERMCH1115::OLEDclearBuffer()
{
	memset( this->OLEDbuffer, 0x00, (this->bufferWidth * (this->bufferHeight /8))  ); 
}

//Desc: Draw a bitmap to the screen
//Param1: x offset 0-128
//Param2: y offset 0-64
//Param3: width 0-128
//Param4 height 0-64
//Param5 the bitmap
//Note: Called by OLEDupdate
void ERMCH1115::OLEDBuffer(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t* data) 
{
 ERMCH1115_CS_SetLow;

  uint8_t tx, ty; 
  uint16_t offset = 0; 
  uint8_t column = (x < 0) ? 0 : x;
  uint8_t page = (y < 0) ? 0 : y/8;

  for (ty = 0; ty < h; ty = ty + 8) 
  {
	if (y + ty < 0 || y + ty >= OLED_HEIGHT) {continue;}
	
	send_command(ERMCH1115_SET_COLADD_LSB, (column & 0x0F)); 
	send_command(ERMCH1115_SET_COLADD_MSB, (column & 0XF0) >> 4); 
	send_command(ERMCH1115_SET_PAGEADD, page++); 
 
	for (tx = 0; tx < w; tx++) 
	{
		  if (x + tx < 0 || x + tx >= OLED_WIDTH) {continue;}
		  offset = (w * (ty /8)) + tx; 
		  send_data(data[offset++]);
	}
  }
ERMCH1115_CS_SetHigh;

}

// Desc: Draws a Pixel to the screen overides the  graphics library
// Passed x and y co-ords and colour of pixel.
void ERMCH1115::drawPixel(int16_t x, int16_t y, uint8_t colour) 
{
  if ((x < 0) || (x >= this->bufferWidth) || (y < 0) || (y >= this->bufferHeight)) {
	return;
  }
	  uint16_t tc = (bufferWidth * (y /8)) + x; 
	  switch (colour)
	  {
		case FOREGROUND:  this->OLEDbuffer[tc] |= (1 << (y & 7)); break;
		case BACKGROUND:  this->OLEDbuffer[tc] &= ~(1 << (y & 7)); break;
		case INVERSE: this->OLEDbuffer[tc] ^= (1 << (y & 7)); break;
	  }

}

//************** EOF *************************
