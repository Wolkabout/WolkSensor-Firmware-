/*
 * logger.h
 *
 * Created: 1/22/2015 11:02:26 AM
 *  Author: btomic
 */ 

#ifndef MQTT_H_
#define MQTT_H_

#include "platform_specific.h"
#include "communication_protocol.h"
#include "system.h"

#ifdef __cplusplus
extern "C"
{
#endif

void mqtt_protocol_init(void);

communication_protocol_process_handle_t mqtt_protocol_send_sensor_readings_and_system_data(circular_buffer_t* sensor_readings_buffer, circular_buffer_t* system_buffer, uint16_t* sent_sensor_readings, uint16_t* sent_system_items);
communication_protocol_process_handle_t mqtt_protocol_send_actuator_state(actuator_t* actuator, actuator_state_t* actuator_state);
communication_protocol_process_handle_t mqtt_protocol_receive_commands(circular_buffer_t* commands_buffer);
communication_protocol_process_handle_t mqtt_protocol_disconnect(void);

communication_protocol_type_data_t get_mqtt_communication_result(void);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_H_ */
