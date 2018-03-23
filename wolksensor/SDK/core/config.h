#ifndef CONFIG_H_
#define CONFIG_H_

#include "platform_specific.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_PRESHARED_KEY_SIZE 30
#define MAX_DEVICE_ID_SIZE 30

#define MAX_WIFI_SSID_SIZE 32
#define MAX_WIFI_PASSWORD_SIZE 63
#define MIN_WIFI_PASSWORD_SIZE_WEP 10
#define MAX_WIFI_PASSWORD_SIZE_WEP 26

#define MAX_WIFI_STATIC_IP_SIZE 30
#define MAX_WIFI_STATIC_MASK_SIZE 30
#define MAX_WIFI_STATIC_GATEWAY_SIZE 30
#define MAX_WIFI_STATIC_DNS_SIZE 30

#define MAX_SERVER_IP_SIZE 29
#define MAX_HOSTNAME_SIZE 29

#define DEFAULT_SYSTEM_HEARTBEAT 10

#define MQTT_USERNAME_SIZE 30
#define MQTT_PASSWORD_SIZE 30

#define MAX_PORT_NUMBER	65535

#define TEMPERATURE_OFFSET_MIN	-20
#define TEMPERATURE_OFFSET_MAX	37
#define HUMIDITY_OFFSET_MIN		-30
#define HUMIDITY_OFFSET_MAX		30
#define PRESSURE_OFFSET_MIN		-100
#define PRESSURE_OFFSET_MAX		100

typedef enum
{
	CFG_SYSTEM_HEARTBEAT = 0,

	CFG_SERVER_IP,
	CFG_SERVER_PORT,
	
	CFG_DEVICE_ID,
	CFG_DEVICE_PRESHARED_KEY,

	CFG_WIFI_SSID,
	CFG_WIFI_PASS,
	CFG_WIFI_AUTH,

	CFG_MOVEMENT,
	CFG_ATMO,
	
	CFG_STATE,
	
	CFG_WIFI_STATIC_IP,
	CFG_WIFI_STATIC_MASK,
	CFG_WIFI_STATIC_GATEWAY,
	CFG_WIFI_STATIC_DNS,
	
	CFG_MAC,
	
	CFG_KNX_PHYSICAL_ADDRESS,
	CFG_KNX_GROUP_ADDRESS,
	
	CFG_KNX_MULTICAST_ADDRESS,
	CFG_KNX_MULTICAST_PORT,
	
	CFG_KNX_NAT,
	
	CFG_LOCATION,
	
	CFG_SSL,
	
	CFG_MQTT_USERNAME,
	CFG_MQTT_PASSWORD,

	CFG_OFFSET,
	CFG_OFFSET_FACTORY,

	CFG_WIFI_SSID_SEC,
	CFG_WIFI_PASS_SEC,
	CFG_WIFI_PASS_THIRD,

	CFG_EMPTY = 255
}
cfg_t;

extern uint8_t device_preshared_key[MAX_PRESHARED_KEY_SIZE];
extern char	device_id[MAX_DEVICE_ID_SIZE];

extern uint16_t system_heartbeat;

extern char wifi_ssid[MAX_WIFI_SSID_SIZE + 1];
extern char wifi_password[MAX_WIFI_PASSWORD_SIZE + 1];
extern uint8_t wifi_auth_type;

extern char wifi_static_ip[MAX_WIFI_STATIC_IP_SIZE];
extern char wifi_static_mask[MAX_WIFI_STATIC_MASK_SIZE];
extern char wifi_static_gateway[MAX_WIFI_STATIC_GATEWAY_SIZE];
extern char wifi_static_dns[MAX_WIFI_STATIC_DNS_SIZE];

extern bool ssl;

extern unsigned char mac_address_nwmem[6];

extern char server_ip[MAX_SERVER_IP_SIZE + 1];
extern char hostname[MAX_HOSTNAME_SIZE + 1];

extern uint16_t server_port;

extern bool movement_status;
extern bool atmo_status;

extern bool location;

extern char mqtt_username[MQTT_USERNAME_SIZE];
extern char	mqtt_password[MQTT_PASSWORD_SIZE];

extern uint32_t atmo_offset[3];
extern uint32_t atmo_offset_factory[5];

bool load_device_id(void);
bool load_device_preshared_key(void);

bool load_system_heartbeat(void);

bool load_wifi_ssid(void);
bool load_wifi_password(void);
bool load_wifi_auth_type(void);
bool load_wifi_static_ip(void);
bool load_wifi_static_mask(void);
bool load_wifi_static_gateway(void);
bool load_wifi_static_dns(void);
bool load_wifi_mac_address(void);

bool load_server_ip(void);
bool load_server_port(void);

bool load_movement_status(void);
bool load_atmo_status(void);

bool load_location_status(void);

bool load_ssl_status(void);

bool load_mqtt_username(void);
bool load_mqtt_password(void);
/*
bool load_temp_offset_status(void);
bool load_humidity_offset_status(void);
bool load_pressure_offset_status(void);

bool load_temp_offset_factory_status(void);
*/

bool load_offset_status(void);
bool load_offset_factory_status(void);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_H_ */
