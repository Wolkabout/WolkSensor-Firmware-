#ifndef TCP_COMMUNICATION_MODULE_DEPENDENCIES_H_
#define TCP_COMMUNICATION_MODULE_DEPENDENCIES_H_

#include "platform_specific.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{	
	int (*open_socket)(char* address, uint16_t port, bool secure);
	bool (*close_socket)(int socket_id);
	int (*receive)(int socket, uint8_t* buffer, uint16_t length);
	int (*send)(int socket, uint8_t* buffer, uint16_t length);
	
	uint16_t (*serialize_platform_specific_error_code)(uint32_t error_code, char* buffer);
	
	void (*add_second_expired_listener)(void (*listener)(void));
	void (*add_milisecond_expired_listener)(void (*listener)(void));
	void (*add_socket_closed_listener)(void (*listener)(void));
	void (*add_platform_specific_error_code_listener)(void (*listener)(uint32_t operation_code));
}
tcp_communication_module_dependencies_t;

extern tcp_communication_module_dependencies_t tcp_communication_module_dependencies;

#ifdef __cplusplus
}
#endif

#endif /* TCP_COMMUNICATION_MODULE_DEPENDENCIES_H_ */
