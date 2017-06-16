#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "util\delay.h"
#include "clock.h"
#include "RTC.h"
#include "twi.h"
#include "chrono.h"
#include "clock.h"
#include "test.h"
#include "logger.h"

#include "Sensors/BME280_Defs.h"
#include "Sensors/BME280.h"
#include "config/conf_os.h"

uint8_t tmp_data[20] = {0};
bme280_calibration_param cal_param;
int32_t adc_t, adc_p, adc_h, t_fine;

long Pressure;
unsigned long Temperature;
unsigned int Humidity;


static char I2C_WriteRegister(char wrAddr, char wrData) {
	tmp_data[0] = wrAddr;
	tmp_data[1] = wrData;
	bool timeout=false;

	TWI_MasterWriteRead(&sensor_twi, BME280_I2C_ADDRESS1, tmp_data, 2, 0);
	start_auxTimeout(10);
	while ((sensor_twi.status != TWIM_STATUS_READY) && !timeout) {timeout=read_auxTimeout();}
	if(timeout || (sensor_twi.result != TWIM_RESULT_OK)) return false;

	return true;
}

static char I2C_ReadRegister(char rAddr) {
	tmp_data[0] = rAddr;
	bool  timeout=false;

	TWI_MasterWriteRead(&sensor_twi, BME280_I2C_ADDRESS1, tmp_data, 1, 1);

	start_auxTimeout(10);
	while ((sensor_twi.status != TWIM_STATUS_READY) && !timeout) {timeout=read_auxTimeout();}
	if(timeout || (sensor_twi.result != TWIM_RESULT_OK)) return false;

	return sensor_twi.readData[0];
}


bool BME280_ReadMeasurements(void) {
	tmp_data[0] = BME280_PRESSURE_MSB_REG;
	bool  timeout=false;

	if(!TWI_MasterWriteRead(&sensor_twi, BME280_I2C_ADDRESS1, tmp_data, 1, BME280_DATA_FRAME_SIZE)) return false;
	start_auxTimeout(10);
	while ((sensor_twi.status != TWIM_STATUS_READY) && !timeout) {timeout=read_auxTimeout();}
	if(timeout || (sensor_twi.result != TWIM_RESULT_OK)) return false;
	
	adc_h = sensor_twi.readData[BME280_DATA_FRAME_HUMIDITY_LSB_BYTE];
	adc_h |= (uint16_t)sensor_twi.readData[BME280_DATA_FRAME_HUMIDITY_MSB_BYTE] << 8;

	adc_t  = (uint32_t)sensor_twi.readData[BME280_DATA_FRAME_TEMPERATURE_XLSB_BYTE] >> 4;
	adc_t |= (uint32_t)sensor_twi.readData[BME280_DATA_FRAME_TEMPERATURE_LSB_BYTE] << 4;
	adc_t |= (uint32_t)sensor_twi.readData[BME280_DATA_FRAME_TEMPERATURE_MSB_BYTE] << 12;

	adc_p  = (uint32_t)sensor_twi.readData[BME280_DATA_FRAME_PRESSURE_XLSB_BYTE] >> 4;
	adc_p |= (uint32_t)sensor_twi.readData[BME280_DATA_FRAME_PRESSURE_LSB_BYTE] << 4;
	adc_p |= (uint32_t)sensor_twi.readData[BME280_DATA_FRAME_PRESSURE_MSB_BYTE] << 12;

	return true;
}


char BME280_GetID(void) {
	return I2C_ReadRegister(BME280_CHIP_ID_REG);
}

void BME280_SoftReset(void) {
	I2C_WriteRegister(BME280_RST_REG, BME280_SOFT_RESET);
}

char BME280_GetStatus(void) {
	return I2C_ReadRegister(BME280_STAT_REG);
}

char BME280_GetCtrlMeasurement(void) {
	return I2C_ReadRegister(BME280_CTRL_MEAS_REG);
}

char BME280_GetCtrlHumidity(void) {
	return I2C_ReadRegister(BME280_CTRL_HUMIDITY_REG);
}

char BME280_GetConfig(void) {
	return I2C_ReadRegister(BME280_CONFIG_REG);
}


void BME280_ReadCalibrationParams(void) {
	char lsb, msb;

	msb = I2C_ReadRegister(BME280_TEMPERATURE_CALIB_DIG_T1_MSB_REG);
	cal_param.dig_T1 = (unsigned int) msb;
	lsb = I2C_ReadRegister(BME280_TEMPERATURE_CALIB_DIG_T1_LSB_REG);
	cal_param.dig_T1 = (cal_param.dig_T1 << 8) + lsb;
	

	msb = I2C_ReadRegister(BME280_TEMPERATURE_CALIB_DIG_T2_MSB_REG);
	cal_param.dig_T2 = (int) msb;
	lsb = I2C_ReadRegister(BME280_TEMPERATURE_CALIB_DIG_T2_LSB_REG);
	cal_param.dig_T2 = (cal_param.dig_T2 << 8) + lsb;
	

	msb = I2C_ReadRegister(BME280_TEMPERATURE_CALIB_DIG_T3_MSB_REG);
	cal_param.dig_T3 = (int) msb;
	lsb = I2C_ReadRegister(BME280_TEMPERATURE_CALIB_DIG_T3_LSB_REG);
	cal_param.dig_T3 = (cal_param.dig_T3 << 8) + lsb;
	

	msb = I2C_ReadRegister(BME280_PRESSURE_CALIB_DIG_P1_MSB_REG);
	cal_param.dig_P1 = (unsigned int) msb;
	lsb = I2C_ReadRegister(BME280_PRESSURE_CALIB_DIG_P1_LSB_REG);
	cal_param.dig_P1 = (cal_param.dig_P1 << 8) + lsb;
	

	msb = I2C_ReadRegister(BME280_PRESSURE_CALIB_DIG_P2_MSB_REG);
	cal_param.dig_P2 = (int) msb;
	lsb = I2C_ReadRegister(BME280_PRESSURE_CALIB_DIG_P2_LSB_REG);
	cal_param.dig_P2 = (cal_param.dig_P2 << 8) + lsb;
	

	msb = I2C_ReadRegister(BME280_PRESSURE_CALIB_DIG_P3_MSB_REG);
	cal_param.dig_P3 = (int) msb;
	lsb = I2C_ReadRegister(BME280_PRESSURE_CALIB_DIG_P3_LSB_REG);
	cal_param.dig_P3 = (cal_param.dig_P3 << 8) + lsb;
	

	msb = I2C_ReadRegister(BME280_PRESSURE_CALIB_DIG_P4_MSB_REG);
	cal_param.dig_P4 = (int) msb;
	lsb = I2C_ReadRegister(BME280_PRESSURE_CALIB_DIG_P4_LSB_REG);
	cal_param.dig_P4 = (cal_param.dig_P4 << 8) + lsb;
	

	msb = I2C_ReadRegister(BME280_PRESSURE_CALIB_DIG_P5_MSB_REG);
	cal_param.dig_P5 = (int) msb;
	lsb = I2C_ReadRegister(BME280_PRESSURE_CALIB_DIG_P5_LSB_REG);
	cal_param.dig_P5 = (cal_param.dig_P5 << 8) + lsb;
	

	msb = I2C_ReadRegister(BME280_PRESSURE_CALIB_DIG_P6_MSB_REG);
	cal_param.dig_P6 = (int) msb;
	lsb = I2C_ReadRegister(BME280_PRESSURE_CALIB_DIG_P6_LSB_REG);
	cal_param.dig_P6 = (cal_param.dig_P6 << 8) + lsb;
	

	msb = I2C_ReadRegister(BME280_PRESSURE_CALIB_DIG_P7_MSB_REG);
	cal_param.dig_P7 = (int) msb;
	lsb = I2C_ReadRegister(BME280_PRESSURE_CALIB_DIG_P7_LSB_REG);
	cal_param.dig_P7 = (cal_param.dig_P7 << 8) + lsb;
	

	msb = I2C_ReadRegister(BME280_PRESSURE_CALIB_DIG_P8_MSB_REG);
	cal_param.dig_P8 = (int) msb;
	lsb = I2C_ReadRegister(BME280_PRESSURE_CALIB_DIG_P8_LSB_REG);
	cal_param.dig_P8 = (cal_param.dig_P8 << 8) + lsb;
	

	msb = I2C_ReadRegister(BME280_PRESSURE_CALIB_DIG_P9_MSB_REG);
	cal_param.dig_P9 = (int) msb;
	lsb = I2C_ReadRegister(BME280_PRESSURE_CALIB_DIG_P9_LSB_REG);
	cal_param.dig_P9 = (cal_param.dig_P9 << 8) + lsb;
	
	lsb = I2C_ReadRegister(BME280_HUMIDITY_CALIB_DIG_H1_REG);
	cal_param.dig_H1 = (char) lsb;
	

	msb = I2C_ReadRegister(BME280_HUMIDITY_CALIB_DIG_H2_MSB_REG);
	cal_param.dig_H2 = (int) msb;
	lsb = I2C_ReadRegister(BME280_HUMIDITY_CALIB_DIG_H2_LSB_REG);
	cal_param.dig_H2 = (cal_param.dig_H2 << 8) + lsb;
	
	lsb = I2C_ReadRegister(BME280_HUMIDITY_CALIB_DIG_H3_REG);
	cal_param.dig_H3 = (char) lsb;
	

	msb = I2C_ReadRegister(BME280_HUMIDITY_CALIB_DIG_H4_MSB_REG);
	cal_param.dig_H4 = (int) msb;
	lsb = I2C_ReadRegister(BME280_HUMIDITY_CALIB_DIG_H4_LSB_REG);
	cal_param.dig_H4 = (cal_param.dig_H4 << 4) | (lsb & 0xF);
	
	msb = I2C_ReadRegister(BME280_HUMIDITY_CALIB_DIG_H5_MSB_REG);
	cal_param.dig_H5 = (int) msb;
	cal_param.dig_H5 = (cal_param.dig_H5 << 4) | (lsb >> 4);
	
	lsb = I2C_ReadRegister(BME280_HUMIDITY_CALIB_DIG_H6_REG);
	cal_param.dig_H6 = (short) lsb;
}

bool BME280_SetOversamplingPressure(char Value) {
	char ctrlm;

	ctrlm = BME280_GetCtrlMeasurement();
	ctrlm &= ~BME280_CTRL_MEAS_REG_OVERSAMP_PRESSURE__MSK;
	ctrlm |= Value << BME280_CTRL_MEAS_REG_OVERSAMP_PRESSURE__POS;

	if(!I2C_WriteRegister(BME280_CTRL_MEAS_REG, ctrlm))	return false;

	return true;
}

bool BME280_SetOversamplingTemperature(char Value) {
	char ctrlm;

	ctrlm = BME280_GetCtrlMeasurement();
	ctrlm &= ~BME280_CTRL_MEAS_REG_OVERSAMP_TEMPERATURE__MSK;
	ctrlm |= Value << BME280_CTRL_MEAS_REG_OVERSAMP_TEMPERATURE__POS;

	if(!I2C_WriteRegister(BME280_CTRL_MEAS_REG, ctrlm))	return false;

	return true;
}

bool BME280_SetOversamplingHumidity(char Value) {

	if(!I2C_WriteRegister(BME280_CTRL_HUMIDITY_REG, Value )) return false;

	return true;
}

bool BME280_SetOversamplingMode(char Value) {
	char ctrlm;
	ctrlm = BME280_GetCtrlMeasurement();
	ctrlm |= Value;

	if(!I2C_WriteRegister(BME280_CTRL_MEAS_REG, ctrlm)) return false;

	return true;
}

void BME280_SetFilterCoefficient(char Value) {
	char cfgv;
	cfgv = BME280_GetConfig();
	cfgv &= ~BME280_CONFIG_REG_FILTER__MSK;
	cfgv |= Value << BME280_CONFIG_REG_FILTER__POS;
}

void BME280_SetStandbyTime(char Value) {
	char cfgv;
	cfgv = BME280_GetConfig();
	cfgv &= ~BME280_CONFIG_REG_TSB__MSK;
	cfgv |= Value << BME280_CONFIG_REG_TSB__POS;
}

char BME280_IsMeasuring(void) {
	char output;
	output = BME280_GetStatus();
	return (output & BME280_STAT_REG_MEASURING__MSK);
}

/****************************************************************************************************/
/* Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.  */
/***************************************************************************************************/
int32_t BME280_Compensate_T(void) {
	int32_t temp1, temp2, T;

	temp1 = ((((adc_t>>3) -((int32_t)cal_param.dig_T1<<1))) * ((int32_t)cal_param.dig_T2)) >> 11;
	temp2 = (((((adc_t>>4) - ((int32_t)cal_param.dig_T1)) * ((adc_t>>4) - ((int32_t)cal_param.dig_T1))) >> 12) * ((int32_t)cal_param.dig_T3)) >> 14;
	t_fine = temp1 + temp2;
	T = (t_fine * 5 + 128) >> 8;
	return T;
}

/************************************************************************************************************/
/* Returns humidity in %RH as unsigned 32 bit integer in Q22.10 format (22 integer and 10 fractional bits). */
/* Output value of “47445” represents 47445/1024 = 46.333 %RH */
/************************************************************************************************************/
uint32_t BME280_Compensate_H(void) {
	int32_t v_x1_u32r;

	v_x1_u32r = (t_fine - ((int32_t)76800));
	v_x1_u32r = (((((adc_h << 14) - (((int32_t)cal_param.dig_H4) << 20) - (((int32_t)cal_param.dig_H5) * v_x1_u32r)) +
	((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)cal_param.dig_H6)) >> 10) * (((v_x1_u32r *
	((int32_t)cal_param.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) *
	((int32_t)cal_param.dig_H2) + 8192) >> 14));
	v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)cal_param.dig_H1)) >> 4));
	v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
	v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

	return (uint32_t)(v_x1_u32r>>12);
}

/***********************************************************************************************************/
/* Returns pressure in Pa as unsigned 32 bit integer. Output value of “96386” equals 96386 Pa = 963.86 hPa */
/***********************************************************************************************************/

uint32_t BME280_Compensate_P(void) {
	int64_t  var1, var2, p;

	var1 = ((int64_t)t_fine) - 128000;
	var2 = var1 * var1 * (int64_t)cal_param.dig_P6;
	var2 = var2 + ((var1*(int64_t)cal_param.dig_P5)<<17);
	var2 = var2 + (((int64_t)cal_param.dig_P4)<<35);
	var1 = ((var1 * var1 * (int64_t)cal_param.dig_P3)>>8) + ((var1 * (int64_t)cal_param.dig_P2)<<12);
	var1 = (((((int64_t)1)<<47) + var1)) * ((int64_t)cal_param.dig_P1)>>33;

	if (var1 == 0)
	return 0; 	//avoid exception caused by division by zero

	p = 1048576 - adc_p;
	p = (((p << 31) - var2) * 3125) / var1;
	var1 = (((int64_t)cal_param.dig_P9) * (p>>13) * (p>>13)) >> 25;
	var2 = (((int64_t)cal_param.dig_P8) * p) >> 19;
	p = ((p + var1 + var2) >> 8) + (((int64_t)cal_param.dig_P7)<<4);
	
	return (uint32_t)(p/256);
}


float BME280_GetTemperature(void) {
	return (float)BME280_Compensate_T() / 100;
}

float BME280_GetHumidity(void) {
	return (float)BME280_Compensate_H() / 1024;
}

float BME280_GetPressure(void) {
	return (float) BME280_Compensate_P() / 100;
}


bool BME280_poll(void)
{
	if(!BME280_SetOversamplingPressure(BME280_OVERSAMP_1X)) return false;
	if(!BME280_SetOversamplingTemperature(BME280_OVERSAMP_1X)) return false;
	if(!BME280_SetOversamplingHumidity(BME280_OVERSAMP_1X)) return false;
	if(!BME280_SetOversamplingMode(BME280_FORCED_MODE)) return false;
	_delay_ms(50);
	
	while(BME280_IsMeasuring());
	BME280_ReadMeasurements();

	return true;
}

bool BME280_init(void) {
	char tmp;
	
	tmp = BME280_GetID();
	if (tmp != BME280_CHIP_ID)
	{
		LOG(1, "\n\rCHIP BME280 ERROR");
		return false;
	}
	
	_delay_ms(200);
	BME280_ReadCalibrationParams();                                               //Read calibration parameters
	BME280_SetFilterCoefficient(BME280_FILTER_COEFF_OFF);                          // IIR Filter coefficient 16
	BME280_SetOversamplingPressure(BME280_OVERSAMP_1X);                          // Pressure x16 oversampling
	BME280_SetOversamplingTemperature(BME280_OVERSAMP_1X);                        // Temperature x2 oversampling
	BME280_SetOversamplingHumidity(BME280_OVERSAMP_1X);                           // Humidity x1 oversampling
	BME280_SetOversamplingMode(BME280_FORCED_MODE);
  	
	while(BME280_IsMeasuring());
  	BME280_ReadMeasurements();
	
	return true;
}
