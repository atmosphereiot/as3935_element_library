/**
 *  ----------------------------------------------------------------------------
 *  Copyright (c) 2016, Anaren Microwave, Inc.
 *
 *  For more information on licensing, please see Anaren Microwave, Inc's
 *  end user software licensing agreement: EULA.txt.
 *
 *  ----------------------------------------------------------------------------
 *
 *  as3935_common.c - driver interface for the ams AG AS3935 Frankling Lightning
 *  Sensor.
 *
 *  @version    1.0.0
 *  @date       19 Sep 2016
 *  @author     Anaren, air@anaren.com
 *
 *  assumptions
 *  ===========
 *  - The i2c driver provides the proper signaling sequences for read & write
 *    operations.
 *  - The i2c driver meets the timing requirements specified in the AS3935
 *    datasheet.
 *
 *  file dependency
 *  ===============
 *  i2c.h : defines the i2c read & write interfaces.
 *	math.h : floating point calculations.
 *	fp_math.h : floating point calculations using fixed point math.  Use when math.h not implemented.
 *
 *  revision history
 *  ================
 *  ver 1.0.00 : 19 September 2016
 *  - initial development, replace this line upon release.
 */
#include "as3935.h"

#ifdef AIR_FLOATING_POINT_AVAILABLE
#include <math.h>
#else
#include "../fp_math/fp_math.h"
#endif

// -----------------------------------------------------------------------------
/**
 *  Global data
 */

// -----------------------------------------------------------------------------
/**
 *  Private interface
 */

/**
Calculate temperature of an object based on tdie and vobj
@param tDie temperature of the die in Kelvin
@param vObj object voltage converted first by multiplying 1.5625e-7
@return temperature of an object in Celsius
@see AS3935 datasheet and application note
@note from TI AS3935 BoosterPack sample code
*/
static float AS3935_CalculateTemperature(const float *tDie, const float *vObj)
{
  const float S0 = 6.0E-14;
  const float a1 = 1.75E-3;
  const float a2 = -1.678E-5;
  const float b0 = -2.94E-5;
  const float b1 = -5.7E-7;
  const float b2 = 4.63E-9;
  const float c2 = 13.4;
  const float Tref = 298.15;

#ifdef AIR_FLOATING_POINT_AVAILABLE
  float S = S0*((float)1.0 + a1*(*tDie - Tref) + a2*pow((*tDie - Tref),2));
  float Vos = b0 + b1*(*tDie - Tref) + b2*pow((*tDie - Tref),2);
  float fObj = (*vObj - Vos) + c2*pow((*vObj - Vos),2);
  float Tobj = sqrt(sqrt(pow(*tDie,4) + (fObj/S)));
#else
  float S = S0*((float)1.0 + a1*(*tDie - Tref) + a2*fp_pow((*tDie - Tref),2));
  float Vos = b0 + b1*(*tDie - Tref) + b2*fp_pow((*tDie - Tref),2);
  float fObj = (*vObj - Vos) + c2*fp_pow((*vObj - Vos),2);
  float Tobj = fp_sqrt(fp_sqrt(fp_pow(*tDie,4) + (fObj/S)));
#endif

  return (Tobj - (float)273.15);
}

// -----------------------------------------------------------------------------
/**
 *  Public interface
 */

void AS3935_WriteReg(uint8_t id, uint8_t addr, uint16_t data)
{
  uint8_t writeBytes[3];

  writeBytes[0] = addr;
  writeBytes[1] = data >> 8;
  writeBytes[2] = data & 0xFF;
  AIR_I2C_Write(AS3935_I2C_BASE_ADDR + id, writeBytes, 3);
}

uint16_t AS3935_ReadReg(uint8_t id, uint8_t addr)
{
  uint8_t writeBytes[1] = {0};
  uint8_t readBytes[2] = {0};
  uint16_t readData = 0;

  writeBytes[0] = addr;
  AIR_I2C_ComboRead(AS3935_I2C_BASE_ADDR + id, writeBytes, 1, readBytes, 2);
  readData = (unsigned int)readBytes[0] << 8;
  readData |= readBytes[1];
  return readData;
}

void AS3935_SoftwareReset(uint8_t id)
{
  AS3935_WriteReg(id, AS3935_PRESET_DEF_REG_ADDR, AS3935_DIRECT_CMD_REG_VALU);
}

void AS3935_SetOperatingMode(uint8_t id, enum eAS3935Mode mode)
{
  uint16_t data = AS3935_ReadReg(id, AS3935_CONFIG_REG_ADDR);

  data &= ~AS3935_CONFIG_REG_MOD;
  data |= (uint16_t)mode;
  AS3935_WriteReg(id, AS3935_CONFIG_REG_ADDR, data);
}

enum eAS3935Mode AS3935_GetOperatingMode(uint8_t id)
{
  return (enum eAS3935Mode)(AS3935_ReadReg(id, AS3935_CONFIG_REG_ADDR) & AS3935_CONFIG_REG_MOD);
}

void AS3935_SetConversionRate(uint8_t id, enum eAS3935Rate rate)
{
  uint16_t data = AS3935_ReadReg(id, AS3935_CONFIG_REG_ADDR);

  data &= ~AS3935_CONFIG_REG_CR;
  data |= (uint16_t)rate;
  AS3935_WriteReg(id, AS3935_CONFIG_REG_ADDR, data);
}

enum eAS3935Rate AS3935_GetConversionRate(uint8_t id)
{
  return (enum eAS3935Rate)(AS3935_ReadReg(id, AS3935_CONFIG_REG_ADDR) & AS3935_CONFIG_REG_CR);
}

void AS3935_SetDataReadyEnable(uint8_t id, bool en)
{
  uint16_t data = AS3935_ReadReg(id, AS3935_CONFIG_REG_ADDR);

  data &= ~AS3935_CONFIG_REG_EN;
  if (en) data |= AS3935_CONFIG_REG_EN;
  AS3935_WriteReg(id, AS3935_CONFIG_REG_ADDR, data);
}

bool AS3935_GetDataReadyEnable(uint8_t id)
{
  return (AS3935_ReadReg(id, AS3935_CONFIG_REG_ADDR) & AS3935_CONFIG_REG_EN) ? true : false;
}

void AS3935_ClearDataReadyStatus(uint8_t id)
{
  uint16_t data = AS3935_ReadReg(id, AS3935_CONFIG_REG_ADDR);

  data &= ~AS3935_CONFIG_REG_DRDY;
  AS3935_WriteReg(id, AS3935_CONFIG_REG_ADDR, data);
}

bool AS3935_GetDataReadyStatus(uint8_t id)
{
  return (AS3935_ReadReg(id, AS3935_CONFIG_REG_ADDR) & AS3935_CONFIG_REG_DRDY) ? true : false;
}

float AS3935_GetAmbientTemperature(uint8_t id)
{
  int16_t tDieRaw = (int16_t)AS3935_ReadReg(id, AS3935_TAMBIENT_REG_ADDR);

  return (((float)(tDieRaw >> 2)) * (float)0.03125);
}

float AS3935_GetObjectTemperature(uint8_t id)
{
  float tDie = AS3935_GetAmbientTemperature(id) + (float)273.15;
  int16_t vObjRaw = (int16_t)AS3935_ReadReg(id, AS3935_VOBJECT_REG_ADDR);
  float vObj = ((float)(vObjRaw)) * (float)156.25E-9;

  return AS3935_CalculateTemperature(&tDie, &vObj);
}

float AS3935_GetObjectTemperatureWithTransientCorrection(uint8_t id, float *tDie)
{
  int16_t vObjRaw = (int16_t)AS3935_ReadReg(id, AS3935_VOBJECT_REG_ADDR);
  float tSlope;
  float vObjCorr;

  tDie[0] = tDie[1];
  tDie[1] = tDie[2];
  tDie[2] = tDie[3];
  tDie[3] = AS3935_GetAmbientTemperature(id) + (float)273.15;
  tSlope = (tDie[0] != 0.0) ? -((float)0.3*tDie[0])-((float)0.1*tDie[1])+((float)0.1*tDie[2])+((float)0.3*tDie[3]) : 0.0;
  vObjCorr = (((float)(vObjRaw)) * (float)156.25E-9) + (tSlope * (float)2.96E-4);

  return AS3935_CalculateTemperature(&tDie[3], &vObjCorr);
}

uint16_t AS3935_GetMfgId(uint8_t id)
{
  return AS3935_ReadReg(id, AS3935_MFG_ID_REG_ADDR);
}

uint16_t AS3935_GetDeviceId(uint8_t id)
{
  return AS3935_ReadReg(id, AS3935_DEVICE_ID_REG_ADDR);
}
