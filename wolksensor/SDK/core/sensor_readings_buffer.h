#ifndef SENSORREADINGSBUFFER_H_
#define SENSORREADINGSBUFFER_H_

#include "circular_buffer.h"
#include "chrono.h"
#include "sensors.h"
#include "platform_specific.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define SENSOR_READINGS_BUFFER_SIZE 150/* 2.5 hours of readings per minute */

typedef struct
{
	uint32_t timestamp;
	float    values[NUMBER_OF_SENSORS];
}
sensor_readings_t;

extern circular_buffer_t sensor_readings_buffer NO_INIT_MEMORY;

void init_sensor_readings_buffer(bool clear);
void store_sensor_readings(float* sensor_values);
void remove_sensor_readings(uint16_t count);
uint16_t sensor_readings_count(void);
bool sensor_readings_buffer_full(void);
void sensor_readings_buffer_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* SENSORREADINGSBUFFER_H_ */
