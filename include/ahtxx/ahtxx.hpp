/*
 * Project Name: library for Aosong ASAIR AHT10,
  AHT15 AHT20 Digital Humidity & Temperature Sensor
 * File: ahtxx.hpp
 * Description: library header file
 * See URL for full details.
 * URL: https://github.com/gavinlyonsrepo/AHTXX_PICO
 */

#ifndef _AHT10_h
#define _AHT10_h

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define AHT10_ADDRESS_0X38         0x38  // I2C address no.1 for AHT10/AHT15/AHT20, adres pin connect to GND
#define AHT10_ADDRESS_0X39         0x39  // I2C address no.2 for AHT10 only, adres pin connect to Vcc
#define AHT10_MY_I2C_DELAY        50000 // Timeout for I2C comms, uS,

#define AHT10_INIT_CMD             0xE1  // init command for AHT10/AHT15
#define AHT20_INIT_CMD             0xBE  // init command for AHT20
#define AHT10_START_MEASURMENT_CMD 0xAC  // start measurement command
#define AHT10_NORMAL_CMD           0xA8  // normal cycle mode command, no info in datasheet!!!
#define AHT10_SOFT_RESET_CMD       0xBA  // soft reset command

#define AHT10_INIT_NORMAL_MODE     0x00  // enable normal mode
#define AHT10_INIT_CYCLE_MODE      0x20  // enable cycle mode
#define AHT10_INIT_CMD_MODE        0x40  // enable command mode
#define AHT10_INIT_CAL_ENABLE      0x08  // load factory calibration coeff

#define AHT10_DATA_MEASURMENT_CMD  0x33
#define AHT10_DATA_NOP             0x00

#define AHT10_MEASURMENT_DELAY     80    // > 75 milliseconds
#define AHT10_POWER_ON_DELAY       40    // 20-40 milliseconds
#define AHT10_CMD_DELAY            350   // > 300 milliseconds
#define AHT10_SOFT_RESET_DELAY     20    // < 20 milliseconds

#define AHT10_FORCE_READ_DATA      true  // force to read data
#define AHT10_USE_READ_DATA        false // force to use data from previous read
#define AHT10_ERROR                0xFF  // returns 255, if communication error is occurred


typedef enum ASAIR_I2C_SENSOR_e{
	AHT10_SENSOR = 0x00,
    AHT15_SENSOR = 0x01,
    AHT20_SENSOR = 0x02
}ASAIR_I2C_SENSOR_e;

class LIB_AHTXX
{
private:
    i2c_inst_t *i2c = i2c0;
    uint8_t  _address;
    ASAIR_I2C_SENSOR_e _sensorName;
    uint8_t  _rawDataBuffer[6] = {AHT10_ERROR, 0, 0, 0, 0, 0};
    int16_t  returnValue = 0;
    bool isConnected = false;

public:
    // Constructor
    LIB_AHTXX(uint8_t , ASAIR_I2C_SENSOR_e);

    void     AHT10_InitI2C(i2c_inst_t* i2c_type,  uint8_t sdata , uint8_t sclk ,uint16_t clockspeed);
    void     AHT10_DeInit(i2c_inst_t* i2c_type);
    bool     AHT10_begin();
    uint8_t  AHT10_readRawData();
    float    AHT10_readTemperature(bool);
    float    AHT10_readHumidity(bool);
    bool     AHT10_softReset();
    bool     AHT10_setNormalMode();
    bool     AHT10_setCycleMode();

    uint8_t  AHT10_readStatusByte();
    uint8_t  AHT10_getCalibrationBit(bool);
    bool     AHT10_enableFactoryCalCoeff();
    uint8_t  AHT10_getBusyBit(bool);

    void AHT10_SetIsConnected(bool);
    bool AHT10_GetIsConnected(void);
};
#endif

