#ifndef COMMUNICATION_PROTOCOL_H_
#define COMMUNICATION_PROTOCOL_H_

#include "platform_specific.h"
#include "system.h"
#include "circular_buffer.h"
#include "actuators.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef bool (*communication_protocol_process_handle_t)(void);

typedef struct  
{
	communication_protocol_process_handle_t (*send_sensor_readings_and_system_data)(circular_buffer_t* sensor_readings_buffer, circular_buffer_t* system_buffer, uint16_t* sent_sensor_readings, uint16_t* sent_system_items);
	communication_protocol_process_handle_t (*send_actuator_state)(actuator_t* actuator, actuator_state_t* actuator_state);
	communication_protocol_process_handle_t (*receive_commands)(circular_buffer_t* commands_buffer);
	communication_protocol_process_handle_t (*disconnect)(void);
	communication_protocol_type_data_t (*get_communication_result)(void);
}
communication_protocol_t;

extern communication_protocol_t communication_protocol;

#ifdef __cplusplus
}
#endif

#endif /* COMMUNICATION_PROTOCOL_H_ */
