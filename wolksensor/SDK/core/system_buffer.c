/*
 * system_buffer.c
 *
 * Created: 1/29/2015 2:54:55 PM
 *  Author: btomic
 */ 

#include "system_buffer.h"
#include "platform_specific.h"
#include "logger.h"

circular_buffer_t system_buffer NO_INIT_MEMORY;
static system_t system_storage[SYSTEM_BUFFER_SIZE] NO_INIT_MEMORY;

void init_system_buffer(bool clear)
{
	LOG_PRINT(1, PSTR("Init system buffer, clear %u\r\n"), clear);
	circular_buffer_init(&system_buffer, system_storage, SYSTEM_BUFFER_SIZE, sizeof(system_t), true, clear);
	LOG_PRINT(2, PSTR("System buffer size after init %u\r\n"), circular_buffer_size(&system_buffer));
}

void remove_system_data(uint16_t count)
{
	LOG_PRINT(2, PSTR("Removing system %u items, size before %u\r\n"), count, circular_buffer_size(&system_buffer));
	circular_buffer_drop_from_beggining(&system_buffer, count);
	LOG_PRINT(2, PSTR("Removed system items, size after %u\r\n"), circular_buffer_size(&system_buffer));
}

uint16_t system_items_count(void)
{
	return circular_buffer_size(&system_buffer);
}

bool pop_system_item(system_t* system_item)
{
	return circular_buffer_pop(&system_buffer, system_item);
}

void system_buffer_clear(void)
{
	LOG(1, "Clearing system buffer");
	circular_buffer_clear(&system_buffer);
	LOG_PRINT(2, PSTR("System buffer size after clear %u\r\n"), circular_buffer_size(&system_buffer));
}

void add_communication_and_battery_data(communication_and_battery_data_t* communication_and_battery_data)
{
	system_t system;
	
	system.timestamp = rtc_get_ts();
	
	system.type = COMMUNICATION_AND_BATTERY_DATA;
	memcpy(&system.data.communication_and_battery_data, communication_and_battery_data, sizeof(communication_and_battery_data_t));
	
	circular_buffer_add(&system_buffer, &system);
}

void add_communication_protocol_data(communication_protocol_type_data_t* communication_protocol_type_data)
{
	system_t system;

	system.timestamp = rtc_get_ts();

	system.type = COMMUNICATION_PROTOCOL_DATA;
	memcpy(&system.data.communication_protocol_type_data, communication_protocol_type_data, sizeof(communication_protocol_type_data_t));

	circular_buffer_add(&system_buffer, &system);
}

void add_system_error(system_error_t* system_error)
{
	system_t system;
	
	system.timestamp = rtc_get_ts();
	
	system.type = SYSTEM_ERROR;
	memcpy(&system.data.system_error, system_error, sizeof(system_error_t));
	
	circular_buffer_add(&system_buffer, &system);
}
