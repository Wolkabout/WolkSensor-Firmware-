#ifndef UDP_COMMUNICATION_MODULE_DEPENDENCIES_H_
#define UDP_COMMUNICATION_MODULE_DEPENDENCIES_H_

#include "platform_specific.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{	
	int (*open_udp_socket)(char* address, uint16_t port);
	bool (*close_socket)(int socket_id);
	int (*send_to)(int socket, uint8_t* buffer, uint16_t size, char* address, uint16_t port);
	int (*receive_from)(int socket, uint8_t* buffer, uint16_t size, char* address, uint16_t* port);
	
	uint16_t (*serialize_platform_specific_error_code)(uint32_t error_code, char* buffer);
	
	void (*add_second_expired_listener)(void (*listener)(void));
	void (*add_milisecond_expired_listener)(void (*listener)(void));
	void (*add_platform_specific_error_code_listener)(void (*listener)(uint32_t operation_code));
}
udp_communication_module_dependencies_t;

extern udp_communication_module_dependencies_t udp_communication_module_dependencies;

#ifdef __cplusplus
}
#endif

#endif /* UDP_COMMUNICATION_MODULE_DEPENDENCIES_H_ */
