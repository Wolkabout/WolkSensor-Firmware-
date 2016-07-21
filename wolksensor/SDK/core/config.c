#include "config.h"
#include "logger.h"
#include "global_dependencies.h"
#include "wolksensor_dependencies.h"

uint8_t device_preshared_key[MAX_PRESHARED_KEY_SIZE];
char device_id[MAX_DEVICE_ID_SIZE];

uint16_t system_heartbeat = DEFAULT_SYSTEM_HEARTBEAT;

char wifi_ssid[MAX_WIFI_SSID_SIZE];
char wifi_password[MAX_WIFI_PASSWORD_SIZE];
uint8_t wifi_auth_type = WIFI_SECURITY_UNSECURED;

char wifi_static_ip[MAX_WIFI_STATIC_IP_SIZE];
char wifi_static_mask[MAX_WIFI_STATIC_MASK_SIZE];
char wifi_static_gateway[MAX_WIFI_STATIC_GATEWAY_SIZE];
char wifi_static_dns[MAX_WIFI_STATIC_DNS_SIZE];

bool ssl = true;

unsigned char mac_address_nwmem[6];

char server_ip[MAX_SERVER_IP_SIZE];
uint16_t server_port = 8883;

bool movement_status = false;
bool atmo_status = true;

uint8_t knx_physical_address[2] = {0, 0};
uint8_t knx_group_address[2] = {0, 0};
	
char knx_multicast_address[MAX_KNX_MULTICAST_ADDRESS_SIZE];
uint16_t knx_multicast_port;

bool knx_nat = false;

bool location = false;

char mqtt_username[MQTT_USERNAME_SIZE];
char mqtt_password[MQTT_PASSWORD_SIZE];

bool load_device_id(void)
{
	if (global_dependencies.config_read(&device_id, CFG_DEVICE_ID, 1, sizeof(device_id)))
	{
		LOG_PRINT(1, PSTR("Device ID was read: %s\r\n"), device_id);
		return true;
	}

	LOG(1, "Unable to read device ID");
	memset(device_id, 0, sizeof(device_id));
	return false;
}

bool load_device_preshared_key(void)
{
	if (global_dependencies.config_read(&device_preshared_key, CFG_DEVICE_PRESHARED_KEY, 1, sizeof(device_preshared_key)))
	{
		LOG_PRINT(1, PSTR("Device preshared key was read: %s\r\n"), device_preshared_key);
		return true;
	}
	
	LOG(1, "Unable to read device preshared key");
	memset(device_preshared_key, 0, sizeof(device_preshared_key));
	return false;
}

bool load_system_heartbeat(void)
{
	if (global_dependencies.config_read(&system_heartbeat, CFG_SYSTEM_HEARTBEAT, 1, sizeof(system_heartbeat)))
	{
		LOG_PRINT(1, PSTR("System heartbeat read: %u \r\n"), system_heartbeat);
		return true;
	}
	
	LOG(1, "Unable to read system heartbeat");
	system_heartbeat = DEFAULT_SYSTEM_HEARTBEAT;
	return false;
}

bool load_wifi_ssid(void)
{
	if (global_dependencies.config_read(&wifi_ssid, CFG_WIFI_SSID, 1, sizeof(wifi_ssid)))
	{
		LOG_PRINT(1, PSTR("SSID was read: %s\r\n"), wifi_ssid);
		return true;
	}
	
	LOG(1, "Unable to read SSID");
	memset(wifi_ssid, 0, sizeof(wifi_ssid));
	
	return false;
}

bool load_wifi_password(void)
{
	if (global_dependencies.config_read(&wifi_password, CFG_WIFI_PASS, 1, sizeof(wifi_password)))
	{
		LOG_PRINT(1, PSTR("Password was read: %s\r\n"), wifi_password);
		return true;
	}

	LOG(1, "Unable to read password");
	memset(wifi_password, 0, sizeof(wifi_password));
	return false;
}

bool load_wifi_auth_type(void)
{
	if (global_dependencies.config_read(&wifi_auth_type, CFG_WIFI_AUTH, 1, sizeof(wifi_auth_type)))
	{
		LOG_PRINT(1, PSTR("Auth type was read: %u\r\n"), wifi_auth_type);
		return true;
	}
	
	LOG(1, "Unable to read auth type, using default value");
	wifi_auth_type = WIFI_SECURITY_UNSECURED;
	return false;
}

bool load_wifi_static_ip(void)
{
	if (global_dependencies.config_read(&wifi_static_ip, CFG_WIFI_STATIC_IP, 1, sizeof(wifi_static_ip)))
	{
		LOG_PRINT(1, PSTR("Static IP was read: %u \n"), wifi_static_ip);
		return true;
	}

	LOG(1, "Unable to read static IP");
	memset(wifi_static_ip, 0, sizeof(wifi_static_ip));
	return false;
}

bool load_wifi_static_mask(void)
{
	if (global_dependencies.config_read(&wifi_static_mask, CFG_WIFI_STATIC_MASK, 1, sizeof(wifi_static_mask)))
	{
		LOG_PRINT(1, PSTR("Static mask was read: %u \n"), wifi_static_mask);
		return true;
	}
	
	LOG(1, "Unable to read static MASK");
	memset(wifi_static_mask, 0, sizeof(wifi_static_mask));
	return false;
}

bool load_wifi_static_gateway(void)
{
	if (global_dependencies.config_read(&wifi_static_gateway, CFG_WIFI_STATIC_GATEWAY, 1, sizeof(wifi_static_gateway)))
	{
		LOG_PRINT(1, PSTR("Static gateway was read: %u \n"), wifi_static_gateway);
		return true;
	}

	LOG(1, "Unable to read static gateway");
	memset(wifi_static_gateway, 0, sizeof(wifi_static_gateway));
	return false;
}

bool load_wifi_static_dns(void)
{
	if (global_dependencies.config_read(&wifi_static_dns, CFG_WIFI_STATIC_DNS, 1, sizeof(wifi_static_dns)))
	{
		LOG_PRINT(1, PSTR("Static DNS was read: %u \n"), wifi_static_dns);
		return true;
	}
	
	LOG(1, "Unable to read static DNS");
	memset(wifi_static_dns, 0, sizeof(wifi_static_dns));
	return false;
}

bool load_wifi_mac_address(void)
{
	if(global_dependencies.config_read(&mac_address_nwmem, CFG_MAC, 1, sizeof(mac_address_nwmem)))
	{
		LOG_PRINT(1, PSTR("mac_address_nwmem was read: %02x \n"), mac_address_nwmem);
		return true;
	}

	LOG(1, "Unable to read mac_address_nwmem from Atmel EEPROM");
	memset(mac_address_nwmem, 0, sizeof(mac_address_nwmem));
	return false;
}

bool load_server_ip(void)
{
	if (global_dependencies.config_read(&server_ip, CFG_SERVER_IP, 1, sizeof(server_ip)))
	{
		LOG_PRINT(1, PSTR("Server IP was read: %s\r\n"), server_ip);
		
		return true;
	}

	LOG(1, "Unable to read server IP");
	memset(server_ip, 0, sizeof(server_ip));
	return false;
}

bool load_server_port(void)
{
	if (global_dependencies.config_read(&server_port, CFG_SERVER_PORT, 1, sizeof(server_port)))
	{
		LOG_PRINT(1, PSTR("Server port was read: %u\r\n"), server_port);
		return true;
	}
	
	LOG(1, "Unable to read server port");
	server_port = 8883;
	return false;
}

bool load_movement_status(void)
{
	if (global_dependencies.config_read(&movement_status, CFG_MOVEMENT, 1, sizeof(movement_status)))
	{
		movement_status = (movement_status == 0) ? false : true;
		LOG_PRINT(1, PSTR("Movement sensor status read %u\r\n"), movement_status);
		
		if(movement_status)
		{
			wolksensor_dependencies.enable_movement();
		}
		else
		{
			wolksensor_dependencies.disable_movement();
		}
		
		return true;
	}
	
	LOG(1, "Could not read movement sensor status, defaulting to OFF");
	movement_status = false;
	wolksensor_dependencies.disable_movement();
	return false;
}

bool load_atmo_status(void)
{
	if (global_dependencies.config_read(&atmo_status, CFG_ATMO, 1, sizeof(atmo_status)))
	{
		atmo_status = (atmo_status == 0) ? false : true;
		LOG_PRINT(1, PSTR("Atmo sensor status read %u\r\n"), atmo_status);
		return true;
	}
	
	LOG(1, "Could not read atmo sensor status, defaulting to ON");
	atmo_status = true;
	return false;
}

bool load_knx_physical_address(void)
{
	if (global_dependencies.config_read(&knx_physical_address, CFG_KNX_PHYSICAL_ADDRESS, 1, sizeof(knx_physical_address)))
	{
		LOG_PRINT(1, PSTR("KNX physical address read: %02x%02x\r\n"), knx_physical_address[0], knx_physical_address[1]);
		return true;
	}
	
	LOG(1, "Unable to read KNX physical address");
	memset(knx_physical_address, 0, 2);
	return false;
}

bool load_knx_group_address(void)
{
	if (global_dependencies.config_read(&knx_group_address, CFG_KNX_GROUP_ADDRESS, 1, sizeof(knx_group_address)))
	{
		LOG_PRINT(1, PSTR("KNX group address read: %02x%02x\r\n"), knx_group_address[0], knx_group_address[1]);
		return true;
	}
	
	LOG(1, "Unable to read KNX group address");
	memset(knx_group_address, 0, 2);
	return false;
}

bool load_knx_multicast_address(void)
{
	if (global_dependencies.config_read(&knx_multicast_address, CFG_KNX_MULTICAST_ADDRESS, 1, sizeof(knx_multicast_address)))
	{
		LOG_PRINT(1, PSTR("KNX multicast address was read: %s\r\n"), knx_multicast_address);
		
		return true;
	}

	LOG(1, "Unable to read KNX multicast address");
	memset(knx_multicast_address, 0, sizeof(knx_multicast_address));
	
	return false;
}

bool load_knx_multicast_port(void)
{
	if (global_dependencies.config_read(&knx_multicast_port, CFG_KNX_MULTICAST_PORT, 1, sizeof(knx_multicast_port)))
	{
		LOG_PRINT(1, PSTR("KNX multicast port was read: %u\r\n"), knx_multicast_port);
		return true;
	}
	
	LOG(1, "Unable to read KNX multicast port");
	knx_multicast_port = 0;
	
	return false;
}

bool load_knx_nat(void)
{
	if (global_dependencies.config_read(&knx_nat, CFG_KNX_NAT, 1, sizeof(knx_nat)))
	{
		knx_nat = (knx_nat == 0) ? false : true;
		LOG_PRINT(1, PSTR("Knx use nat read %u\r\n"), knx_nat);
		return true;
	}
	
	LOG(1, "Could not read knx nat status, defaulting to OFF");
	knx_nat = false;
	
	return false;
}

bool load_location_status(void)
{
	if (global_dependencies.config_read(&location, CFG_LOCATION, 1, sizeof(location)))
	{
		location = (location == 0) ? false : true;
		LOG_PRINT(1, PSTR("Location status read %u\r\n"), location);
		return true;
	}
	
	LOG(1, "Could not read location status, defaulting to OFF");
	location = false;
	
	return false;
}

bool load_ssl_status(void)
{
	if (global_dependencies.config_read(&ssl, CFG_SSL, 1, sizeof(ssl)))
	{
		ssl = (ssl == 0) ? false : true;
		LOG_PRINT(1, PSTR("SSL status read %u\r\n"), ssl);
		return true;
	}
	
	LOG(1, "Could not read ssl status, defaulting to OFF");
	ssl = true;
	
	return false;
}

bool load_mqtt_username(void)
{
	if (global_dependencies.config_read(&mqtt_username, CFG_MQTT_USERNAME, 1, sizeof(mqtt_username)))
	{
		LOG_PRINT(1, PSTR("Mqtt username was read: %s\r\n"), mqtt_username);
		return true;
	}

	LOG(1, "Unable to read mqtt username");
	memset(mqtt_username, 0, sizeof(mqtt_username));
	return false;
}

bool load_mqtt_password(void)
{
	if (global_dependencies.config_read(&mqtt_password, CFG_MQTT_PASSWORD, 1, sizeof(mqtt_password)))
	{
		LOG_PRINT(1, PSTR("Mqtt password was read: %s\r\n"), mqtt_password);
		return true;
	}

	LOG(1, "Unable to read mqtt password");
	memset(mqtt_password, 0, sizeof(mqtt_password));
	return false;
}