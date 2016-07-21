#ifndef WIFI_COMMUNICATION_MODULE_DEPENDENCIES_H_
#define WIFI_COMMUNICATION_MODULE_DEPENDENCIES_H_

#include "platform_specific.h"
#include "wifi_communication_module.h"

typedef struct
{	
	bool (*wifi_start)(void);
	bool (*wifi_connect)(char* ssid, char* password, uint8_t auth_type);
	bool (*wifi_disconnect)(void);
	bool (*wifi_stop)(void);
	bool (*wifi_reset)(void);
	
	int (*wifi_open_socket)(char* address, uint16_t port, bool secure);
	int (*wifi_open_udp_socket)(char* address, uint16_t port);
	bool (*wifi_close_socket)(int socket_id);
	int (*wifi_send)(int socket, uint8_t* buffer, uint16_t length);
	int (*wifi_send_to)(int socket, uint8_t* buffer, uint16_t size, char* multicast_address, uint16_t multicast_port);
	int (*wifi_receive)(int socket, uint8_t* buffer, uint16_t length);
	int (*wifi_receive_from)(int socket, uint8_t* buffer, uint16_t size, char* address, uint16_t* port);
	
	uint32_t (*get_ip_address)(char* ip_address);
	
	uint16_t (*serialize_wifi_platform_specific_error_code)(uint32_t error_code, char* buffer);
	
	void (*add_second_expired_listener)(void (*listener)(void));
	void (*add_milisecond_expired_listener)(void (*listener)(void));
	
	void (*add_wifi_connected_listener)(void (*listener)(void));
	void (*add_wifi_ip_address_acquired_listener)(void (*listener)(void));
	void (*add_wifi_disconnected_listener)(void (*listener)(void));
	void (*add_wifi_error_listener)(void (*listener)(void));
	void (*add_wifi_socket_closed_listener)(void (*listener)(void));
	void (*add_wifi_platform_specific_error_code_listener)(void (*listener)(uint32_t error_code));
}
wifi_communication_module_dependencies_t;

extern wifi_communication_module_dependencies_t wifi_communication_module_dependencies; 


#endif /* WIFI_COMMUNICATION_MODULE_DEPENDENCIES_H_ */
