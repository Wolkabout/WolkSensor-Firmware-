#ifndef TCP_COMMUNICATION_MODULE_H_
#define TCP_COMMUNICATION_MODULE_H_

#include "platform_specific.h"
#include "system_buffer.h"
#include "communication_module.h"

#ifdef __cplusplus
extern "C"
{
#endif

void init_tcp_communication_module(void);

communication_module_process_handle_t tcp_communication_module_send(uint8_t* data_in, uint16_t data_in_size);
communication_module_process_handle_t tcp_communication_module_receive(uint8_t* buffer_out, uint16_t buffer_out_size, uint16_t* received_data_size_out);
communication_module_process_handle_t tcp_communication_module_close_socket(void);

communication_module_type_data_t get_tcp_communication_module_result(void);

void get_tcp_communication_module_status(char* status, uint16_t status_length);

#ifdef __cplusplus
}
#endif

#endif /* TCP_COMMUNICATION_MODULE_H_ */
