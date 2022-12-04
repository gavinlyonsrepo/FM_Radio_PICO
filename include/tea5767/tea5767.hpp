
/*
 * Project Name: Library for the TEA5767HN FM radio Stereo Module
 * File: tea5767.hpp
 * Description: library header file
 * Toolchain :: Rpi PICO ,rp2040, SDK C++
 * Created DEC 2022
 * Description: See URL for full details.
 * URL: https://github.com/gavinlyonsrepo/TEA5767_PICO
 */

#ifndef TEA5767N_h
#define TEA5767N_h

#include <stdio.h> // optional for printf debug messages
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define TEA5767_I2C_ADDRESS           0x60
#define TEA5767_I2C_DELAY             50000 // uS I2C timeout

#define FIRST_DATA                    0
#define SECOND_DATA                   1
#define THIRD_DATA                    2
#define FOURTH_DATA                   3
#define FIFTH_DATA                    4

#define LOW_STOP_LEVEL                1
#define MID_STOP_LEVEL                2
#define HIGH_STOP_LEVEL               3

#define HIGH_SIDE_INJECTION           1
#define LOW_SIDE_INJECTION            0

#define STEREO_ON                     0
#define STEREO_OFF                    1

#define MUTE_RIGHT_ON                 1
#define MUTE_RIGHT_OFF                0
#define MUTE_LEFT_ON                  1
#define MUTE_LEFT_OFF                 0

#define SWP1_HIGH                     1
#define SWP1_LOW                      0
#define SWP2_HIGH                     1
#define SWP2_LOW                      0

#define STBY_ON                       1
#define STBY_OFF                      0

#define JAPANESE_FM_BAND              1
#define US_EUROPE_FM_BAND             0

#define SOFT_MUTE_ON                  1
#define SOFT_MUTE_OFF                 0

#define HIGH_CUT_CONTROL_ON           1
#define HIGH_CUT_CONTROL_OFF          0

#define STEREO_NOISE_CANCELLING_ON    1
#define STEREO_NOISE_CANCELLING_OFF   0

#define SEARCH_INDICATOR_ON           1
#define SEARCH_INDICATOR_OFF          0

class TEA5767N {
	private:
	  bool bdebug = false; // If true debug information printed
	  bool isConnected = false;
	  i2c_inst_t * i2c = i2c1;  // i2C port number
	  uint8_t _i2cAddress;

	  float frequency;
	  uint8_t hiInjection;
	  uint8_t frequencyH;
	  uint8_t frequencyL;
	  uint8_t transmission_data[5];
	  uint8_t reception_data[5];
	  bool muted;
		
	  void setFrequency(float);
	  void transmitFrequency(float);
	  void transmitData();
	  void initializeTransmissionData();
	  void readStatus();
	  float getFrequencyInMHz(unsigned int);
	  void calculateOptimalHiLoInjection(float);
	  void setHighSideLOInjection();
	  void setLowSideLOInjection();
	  void setSoundOn();
	  void setSoundOff();
	  void loadFrequency();
	  uint8_t isReady();
	  uint8_t isBandLimitReached();
		
	public:

	  TEA5767N();
	  void begin(uint8_t i2cAddress, i2c_inst_t* i2c_type, uint8_t SDApin, uint8_t SCLKpin, uint16_t CLKspeed);
	  void deinitI2C(i2c_inst_t* i2c_type);
	  void setDebug(bool OnOff);
	  bool GetIsConnected(void);
	  void SetIsConnected(bool);
	  int16_t CheckConnection(void);

	  void selectFrequency(float);
	  void selectFrequencyMuting(float);

	  void mute();
	  void turnTheSoundBackOn();
	  void muteLeft();
	  void turnTheLeftSoundBackOn();
	  void muteRight();
	  void turnTheRightSoundBackOn();
	  float readFrequencyInMHz();

	  void setSearchUp();
	  void setSearchDown();
	  void setSearchLowStopLevel();
	  void setSearchMidStopLevel();
	  void setSearchHighStopLevel();

	  void setStereoReception();
	  void setMonoReception();
	  void setSoftMuteOn();
	  void setSoftMuteOff();
	  
	  void setStandByOn();
	  void setStandByOff();
	  void setHighCutControlOn();
	  void setHighCutControlOff();
	  void setStereoNoiseCancellingOn();
	  void setStereoNoiseCancellingOff();
	  
	  uint8_t searchNext();
	  uint8_t searchNextMuting();
	  uint8_t startsSearchFrom(float frequency);
	  uint8_t startsSearchFromBeginning();
	  uint8_t startsSearchFromEnd();
	  uint8_t startsSearchMutingFromBeginning();
	  uint8_t startsSearchMutingFromEnd();
	  uint8_t getSignalLevel();
	  uint8_t isStereo();
	  uint8_t isSearchUp();
	  uint8_t isSearchDown();
	  bool isMuted();
	  bool isStandBy();
	  
};

#endif