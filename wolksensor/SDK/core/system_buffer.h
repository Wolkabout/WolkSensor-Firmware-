/*
 * system_buffer.h
 *
 * Created: 1/29/2015 2:54:39 PM
 *  Author: btomic
 */ 


#ifndef SYSTEM_BUFFER_H_
#define SYSTEM_BUFFER_H_

#include "circular_buffer.h"
#include "platform_specific.h"
#include "system.h"
#include "chrono.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define SYSTEM_BUFFER_SIZE 60

extern circular_buffer_t system_buffer NO_INIT_MEMORY;

void init_system_buffer(bool clear);
void add_system_data(system_data_t* system_data);
void add_communication_and_battery_data(communication_and_battery_data_t* communication_and_battery_data);
void add_communication_protocol_data(communication_protocol_type_data_t* communication_protocol_type_data);
void add_system_error(system_error_t* system_error);
void remove_system_data(uint16_t count);
uint16_t system_items_count(void);
void system_buffer_clear(void);
bool pop_system_item(system_t* system_item);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_BUFFER_H_ */
