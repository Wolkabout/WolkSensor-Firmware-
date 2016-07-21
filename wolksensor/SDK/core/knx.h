/*
 * logger.h
 *
 * Created: 1/22/2015 11:02:26 AM
 *  Author: btomic
 */ 

#ifndef KNX_H_
#define KNX_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "platform_specific.h"
#include "circular_buffer.h"
#include "communication_protocol.h"

void knx_init(void);

bool create_knx_temperature_message(int16_t temperature_value, circular_buffer_t* message_buffer);

bool is_knx_physical_address_set(void);
bool is_knx_group_address_set(void);

communication_protocol_process_handle_t knx_protocol_send_sensor_readings_and_system_data(circular_buffer_t* sensor_readings_buffer, circular_buffer_t* system_buffer, uint16_t* sent_sensor_readings, uint16_t* sent_system_items);
communication_protocol_process_handle_t knx_protocol_receive_commands(circular_buffer_t* commands_buffer);
communication_protocol_process_handle_t knx_protocol_disconnect(void);

communication_protocol_type_data_t get_knx_communication_result(void);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_H_ */
