
/*
 * Project Name: Library for the TEA5767HN FM radio Stereo Module
 * File: tea5767.cpp
 * Description: library source file
 * Toolchain :: Rpi PICO ,rp2040, SDK C++
 * Created DEC 2022
 * Description: See URL for full details.
 * URL: https://github.com/gavinlyonsrepo/TEA5767_PICO
 */

#include "../include/tea5767/tea5767.hpp"

TEA5767N::TEA5767N() {
  initializeTransmissionData();
  muted = false;
}

// Initialize the I2C setup 
// Param1 :: the I2C address 0x60 for this device
// Param2 :: The I2C interface i2c0 or ic21
// Param3 :: The I2C Data pin SDA
// Param4 :: The I2C clock pin SCLK
// Param5 :: The I2C bus clock speed in Khz , max 400.
void TEA5767N::begin(uint8_t i2cAddress, i2c_inst_t* i2c_type, uint8_t SDApin, uint8_t SCLKpin, uint16_t CLKspeed) {
 
	_i2cAddress = i2cAddress;
	i2c_inst_t *i2c = i2c_type;
	i2c_init(i2c, CLKspeed * 1000);
	gpio_set_function(SDApin, GPIO_FUNC_I2C);
	gpio_set_function(SCLKpin, GPIO_FUNC_I2C);
	gpio_pull_up(SDApin);
	gpio_pull_up(SCLKpin);
	
}

// Switch off the  I2C
// Param1 :: The I2C interface i2c0 or ic21
void TEA5767N::deinitI2C(i2c_inst_t* i2c_type)
{
	i2c_inst_t *i2c = i2c_type;
	i2c_deinit(i2c); 	
}

// Check Connection Function
// Check if TEA5767 is on the bus asks for one byte
// Returns int16_t if less than zero = error 
int16_t TEA5767N::CheckConnection()
{
	int16_t returnValue;
	uint8_t rxdata;
	returnValue = i2c_read_blocking(i2c, TEA5767_I2C_ADDRESS , &rxdata, 1, false);
	if (bdebug) printf("CheckConnection %d , %u \r\n ", returnValue , rxdata);
	return returnValue;
}

bool TEA5767N::GetIsConnected(void)
{return isConnected;}
	  
	  
void TEA5767N::SetIsConnected(bool connected)
{isConnected = connected;}

// Set the debug mode
// Param1  : bool : true debug on ,false debug off.
// Debug mode uses printf to send messages to console.
void TEA5767N::setDebug(bool OnOff){ bdebug = OnOff;}

void TEA5767N::initializeTransmissionData() {
  transmission_data[FIRST_DATA] = 0;            //MUTE: 0 - not muted
                                                //SEARCH MODE: 0 - not in search mode
	
  transmission_data[SECOND_DATA] = 0;           //No frequency defined yet
	
  transmission_data[THIRD_DATA] = 0xB0;         //10110000
                                                //SUD: 1 - search up
                                                //SSL[1:0]: 01 - low; level ADC output = 5
                                                //HLSI: 1 - high side LO injection
                                                //MS: 0 - stereo ON
                                                //MR: 0 - right audio channel is not muted
                                                //ML: 0 - left audio channel is not muted
                                                //SWP1: 0 - port 1 is LOW
	
  transmission_data[FOURTH_DATA] = 0x10;        //00010000
                                                //SWP2: 0 - port 2 is LOW
                                                //STBY: 0 - not in Standby mode
                                                //BL: 0 - US/Europe FM band
                                                //XTAL: 1 - 32.768 kHz
                                                //SMUTE: 0 - soft mute is OFF
                                                //HCC: 0 - high cut control is OFF
                                                //SNC: 0 - stereo noise cancelling is OFF
                                                //SI: 0 - pin SWPORT1 is software programmable port 1
	
  transmission_data[FIFTH_DATA] = 0x00;         //PLLREF: 0 - the 6.5 MHz reference frequency for the PLL is disabled
                                                //DTC: 0 - the de-emphasis time constant is 50 ms
}

void TEA5767N::calculateOptimalHiLoInjection(float freq) {
	uint8_t signalHigh;
	uint8_t signalLow;
	
	setHighSideLOInjection();
	transmitFrequency((float) (freq + 0.45));
	
	signalHigh = getSignalLevel();
	
	setLowSideLOInjection();
	transmitFrequency((float) (freq - 0.45));
	
	signalLow = getSignalLevel();

	hiInjection = (signalHigh < signalLow) ? 1 : 0;
}

void TEA5767N::setFrequency(float _frequency) {
	frequency = _frequency;
	unsigned int frequencyW;
	
	if (hiInjection) {
		setHighSideLOInjection();
		frequencyW = 4 * ((frequency * 1000000) + 225000) / 32768;
	} else {
		setLowSideLOInjection();
		frequencyW = 4 * ((frequency * 1000000) - 225000) / 32768;
	}
	
	transmission_data[FIRST_DATA] = ((transmission_data[FIRST_DATA] & 0xC0) | ((frequencyW >> 8) & 0x3F));
	transmission_data[SECOND_DATA] = frequencyW & 0XFF;
}

void TEA5767N::transmitData() {
	
	int16_t returnValue = 0;
	returnValue =i2c_write_timeout_us(i2c, _i2cAddress, transmission_data, 5 ,false, TEA5767_I2C_DELAY);
	//returnValue = i2c_write_blocking(i2c, _i2cAddress, transmission_data, 5 ,false); // alternative
	if (bdebug) printf(" tx return value %d \r\n", returnValue);
	busy_wait_ms(100);
}

void TEA5767N::mute() {
	muted = true;
	setSoundOff();
	transmitData();
}

void TEA5767N::setSoundOff() {
	transmission_data[FIRST_DATA] |= 0b10000000;
}

void TEA5767N::turnTheSoundBackOn() {
	muted = false;
	setSoundOn();
	transmitData();
}

void TEA5767N::setSoundOn() {
	transmission_data[FIRST_DATA] &= 0b01111111;
}

bool TEA5767N::isMuted() {
	return muted;
}

void TEA5767N::transmitFrequency(float frequency) {
	setFrequency(frequency);
	transmitData();
}

void TEA5767N::selectFrequency(float frequency) {
	calculateOptimalHiLoInjection(frequency);
	transmitFrequency(frequency);
}

void TEA5767N::selectFrequencyMuting(float frequency) {
	mute();
	calculateOptimalHiLoInjection(frequency);
	transmitFrequency(frequency);
	turnTheSoundBackOn();
}

void TEA5767N::readStatus() {

	int16_t returnValue = 0;
	returnValue = i2c_read_timeout_us(i2c, _i2cAddress, reception_data, 5 ,false, TEA5767_I2C_DELAY );
	//returnValue = i2c_read_blocking(i2c, _i2cAddress, reception_data, 5 ,false); // alternative
	if (bdebug == true) 
		printf(" rx return value %d \r\n", returnValue);
	busy_wait_ms(100);
}

float TEA5767N::readFrequencyInMHz() {
	loadFrequency();
	
	unsigned int frequencyW = (((reception_data[FIRST_DATA] & 0x3F) * 256) + reception_data[SECOND_DATA]);
	return getFrequencyInMHz(frequencyW);
}

void TEA5767N::loadFrequency() {
	readStatus();
	
	//Stores the read frequency that can be the result of a search and it�s not yet in transmission data
	//and is necessary to subsequent calls to search.
	transmission_data[FIRST_DATA] = (transmission_data[FIRST_DATA] & 0xC0) | (reception_data[FIRST_DATA] & 0x3F);
	transmission_data[SECOND_DATA] = reception_data[SECOND_DATA];
}

float TEA5767N::getFrequencyInMHz(unsigned int frequencyW) {
	if (hiInjection) {
		return (((frequencyW / 4.0) * 32768.0) - 225000.0) / 1000000.0;
	} else {
		return (((frequencyW / 4.0) * 32768.0) + 225000.0) / 1000000.0;
	}
}

void TEA5767N::setSearchUp() {
	transmission_data[THIRD_DATA] |= 0b10000000;
}

void TEA5767N::setSearchDown() {
	transmission_data[THIRD_DATA] &= 0b01111111;
}

void TEA5767N::setSearchLowStopLevel() {
	transmission_data[THIRD_DATA] &= 0b10011111;
	transmission_data[THIRD_DATA] |= (LOW_STOP_LEVEL << 5);
}

void TEA5767N::setSearchMidStopLevel() {
	transmission_data[THIRD_DATA] &= 0b10011111;
	transmission_data[THIRD_DATA] |= (MID_STOP_LEVEL << 5);
}

void TEA5767N::setSearchHighStopLevel() {
	transmission_data[THIRD_DATA] &= 0b10011111;
	transmission_data[THIRD_DATA] |= (HIGH_STOP_LEVEL << 5);
}

void TEA5767N::setHighSideLOInjection() {
	transmission_data[THIRD_DATA] |= 0b00010000;
}

void TEA5767N::setLowSideLOInjection() {
	transmission_data[THIRD_DATA] &= 0b11101111;
}

uint8_t TEA5767N::searchNextMuting() {
	uint8_t bandLimitReached;
	
	mute();
	bandLimitReached = searchNext();
	turnTheSoundBackOn();
	
	return bandLimitReached;
}

uint8_t TEA5767N::searchNext() {
	uint8_t bandLimitReached;
	
	if (isSearchUp()) {
		selectFrequency(readFrequencyInMHz() + 0.1);
	} else {
		selectFrequency(readFrequencyInMHz() - 0.1);
	}
	
	//Turns the search on
	transmission_data[FIRST_DATA] |= 0b01000000;
	transmitData();
		
	while(!isReady()) { }
	//Read Band Limit flag
	bandLimitReached = isBandLimitReached();
	//Loads the new selected frequency
	loadFrequency();
	
	//Turns de search off
	transmission_data[FIRST_DATA] &= 0b10111111;
	transmitData();
	
	return bandLimitReached;
}

uint8_t TEA5767N::startsSearchMutingFromBeginning() {
	uint8_t bandLimitReached;
	
	mute();
	bandLimitReached = startsSearchFromBeginning();
	turnTheSoundBackOn();
	
	return bandLimitReached;
}

uint8_t TEA5767N::startsSearchMutingFromEnd() {
	uint8_t bandLimitReached;
	
	mute();
	bandLimitReached = startsSearchFromEnd();
	turnTheSoundBackOn();
	
	return bandLimitReached;
}

uint8_t TEA5767N::startsSearchFromBeginning() {
	setSearchUp();
	return startsSearchFrom(87.0);
}

uint8_t TEA5767N::startsSearchFromEnd() {
	setSearchDown();
	return startsSearchFrom(108.0);
}

uint8_t TEA5767N::startsSearchFrom(float frequency) {
	selectFrequency(frequency);
	return searchNext();
}

uint8_t TEA5767N::getSignalLevel() {
	//Necessary before read status
	transmitData();
	//Read updated status
	readStatus();
	return reception_data[FOURTH_DATA] >> 4;
}

uint8_t TEA5767N::isStereo() {
	readStatus();
	return reception_data[THIRD_DATA] >> 7;
}

uint8_t TEA5767N::isReady() {
	readStatus();
	return reception_data[FIRST_DATA] >> 7;
}

uint8_t TEA5767N::isBandLimitReached() {
	readStatus();
	return (reception_data[FIRST_DATA] >> 6) & 1;
}

uint8_t TEA5767N::isSearchUp() {
	return (transmission_data[THIRD_DATA] & 0b10000000) != 0;
}

uint8_t TEA5767N::isSearchDown() {
	return (transmission_data[THIRD_DATA] & 0b10000000) == 0;
}

bool TEA5767N::isStandBy() {
	readStatus();
	return (transmission_data[FOURTH_DATA] & 0b01000000) != 0;
}

void TEA5767N::setStereoReception() {
	transmission_data[THIRD_DATA] &= 0b11110111;
	transmitData();
}

void TEA5767N::setMonoReception() {
	transmission_data[THIRD_DATA] |= 0b00001000;
	transmitData();
}

void TEA5767N::setSoftMuteOn() {
	transmission_data[FOURTH_DATA] |= 0b00001000;
	transmitData();
}

void TEA5767N::setSoftMuteOff() {
	transmission_data[FOURTH_DATA] &= 0b11110111;
	transmitData();
}

void TEA5767N::muteRight() {
	transmission_data[THIRD_DATA] |= 0b00000100;
	transmitData();
}

void TEA5767N::turnTheRightSoundBackOn() {
	transmission_data[THIRD_DATA] &= 0b11111011;
	transmitData();
}

void TEA5767N::muteLeft() {
	transmission_data[THIRD_DATA] |= 0b00000010;
	transmitData();
}

void TEA5767N::turnTheLeftSoundBackOn() {
	transmission_data[THIRD_DATA] &= 0b11111101;
	transmitData();
}

void TEA5767N::setStandByOn() {
	transmission_data[FOURTH_DATA] |= 0b01000000;
	transmitData();
}

void TEA5767N::setStandByOff() {
	transmission_data[FOURTH_DATA] &= 0b10111111;
	transmitData();
}

void TEA5767N::setHighCutControlOn() {
	transmission_data[FOURTH_DATA] |= 0b00000100;
	transmitData();
}

void TEA5767N::setHighCutControlOff() {
	transmission_data[FOURTH_DATA] &= 0b11111011;
	transmitData();
}

void TEA5767N::setStereoNoiseCancellingOn() {
	transmission_data[FOURTH_DATA] |= 0b00000010;
	transmitData();
}

void TEA5767N::setStereoNoiseCancellingOff() {
	transmission_data[FOURTH_DATA] &= 0b11111101;
	transmitData();
}
