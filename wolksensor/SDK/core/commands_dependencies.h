 #ifndef COMMANDS_DEPENDENCIES_H_
#define COMMANDS_DEPENDENCIES_H_

#include "platform_specific.h"
#include "communication_module.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct 
{
	uint8_t bssid[6];
	char ssid[30];
	int8_t rssi;
}
wifi_network_t;

typedef struct
{
	void (*exchange_data)(void);
	void (*reset)(void);
	void (*start_heartbeat)(uint16_t heartbeat);
	void (*get_application_status)(char* status, uint16_t status_length);
	bool (*set_actuator)(char* id, actuator_value_t value);
	bool (*get_actuator_state)(char* id);
	bool (*is_static_ip_set)(void);
	uint8_t (*get_surroundig_wifi_networks)(wifi_network_t* networks, uint8_t networks_size);
	communication_module_process_handle_t (*wifi_communication_module_disconnect)(void);
	communication_module_process_handle_t (*communication_module_close_socket)(void);
}
commands_dependencies_t;

extern commands_dependencies_t commands_dependencies;

#ifdef __cplusplus
}
#endif

#endif /* COMMANDS_DEPENDENCIES_H_ */
