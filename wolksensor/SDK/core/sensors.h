/*
 * logger.h
 *
 * Created: 1/22/2015 11:02:26 AM
 *  Author: btomic
 */ 

#ifndef SENSORS_H_
#define SENSORS_H_

#include "platform_specific.h"
#include "sensor_readings_buffer.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define SENSOR_VALUE_NOT_SET -32768

typedef enum
{
	VALUE_ON_DEMAND = 0,
	NOTIFIES_VALUE
}
sensor_type_t;

typedef enum
{
	ALARM_TYPE_NOTIFY_ALWAYS = 0,
	ALARM_TYPE_NOTIFY_ONCE
}
sensor_alarm_type_t;

typedef struct
{
	char id;
	sensor_type_t type;
	sensor_alarm_type_t alarm_type;
}
sensor_t;

typedef struct
{
	char id;
	float value;
}
sensor_state_t;

typedef struct
{
	bool enabled;
	int16_t value;
	bool active;
	bool sound;
}
sensor_alarm_t;

typedef struct
{
	sensor_alarm_t alarm_low;
	sensor_alarm_t alarm_high;
}
sensor_alarms_t;

extern sensor_t sensors[NUMBER_OF_SENSORS];
extern sensor_alarms_t sensors_alarms[NUMBER_OF_SENSORS];

void sensors_init(void);
bool check_sensor_alarm_updates(sensor_state_t* sensors_states, uint8_t number_of_sensors);
uint8_t number_of_unsounded_alarms(void);
bool sensors_have_unsounded_alarms(void);
void clear_sounded_alarms(void);
int8_t get_index_of_sensor(char id);
uint8_t get_value_on_demand_sensors_ids(char value_on_demand_sensors_ids[NUMBER_OF_SENSORS]);

#ifdef __cplusplus
}
#endif

#endif /* SENSORS_H_ */
