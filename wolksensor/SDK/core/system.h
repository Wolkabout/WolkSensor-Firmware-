/*
 * logger.h
 *
 * Created: 1/22/2015 11:02:26 AM
 *  Author: btomic
 */ 

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "platform_specific.h"

#ifdef __cplusplus
extern "C"
{
#endif

/************** System wide error ***************************/

typedef enum
{
	SYSTEM_RESET = 0,
	SYSTEM_BROWNOUT
}
system_error_type_t;

typedef union
{
	uint8_t system_reset_reason;
}
system_error_data_t;

typedef struct
{
	system_error_type_t type;
	system_error_data_t data;
}
system_error_t;

/******************** Communication module data **************/

typedef enum
{
	COMMUNICATION_MODULE_WIFI = 0,
	COMMUNICATION_MODULE_TCP,
	COMMUNICATION_MODULE_UDP
}
communication_module_type_t;

typedef enum
{
	WIFI_SUCCESS = 0x00,
	WIFI_ERROR = 0x10,
	WIFI_TIMEOUT = 0x20,
	WIFI_DISCONNECTED = 0x30,
	WIFI_OPERATION_FAILED = 0x40,
	WIFI_PARAMETERS_MISSING = 0x50,
	WIFI_SOCKET_CLOSED = 0x60
}
wifi_communication_module_error_type_t;

typedef struct
{
	uint8_t error;
	uint32_t platform_specific_error_code;
	
	uint16_t connect_to_ap_time;
	uint16_t acquire_ip_address_time;
	uint16_t connect_to_server_time;
	uint16_t data_exchange_time;
	uint16_t disconnect_time;
}
wifi_communication_module_data_t;

typedef enum
{
	TCP_SUCCESS = 0x00,
	TCP_OPERATION_FAILED = 0x10,
	TCP_TIMEOUT = 0x20,
	TCP_SOCKET_CLOSED = 0x30,
	TCP_PARAMETERS_MISSING = 0x40
}
tcp_communication_module_error_type_t;

typedef struct
{
	uint8_t error;
	uint32_t platform_specific_error_code;
	
	uint16_t connect_to_server_time;
	uint16_t data_exchange_time;
	uint16_t disconnect_time;
}
tcp_communication_module_data_t;

typedef enum
{
	UDP_SUCCESS = 0x00,
	UDP_OPERATION_FAILED = 0x10,
	UDP_PARAMETERS_MISSING = 0x20
}
udp_communication_module_error_type_t;

typedef struct
{
	uint8_t error;
	int platform_specific_error_code;
	
	uint16_t connect_to_server_time;
	uint16_t data_exchange_time;
	uint16_t disconnect_time;
}
udp_communication_module_data_t;

typedef union
{
	wifi_communication_module_data_t wifi_communication_module_data;
	tcp_communication_module_data_t tcp_communication_module_data;
	udp_communication_module_data_t udp_communication_module_data;
}
communication_module_data_t;

typedef struct
{
	communication_module_type_t type;
	communication_module_data_t data;
}
communication_module_type_data_t;

void append_wifi_communication_module_data(wifi_communication_module_data_t* operation_data, wifi_communication_module_data_t* total_data);
void append_communication_module_type_data(communication_module_type_data_t* operation_data, communication_module_type_data_t* total_data);

bool is_wifi_communication_module_success(wifi_communication_module_data_t* wifi_communication_module_data);
bool is_communication_module_success(communication_module_type_data_t* communication_module_type_data);

/*************************************************************/
/************** Communication protocol data ******************/

typedef enum
{
	COMMUNICATION_PROTOCOL_MQTT = 0,
	COMMUNICATION_PROTOCOL_KNX
}
communication_protocol_type_t;

typedef enum
{
	MQTT_SUCCESS = 0x00,
	ERROR_SENDING_MQTT_MESSAGE = 0x10,
	ERROR_RECEIVING_MQTT_MESSAGE = 0x20,
	ERROR_INCORRECT_MQTT_MESSAGE_RECEIVED = 0x30,
	ERROR_MQTT_PARAMETERS_MISSING = 0x40
}
mqtt_communication_protocol_error_type_t;

typedef struct
{
	uint8_t error;
}
mqtt_communication_protocol_data_t;

typedef union
{
	mqtt_communication_protocol_data_t mqtt_communication_protocol_data;
}
communication_protocol_data_t;

typedef struct
{
	communication_protocol_type_t type;
	communication_protocol_data_t data;
	communication_module_type_data_t communication_module_type_data;
}
communication_protocol_type_data_t;

typedef struct
{
	communication_protocol_type_data_t communication_protocol_type_data;
	uint16_t battery_min_voltage;
}
communication_and_battery_data_t;

void append_mqtt_communication_protocol_data(mqtt_communication_protocol_data_t* operation_data, mqtt_communication_protocol_data_t* total_data);
void append_communication_protocol_type_data(communication_protocol_type_data_t* operation_data, communication_protocol_type_data_t* total_data);

bool is_mqtt_communication_protocol_success(mqtt_communication_protocol_data_t* mqtt_communication_protocol_data);
bool is_communication_protocol_success(communication_protocol_type_data_t* communication_protocol_type_data);

void append_communication_and_battery_data(communication_and_battery_data_t* operation_data, communication_and_battery_data_t* total_data);

/*************************************************************/

typedef enum
{
	SYSTEM_ERROR = 0,
	COMMUNICATION_PROTOCOL_DATA,
	COMMUNICATION_AND_BATTERY_DATA,
}
system_type_t;

typedef union
{
	system_error_t system_error;
	communication_protocol_type_data_t communication_protocol_type_data;
	communication_and_battery_data_t communication_and_battery_data;
}
system_data_t;

typedef struct
{
	uint32_t timestamp;
	system_type_t type;
	system_data_t data;
}
system_t;

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_H_ */
