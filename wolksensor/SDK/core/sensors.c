#include "state_machine.h"
#include "sensors.h"
#include "config.h"
#include "logger.h"
#include "chrono.h"

sensor_t sensors[NUMBER_OF_SENSORS];
sensor_alarms_t sensors_alarms[NUMBER_OF_SENSORS];

void sensors_init(void)
{	
	uint8_t i;
	for(i = 0; i < NUMBER_OF_SENSORS; i++)
	{
		memset(&sensors_alarms[i], 0, sizeof(sensor_alarms_t));
	}
}

bool check_sensor_alarm_updates(sensor_state_t* sensors_states, uint8_t number_of_sensors)
{
	LOG(1, "Checking sensor alarms");
	
	uint8_t new_alarms = 0;
	
	uint8_t i = 0;
	for(i = 0; i < number_of_sensors; i++)
	{
		uint8_t sensor_index = get_index_of_sensor(sensors_states[i].id);
		
		if(sensors_alarms[sensor_index].alarm_low.enabled && (sensors_states[i].value <= sensors_alarms[sensor_index].alarm_low.value))
		{
			LOG_PRINT(1, PSTR("Low alarm for %c\r\n"), sensors[sensor_index].id);
			new_alarms += !sensors_alarms[sensor_index].alarm_low.active;
			sensors_alarms[sensor_index].alarm_low.sound = sensors_alarms[sensor_index].alarm_low.sound || !sensors_alarms[sensor_index].alarm_low.active;
			sensors_alarms[sensor_index].alarm_low.active = (sensors[sensor_index].alarm_type == ALARM_TYPE_NOTIFY_ONCE);
		}
		else
		{
			sensors_alarms[sensor_index].alarm_low.active = false;
		}
		
		if(sensors_alarms[sensor_index].alarm_high.enabled && (sensors_states[i].value >= sensors_alarms[sensor_index].alarm_high.value))
		{
			LOG_PRINT(1, PSTR("High alarm for %c\r\n"), sensors[sensor_index].id);
			new_alarms += !sensors_alarms[sensor_index].alarm_high.active;
			sensors_alarms[sensor_index].alarm_high.sound = sensors_alarms[sensor_index].alarm_high.sound || !sensors_alarms[sensor_index].alarm_high.active;
			sensors_alarms[sensor_index].alarm_high.active = (sensors[sensor_index].alarm_type == ALARM_TYPE_NOTIFY_ONCE);
		}
		else
		{
			sensors_alarms[sensor_index].alarm_high.active = false;
		}
	}
	
	LOG_PRINT(1, PSTR("New alarms %u\r\n"), new_alarms);
	
	return new_alarms > 0;
}

uint8_t number_of_unsounded_alarms(void)
{
	uint8_t unsounded_alarms = 0;
	uint8_t i = 0;
	for(i = 0; i < NUMBER_OF_SENSORS; i++)
	{
		unsounded_alarms += sensors_alarms[i].alarm_low.sound;
		unsounded_alarms += sensors_alarms[i].alarm_high.sound;
	}
	return unsounded_alarms;
}

bool sensors_have_unsounded_alarms(void)
{
	return number_of_unsounded_alarms() > 0;
}

void clear_sounded_alarms(void)
{
	uint8_t i = 0;
	for(i = 0; i < NUMBER_OF_SENSORS; i++)
	{
		sensors_alarms[i].alarm_low.sound = false;
		sensors_alarms[i].alarm_high.sound = false;
	}
}

int8_t get_index_of_sensor(char id)
{
	int8_t i;
	for(i = 0; i < NUMBER_OF_SENSORS; i++)
	{
		if(sensors[i].id == id)
		{
			return i;
		}
	}
	
	return -1;
}

uint8_t get_value_on_demand_sensors_ids(char value_on_demand_sensors_ids[NUMBER_OF_SENSORS])
{
	uint8_t value_on_demand_sensors_count = 0;
	
	uint8_t i;
	for(i = 0; i < NUMBER_OF_SENSORS; i++)
	{
		if(sensors[i].type == VALUE_ON_DEMAND)
		{
			value_on_demand_sensors_ids[value_on_demand_sensors_count++] = sensors[i].id;
		}
	}
	
	return value_on_demand_sensors_count;
}
