#ifndef BME280_H_
#define BME280_H_

char BME280_GetID(void);
void BME280_SoftReset(void);
char BME280_GetStatus(void);
void BME280_ReadData(void);
char BME280_GetConfig(void);
char BME280_GetCtrlMeasurement(void);
char BME280_GetCtrlHumidity(void);
void BME280_ReadCalibrationParams(void);
bool BME280_ReadMeasurements(void);
bool BME280_SetOversamplingPressure(char Value);
bool BME280_SetOversamplingTemperature(char Value);
bool BME280_SetOversamplingHumidity(char Value);
bool BME280_SetOversamplingMode(char Value);
void BME280_SetFilterCoefficient(char Value);
void BME280_SetStandbyTime(char Value);
char BME280_IsMeasuring(void);
float BME280_GetTemperature(void);
float BME280_GetHumidity(void);
float BME280_GetPressure(void);
bool BME280_init(void);
bool BME280_poll(void);

#endif /* BME280_H_ */