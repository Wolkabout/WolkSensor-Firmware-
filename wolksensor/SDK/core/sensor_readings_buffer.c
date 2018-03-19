/*
 * eventBuffer.c
 *
 * Created: 1/22/2015 10:58:08 AM
 *  Author: btomic
 */ 

#include "sensor_readings_buffer.h"
#include "logger.h"
#include "platform_specific.h"

circular_buffer_t sensor_readings_buffer NO_INIT_MEMORY;
static sensor_readings_t sensor_readings_storage[SENSOR_READINGS_BUFFER_SIZE] NO_INIT_MEMORY;

void init_sensor_readings_buffer(bool clear)
{
	LOG_PRINT(1, PSTR("Readings buffer init, clear %u\r\n"), clear);
	circular_buffer_init(&sensor_readings_buffer, sensor_readings_storage, SENSOR_READINGS_BUFFER_SIZE, sizeof(sensor_readings_t), false, clear);
	LOG_PRINT(2, PSTR("Readings buffer size after init %u\r\n"), circular_buffer_size(&sensor_readings_buffer));
}

void store_sensor_readings(float* sensor_values)
{
	sensor_readings_t sensor_readings;

	sensor_readings.timestamp = rtc_get_ts();

	memcpy(&sensor_readings.values, sensor_values, sizeof(float) * NUMBER_OF_SENSORS);

	LOG(2, "Storing sensors readings");
	
	uint8_t i;
	for(i = 0; i < NUMBER_OF_SENSORS; i++)
	{
		LOG_PRINT(1, PSTR("Storing sensor %c:%.2f\r\n"), sensors[i].id, sensor_readings.values[i]);
	}

	circular_buffer_add(&sensor_readings_buffer, &sensor_readings);
	
	LOG_PRINT(1, PSTR("Stored sensors readings, buffer size %u\r\n"), circular_buffer_size(&sensor_readings_buffer));
}

void remove_sensor_readings(uint16_t count)
{
	LOG_PRINT(2, PSTR("Readings buffer remove %u, size before %u \r\n"), count, circular_buffer_size(&sensor_readings_buffer));
	
	circular_buffer_drop_from_beggining(&sensor_readings_buffer, count);
	
	LOG_PRINT(2, PSTR("Readings buffer size after remove %u \r\n"), circular_buffer_size(&sensor_readings_buffer));
}

uint16_t sensor_readings_count(void)
{
	return circular_buffer_size(&sensor_readings_buffer);
}

bool sensor_readings_buffer_full(void)
{
	return circular_buffer_full(&sensor_readings_buffer);
}

void sensor_readings_buffer_clear(void)
{
	LOG(1, "Clearing sensor readings buffer");
	
	circular_buffer_clear(&sensor_readings_buffer);
	
	LOG_PRINT(2, PSTR("Sensor readings buffer size after clear %u\r\n"), circular_buffer_size(&sensor_readings_buffer));
}
