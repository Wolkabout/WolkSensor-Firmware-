/*
 * logger.h
 *
 * Created: 1/22/2015 11:02:26 AM
 *  Author: btomic
 */ 

#ifndef WIFI_COMMUNICATION_MODULE_H_
#define WIFI_COMMUNICATION_MODULE_H_

#include "platform_specific.h"
#include "state_machine.h"
#include "circular_buffer.h"
#include "system_buffer.h"
#include "event.h"
#include "communication_module.h"
#include "udp_communication_module.h"
#include "tcp_communication_module.h"

#ifdef __cplusplus
extern "C"
{
#endif

void init_wifi_communication_module(void);

typedef enum
{
	STATE_WIFI_STARTING = 0,
	STATE_WIFI_STARTED,
		STATE_WIFI_DISCONNECTED,
		STATE_WIFI_CONNECTING,
			STATE_WIFI_CONNECTING_TO_AP,
			STATE_WIFI_ACQUIRING_IP_ADDRESS,
		STATE_WIFI_CONNECTED,
			STATE_WIFI_SEND,
			STATE_WIFI_SEND_TO,
			STATE_WIFI_RECEIVE,
			STATE_WIFI_RECEIVE_FROM,
			STATE_WIFI_CLOSING_SOCKET,
		STATE_WIFI_DISCONNECTING,
		STATE_WIFI_RESET,
	STATE_WIFI_STOPPING,
	STATE_WIFI_STOPPED
}
wifi_communication_module_states_t;

extern state_machine_state_t wifi_communication_module_state_machine;
extern state_machine_state_t wifi_communication_module_states[16];

communication_module_process_handle_t wifi_communication_module_stop(void);
communication_module_process_handle_t wifi_communication_module_connect(void);
communication_module_process_handle_t wifi_communication_module_send(uint8_t* data_in, uint16_t data_in_size);
communication_module_process_handle_t wifi_communication_module_send_to(uint8_t* data_in, uint16_t data_in_size);
communication_module_process_handle_t wifi_communication_module_receive(uint8_t* buffer_out, uint16_t buffer_out_size, uint16_t* received_data_size_out);
communication_module_process_handle_t wifi_communication_module_receive_from(uint8_t* buffer_out, uint16_t buffer_out_size, uint16_t* received_data_size_out);
communication_module_process_handle_t wifi_communication_module_disconnect(void);
communication_module_process_handle_t wifi_communication_module_close_socket(void);

communication_module_type_data_t get_wifi_communication_result(void);

void get_wifi_communication_module_status(char* status, uint16_t status_length);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_COMMUNICATION_MODULE_H_ */
