/*
 * Project Name: library for Aosong ASAIR AHT10, 
 AHT15 AHT20 Digital Humidity & Temperature Sensor
 * File: ahtxx.cpp
 * Description: library Source file
 * See URL for full details.
 * URL: https://github.com/gavinlyonsrepo/AHTXX_PICO
 */

#include "../include/ahtxx/ahtxx.hpp"

// Constructor Desc init the sensor data types call before begin()
// Param 1 I2C address
// Param 2 Enum with Sensor types

LIB_AHTXX::LIB_AHTXX(uint8_t address, ASAIR_I2C_SENSOR_e sensorName) {
	_address = address;
	_sensorName = sensorName;
}

// Function Desc Initialise the I2C
// Param 1 : IC2 interface ic20 i2c1 etc
// Param 2 : Data pin I2C
// Param 3 : Clock pin I2C
// Param 4 : I2C Clock speed in Khz 0-400Khz
// NOTE :: call before begin method

void LIB_AHTXX::AHT10_InitI2C(i2c_inst_t* i2c_type, uint8_t  SDApin, uint8_t  SCLKpin, uint16_t CLKspeed) {
	i2c_inst_t *i2c = i2c_type;
	//init I2C
	i2c_init(i2c, CLKspeed * 1000);
	gpio_set_function(SDApin, GPIO_FUNC_I2C);
    gpio_set_function(SCLKpin, GPIO_FUNC_I2C);
	gpio_pull_up(SDApin);
    gpio_pull_up(SCLKpin);
}

 // Function desc: Initialize I2C & configure the sensor, call this function before
 // Returns bool true init success False failure

bool LIB_AHTXX::AHT10_begin()
{
	busy_wait_ms(AHT10_POWER_ON_DELAY);    //wait for sensor to initialize
	AHT10_setNormalMode();                //one measurement+sleep mode
	if (AHT10_enableFactoryCalCoeff()) //load factory calibration coeff
		isConnected = true;
	else 
		isConnected = false;
	return isConnected; 
}


//  Function Desc :: readRawData()  Read raw measurement data from sensor over I2C
//  Returns AHT10_ERROR for failure, true for success with data in the buffer

uint8_t LIB_AHTXX::AHT10_readRawData() {

	uint8_t bufTX[3];
	bufTX[0] = AHT10_START_MEASURMENT_CMD;
	bufTX[1] = AHT10_DATA_MEASURMENT_CMD;
	bufTX[2] = AHT10_DATA_NOP;
	// 	Send measurement command
	returnValue = i2c_write_timeout_us(i2c, _address, bufTX, 3 ,false, AHT10_MY_I2C_DELAY );
	if (returnValue < 1 )
		return AHT10_ERROR; //error handler, collision on I2C bus

	if (AHT10_getCalibrationBit(AHT10_FORCE_READ_DATA) != 0x01)
		return AHT10_ERROR;  // error handler, calibration coefficient turned off
	if (AHT10_getBusyBit(AHT10_USE_READ_DATA) != 0x00)
		busy_wait_ms(AHT10_MEASURMENT_DELAY); // measurement delay

	// Read 6-bytes from sensor
	returnValue = i2c_read_timeout_us(i2c, _address, _rawDataBuffer, 6 ,false, AHT10_MY_I2C_DELAY  );
	if (returnValue < 1) {
		_rawDataBuffer[0] = AHT10_ERROR;
		return AHT10_ERROR;
	}

	return true;
}

// Function Desc:  readTemperature() Read temperature, °C
// Param1 readI2C  use last data or new
// Returns: failure  AHT10_ERROR ,  success temperature as floating point
// NOTES:
// temperature range      -40°C..+80°C
// temperature resolution 0.01°C
// temperature accuracy   ±0.3°C

float LIB_AHTXX::AHT10_readTemperature(bool readI2C) {
	if (readI2C == AHT10_FORCE_READ_DATA) {
		if (AHT10_readRawData() == AHT10_ERROR)
			return AHT10_ERROR; //force to read data to _rawDataBuffer & error handler
	}

	if (_rawDataBuffer[0] == AHT10_ERROR)
		return AHT10_ERROR; //error handler, collision on I2C bus

	uint32_t temperature = ((uint32_t) (_rawDataBuffer[3] & 0x0F) << 16)
			| ((uint16_t) _rawDataBuffer[4] << 8) | _rawDataBuffer[5]; //20-bit raw temperature data

	return (float)(temperature * 0.000191 - 50);
}


// Function Desc :  readHumidity()  Read relative humidity, %
// Param1 readI2C  use last data or new
// Returns: failure  AHT10_ERROR ,  success humidity as floating point
// NOTES
// - relative humidity range      0%..100%
//- relative humidity resolution 0.024%
// - relative humidity accuracy   ±2%

float LIB_AHTXX::AHT10_readHumidity(bool readI2C) {
	if (readI2C == AHT10_FORCE_READ_DATA) {
		if (AHT10_readRawData() == AHT10_ERROR)
			return AHT10_ERROR; //force to read data to _rawDataBuffer & error handler
	}

	if (_rawDataBuffer[0] == AHT10_ERROR)
		return AHT10_ERROR; //error handler, collision on I2C bus

	uint32_t rawData = (((uint32_t) _rawDataBuffer[1] << 16)
			| ((uint16_t) _rawDataBuffer[2] << 8) | (_rawDataBuffer[3])) >> 4; //20-bit raw humidity data

	float humidity = (float) rawData * 0.000095;

	if (humidity < 0)
		return 0;
	if (humidity > 100)
		return 100;
	return humidity;
}


// Function Desc:  softReset() Restart sensor, without power off
// Return: failure  AHT10_ERROR ,  success output of  AHT10_enableFactoryCalCoeff()
// NOTE:
// - takes ~20ms
// - all registers restores to default

bool LIB_AHTXX::AHT10_softReset(void) {
	//MX_I2C1_Init;
	uint8_t bufTX[1];
	bufTX[0]= AHT10_SOFT_RESET_CMD;

	returnValue = i2c_write_timeout_us(i2c, _address, bufTX, 1 ,false , AHT10_MY_I2C_DELAY );
	if (returnValue < 1)
		return false;

	busy_wait_ms(AHT10_SOFT_RESET_DELAY);
	AHT10_setNormalMode();                 //reinitialize sensor registers after reset
	return AHT10_enableFactoryCalCoeff();  //reinitialize sensor registers after reset
}


// Function Desc setNormalMode() Set normal measurement mode
// Returns True for success , false for failure

bool LIB_AHTXX::AHT10_setNormalMode(void) {
	uint8_t bufTX[3];

	bufTX[0] = AHT10_NORMAL_CMD;
	bufTX[1] = AHT10_DATA_NOP;
	bufTX[2] = AHT10_DATA_NOP;

	returnValue = i2c_write_timeout_us(i2c, _address, bufTX, 3 ,false , AHT10_MY_I2C_DELAY );
	if (returnValue < 1)
		return false; //safety check, make sure transmission complete

	busy_wait_ms(AHT10_CMD_DELAY);

	return true;
}


// Function Desc  setCycleMode(),  Set cycle measurement, mode continuous measurement
// Returns True for success , false for failure
bool LIB_AHTXX::AHT10_setCycleMode(void) {
	uint8_t bufTX[3];

	if (_sensorName != AHT20_SENSOR)
		bufTX[0] = AHT10_INIT_CMD;
	else
		bufTX[0] = AHT20_INIT_CMD;
	bufTX[1] = AHT10_INIT_CYCLE_MODE | AHT10_INIT_CAL_ENABLE;
	bufTX[2] = AHT10_DATA_NOP;
	returnValue = i2c_write_timeout_us(i2c, _address, bufTX, 3 ,false, AHT10_MY_I2C_DELAY  );

	if (returnValue < 1)
		return false; //safety check, make sure transmission complete


	return true;
}


// Function Desc :  readStatusByte() , Read status byte from sensor over I2C
// Returns Status byte success or AHT10_ERROR failure

uint8_t LIB_AHTXX::AHT10_readStatusByte() {

	returnValue = i2c_read_timeout_us(i2c, _address, _rawDataBuffer, 1 ,false, AHT10_MY_I2C_DELAY );

	if (returnValue < 1) {
		_rawDataBuffer[0] = AHT10_ERROR;
		return AHT10_ERROR;
	}

	return _rawDataBuffer[0];

}


// Function Desc :getCalibrationBit(),  Read Calibration bit from status byte
// Param1 readI2C  use last data or force new read
// Returns The calibration bit for success or AHT10_ERROR for error
// NOTES:
// - 0, factory calibration coeff disabled
// - 1, factory calibration coeff loaded

uint8_t LIB_AHTXX::AHT10_getCalibrationBit(bool readI2C) {
	uint8_t valueBit;
	if (readI2C == AHT10_FORCE_READ_DATA)
		_rawDataBuffer[0] = AHT10_readStatusByte(); //force to read status byte

	if (_rawDataBuffer[0] != AHT10_ERROR)
	{
		valueBit = (_rawDataBuffer[0] & 0x08);
		return (valueBit>>3); //get 3-rd bit 0001000
	}else{
		return AHT10_ERROR;
	}
}


// Function Desc : enableFactoryCalCoeff() , Load factory calibration coefficients
// returns true for success, false for failure

bool LIB_AHTXX::AHT10_enableFactoryCalCoeff() {

	uint8_t bufTX[3];
	if (_sensorName != AHT20_SENSOR)
		bufTX[0] = AHT10_INIT_CMD;
	else
		bufTX[0] = AHT20_INIT_CMD;

	bufTX[1] = AHT10_INIT_CAL_ENABLE;
	bufTX[2] = AHT10_DATA_NOP;

	returnValue = i2c_write_timeout_us(i2c, _address, bufTX, 3 ,false, AHT10_MY_I2C_DELAY  );
	
	if (returnValue < 1)
	{
		return false; //safety check, make sure transmission complete
	}
	busy_wait_ms(AHT10_CMD_DELAY);

	/*check calibration enable */
	if (AHT10_getCalibrationBit(AHT10_FORCE_READ_DATA) == 0x01){
		return true;
	}else{;
		return false;
	}

}

// Function Desc : getBusyBit(),  Read busy bit from status byte
// Param1 readI2C  use last data or force new read
// Returns busy bit for success or AHT10_ERROR for failure
// NOTES:
// - 0, sensor idle & sleeping
// - 1, sensor busy & in measurement state

uint8_t LIB_AHTXX::AHT10_getBusyBit(bool readI2C) {
	uint8_t valueBit;
	if (readI2C == AHT10_FORCE_READ_DATA)
		_rawDataBuffer[0] = AHT10_readStatusByte(); // Read status byte

	if (_rawDataBuffer[0] != AHT10_ERROR)
	{
		valueBit = (_rawDataBuffer[0] & 0x80);
		return (valueBit>>7);; //get 7-th bit 1000 0000  0x80
	}
	else{
		return AHT10_ERROR;
	}
}


void LIB_AHTXX::AHT10_DeInit(i2c_inst_t* i2c_type)
{
	i2c_inst_t *i2c = i2c_type;
	i2c_deinit(i2c); 	
}

void LIB_AHTXX::AHT10_SetIsConnected(bool connected)
{
	isConnected = connected;
}

bool LIB_AHTXX::AHT10_GetIsConnected(void)
{
	return isConnected;
}