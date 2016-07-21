#ifndef UDP_COMMUNICATION_MODULE_H_
#define UDP_COMMUNICATION_MODULE_H_

#include "platform_specific.h"
#include "system_buffer.h"
#include "communication_module.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern char bind_address[15];
extern uint16_t bind_port;

extern char destination_address[15];
extern uint16_t destination_port;

void init_udp_communication_module(void);

communication_module_process_handle_t udp_communication_module_send(uint8_t* data_in, uint16_t data_in_size);
communication_module_process_handle_t udp_communication_module_receive(uint8_t* buffer_out, uint16_t buffer_out_size, uint16_t* received_data_size_out);
communication_module_process_handle_t udp_communication_module_close_socket(void);

communication_module_type_data_t get_udp_communication_module_result(void);

void get_udp_communication_module_status(char* status, uint16_t status_length);

#ifdef __cplusplus
}
#endif

#endif /* UDP_COMMUNICATION_MODULE_H_ */
