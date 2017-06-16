#ifndef COMMANDS_H_
#define COMMANDS_H_

#include "circular_buffer.h"
#include "platform_specific.h"
#include "chrono.h"
#include "sensors.h"
#include "actuators.h"


#ifdef __cplusplus
extern "C"
{
#endif

#define COMMAND_TERMINATOR ';'
#define COMMAND_ARGUMENT_SEPARATOR ' '

#define COMMAND_NAME_MAX_LENGTH 30 /* 15 just in case: SIGNATURE + terminating character + breathing space */

#define ARGUMENT_ITEMS_SEPARATOR '|'
#define ARGUMENT_ITEM_KEY_VALUE_SEPARATOR ':'
#define ARGUMENT_ITEM_VALUES_SEPARATOR ','

#define COMMAND_ARGUMENT_MAX_LENGTH 64
#define COMMAND_MAX_LENGTH (COMMAND_NAME_MAX_LENGTH + COMMAND_ARGUMENT_MAX_LENGTH + 2) /* ' ' + ';' */

#define MAX_INT_LENGTH 5

typedef enum
{
	COMMAND_BAD = 0,
	COMMAND_MAC,
	COMMAND_NOW,
	COMMAND_RELOAD,
	COMMAND_HEARTBEAT,
	COMMAND_RTC,
	COMMAND_VERSION,
	COMMAND_STATUS,
	COMMAND_READINGS,
	COMMAND_ID,
	COMMAND_SIGNATURE,
	COMMAND_URL,
	COMMAND_PORT,
	COMMAND_SSID,
	COMMAND_PASS,
	COMMAND_AUTH,
	COMMAND_MOVEMENT,
	COMMAND_ATMO,
	COMMAND_SYSTEM,
	COMMAND_STATIC_IP,
	COMMAND_STATIC_MASK,
	COMMAND_STATIC_GATEWAY,
	COMMAND_STATIC_DNS,
	COMMAND_ALARM,
	COMMAND_SET,
	COMMAND_LOCATION,
	COMMAND_SSL,
	COMMAND_MQTT_USERNAME,
	COMMAND_TEMP_OFFSET,
	COMMAND_HUMIDITY_OFFSET,
	COMMAND_PRESSURE_OFFSET,
	COMMAND_OFFSET_FACTORY,
	COMMAND_ACQUISITION
}
commands_t;

typedef struct  
{
	char id[ACTUATOR_ID_MAX_LENGTH];
	actuator_value_t target_value;
}
set_argument_t;

typedef union
{
	uint32_t uint32_argument;
	uint16_t uint16_argument;
	char string_argument[COMMAND_ARGUMENT_MAX_LENGTH];
	bool bool_argument;
	sensor_alarms_t sensors_alarms_argument[NUMBER_OF_SENSORS];
	set_argument_t set_argument;
}
command_argument_t;

typedef struct
{
	commands_t type;
	bool has_argument;
	command_argument_t argument;
}
command_t;

typedef enum
{
	COMMAND_EXECUTED_SUCCESSFULLY = 0,
	COMMAND_EXECUTED_PARTIALLY,
	COMMAND_EXECUTION_ERROR
}
command_execution_result_t;

void init_commands(void);

uint8_t process_commands_buffer(circular_buffer_t* character_buffer, circular_buffer_t* response_buffer);
bool get_command_from_buffer(circular_buffer_t* character_buffer, command_t* command);
uint8_t process_commands_string(char* commands_string, uint16_t commands_string_length, circular_buffer_t* response_buffer);

command_execution_result_t execute_command(command_t* command, circular_buffer_t* response_buffer);

command_execution_result_t cmd_mac(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_off(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_now(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_reload(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_heartbeat(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_rtc(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_status(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_version(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_readings(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_id(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_signature(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_url(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_port(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_ssid(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_pass(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_auth(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_movement(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_atmo(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_system(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_static_ip(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_static_mask(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_static_gateway(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_static_dns(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_alarm(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_set(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_location(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_ssl(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_mqtt_username(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_mqtt_password(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_temp_offset(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_humidity_offset(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_pressure_offset(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_acquisition(command_t* command, circular_buffer_t* response_buffer);
command_execution_result_t cmd_offset_factory(command_t* command, circular_buffer_t* response_buffer);

#ifdef __cplusplus
}
#endif

#endif /* COMMAND_H_ */
