#include "command_parser.h"
#include "commands.h"
#include "logger.h"

typedef struct
{
	commands_t type;
	char token[COMMAND_NAME_MAX_LENGTH];
}
command_table_t;

static const command_table_t command_table[] PROGMEM =
{
	{ COMMAND_BAD, "" },
	{ COMMAND_MAC, "MAC" },
	{ COMMAND_NOW, "NOW" },
	{ COMMAND_RELOAD, "RELOAD" },
	{ COMMAND_HEARTBEAT, "HEARTBEAT" },
	{ COMMAND_RTC, "RTC" },
	{ COMMAND_VERSION, "VERSION" },
	{ COMMAND_STATUS, "STATUS" },
	{ COMMAND_READINGS, "READINGS" },
	{ COMMAND_ID, "ID" },
	{ COMMAND_SIGNATURE, "SIGNATURE" },
	{ COMMAND_URL, "URL" },
	{ COMMAND_PORT, "PORT" },
	{ COMMAND_SSID, "SSID" },
	{ COMMAND_PASS, "PASS" },
	{ COMMAND_AUTH, "AUTH" },
	{ COMMAND_MOVEMENT, "MOVEMENT" },
	{ COMMAND_ATMO, "ATMO" },
	{ COMMAND_SYSTEM, "SYSTEM" },
	{ COMMAND_STATIC_IP, "STATIC_IP" },
	{ COMMAND_STATIC_MASK, "STATIC_MASK" },
	{ COMMAND_STATIC_GATEWAY, "STATIC_GATEWAY" },
	{ COMMAND_STATIC_DNS, "STATIC_DNS" },
	{ COMMAND_ALARM, "ALARM" },
	{ COMMAND_SET, "SET" },
	{ COMMAND_KNX_PHYSICAL_ADDRESS, "KNX_PHYSICAL_ADDRESS" },
	{ COMMAND_KNX_GROUP_ADDRESS, "KNX_GROUP_ADDRESS" },
	{ COMMAND_KNX_MULTICAST_ADDRESS, "KNX_MULTICAST_ADDRESS" },
	{ COMMAND_KNX_MULTICAST_PORT, "KNX_MULTICAST_PORT" },
	{ COMMAND_KNX_NAT, "KNX_NAT" },
	{ COMMAND_LOCATION, "LOCATION" },
	{ COMMAND_SSL, "SSL" }
};

/*
* Used to split values by |
*/
static uint8_t find_argument_item(char* items_list)
{
	uint8_t length = strlen(items_list);
	if(length == 0)
	{
		return 0;
	}
	
	uint8_t end_position = 0;
	while((items_list[end_position++] != ARGUMENT_ITEMS_SEPARATOR) && (end_position < length));

	return end_position;
}

static bool parse_alarm_argument_item(char* argument, uint8_t size, char* sensor_id, sensor_alarms_t* sensor_alarms)
{	
	LOG_PRINT(1, PSTR("Alarm item %s \r\n"), argument);
	
	*sensor_id = argument[0];
	
	LOG_PRINT(1, PSTR("Alarm for %c \r\n"), *sensor_id);
	
	if(argument[1] != ARGUMENT_ITEM_KEY_VALUE_SEPARATOR)
	{
		return false;
	}
	
	/* search for ',' */
	uint8_t values_separator_position = 0;
	while(argument[values_separator_position++] != ARGUMENT_ITEM_VALUES_SEPARATOR)
	{
		if(values_separator_position == size) /* end reached */
		{
			return false;
		}
	}
	
	int alarm_value;
	/* low value */
	
	
	if(strncmp_P(argument + 2, PSTR("OFF"), 3) == 0)
	{
		LOG(1, "Alarm low off");
		sensor_alarms->alarm_low.enabled = false;
	}
	else
	{
		alarm_value = atoi(argument + 2);
		sensor_alarms->alarm_low.enabled = true;
		sensor_alarms->alarm_low.value = alarm_value;
		LOG_PRINT(1, PSTR("Alarm low %d \r\n"), sensor_alarms->alarm_low.value);
	}
	
	/* high value */
	if(strncmp_P(argument + values_separator_position, PSTR("OFF"), 3) == 0)
	{
		LOG(1, "Alarm high off");
		sensor_alarms->alarm_high.enabled = false;
	}
	else
	{
		alarm_value = atoi(argument + values_separator_position);
		sensor_alarms->alarm_high.enabled = true;
		sensor_alarms->alarm_high.value = alarm_value;
		LOG_PRINT(1, PSTR("Alarm high %d \r\n"), sensor_alarms->alarm_high.value);
	}
	
	return true;
}

static bool parse_alarm_argument(command_t* command, char* argument)
{		
	LOG_PRINT(1, PSTR("Alarm argument %s \r\n"), argument);
	
	memcpy(&command->argument.sensors_alarms_argument, &sensors_alarms, sizeof(sensors_alarms));
	
	uint8_t item_start_position = 0;
	uint8_t item_end_position = find_argument_item(argument);
	
	while(item_start_position != item_end_position)
	{
		char sensor_id;
		sensor_alarms_t sensor_alarms;
		
		if(!parse_alarm_argument_item(argument + item_start_position, item_end_position, &sensor_id, &sensor_alarms))
		{
			return false;
		}

		int8_t sensor_index = get_index_of_sensor(sensor_id);
		if(sensor_index >= 0)
		{
			command->argument.sensors_alarms_argument[sensor_index].alarm_low.enabled = sensor_alarms.alarm_low.enabled;
			command->argument.sensors_alarms_argument[sensor_index].alarm_low.value = sensor_alarms.alarm_low.value;
			
			command->argument.sensors_alarms_argument[sensor_index].alarm_high.enabled = sensor_alarms.alarm_high.enabled;
			command->argument.sensors_alarms_argument[sensor_index].alarm_high.value = sensor_alarms.alarm_high.value;
		}
		
		item_start_position = item_end_position;
		item_end_position += find_argument_item(argument + item_end_position);
	}

	return true;
}

static bool parse_set_argument(command_t* command, char* argument)
{
	char* id = strtok(argument, ":");
	if(id == NULL)
	{
		return false;
	}
	
	// check if there is such sensor available
	uint8_t i = 0;
	while((i < NUMBER_OF_ACTUATORS) && (strcmp(id, actuators[i].id) != 0))
	{
		i++;
	}
	
	if(i == NUMBER_OF_ACTUATORS) // not found
	{
		LOG(1, "No such actuator");
		return false;
	}
	
	strcpy(command->argument.set_argument.id, id);
	
	LOG_PRINT(1, PSTR("Actuator id %s\r\n"), command->argument.set_argument.id);
	
	char* value = strtok(NULL, ":");
	actuator_type_t actuator_type = actuators[i].type;
	switch(actuator_type)
	{
		case ACTUATOR_TYPE_SWITCH:
		{
			LOG(1, "Actuator type switch");
			
			if(strcmp_P(value, PSTR("ON")) == 0)
			{
				LOG(1, "Actuator value ON");
				command->argument.set_argument.target_value.switch_value = true;
				return true;
			}
			else if(strcmp_P(value, PSTR("OFF")) == 0)
			{
				LOG(1, "Actuator value OFF");
				command->argument.set_argument.target_value.switch_value = false;
				return true;
			}
			else
			{
				return false;
			}
		}
		case ACTUATOR_TYPE_DC_MOTOR:
		{
			LOG(1, "Actuator type dc motor");
			command->argument.set_argument.target_value.dc_motor_value = atoi(value);
			LOG_PRINT(1, PSTR("Actuator value %d\r\n"), command->argument.set_argument.target_value.dc_motor_value);
			return true;
		}
		case ACTUATOR_TYPE_SERVO:
		{
			LOG(1, "Actuator type servo");
			command->argument.set_argument.target_value.dc_motor_value = atoi(value);
			LOG_PRINT(1, PSTR("Actuator value %d\r\n"), command->argument.set_argument.target_value.dc_motor_value);
			return true;
		}
		default:
		{
			return false;
		}
	}
}

static bool parse_knx_physical_address_argument(command_t* command, unsigned char* argument)
{
	char* address_segments[3];
	
	uint8_t i;
	for(i = 0; i < 3; i++)
	{
		address_segments[i] = strtok((i == 0 ? argument : NULL), ".");
		if(address_segments[i] == NULL)
		{
			return false;
		}
	}
	
	memset(&command->argument.knx_address_argument, 0, sizeof(command->argument.knx_address_argument));
	
	uint8_t address_segment = atoi(address_segments[0]);
	command->argument.knx_address_argument[0] = address_segment << 4;

	address_segment = atoi(address_segments[1]);
	command->argument.knx_address_argument[0] |= (address_segment & 0x0F);
	
	address_segment = atoi(address_segments[2]);
	command->argument.knx_address_argument[1] = address_segment;
	
	return true;
}

static bool parse_knx_group_address_argument(command_t* command, unsigned char* argument)
{
	char* address_segments[3];
	
	uint8_t i;
	for(i = 0; i < 3; i++)
	{
		address_segments[i] = strtok((i == 0 ? argument : NULL), ".");
		if(address_segments[i] == NULL)
		{
			return false;
		}
	}
	
	memset(&command->argument.knx_address_argument, 0, sizeof(command->argument.knx_address_argument));
	
	uint8_t address_segment = atoi(address_segments[0]);
	command->argument.knx_address_argument[0] = address_segment << 3;
	
	address_segment = atoi(address_segments[1]);
	command->argument.knx_address_argument[0] |= (address_segment & 0x07);
	
	address_segment = atoi(address_segments[2]);
	command->argument.knx_address_argument[1] = atoi(address_segments[2]);
	
	return true;
}

static bool parse_commad_argument(command_t* command, char* argument)
{
	switch (command->type) {
		case COMMAND_MOVEMENT:
		case COMMAND_ATMO:
		case COMMAND_KNX_NAT:
		case COMMAND_LOCATION:
		case COMMAND_SSL:
		{
			if(!strcmp_P(argument, PSTR("ON")))
			{
				command->argument.bool_argument = true;
				return true;
			}
			else if(!strcmp_P(argument, PSTR("OFF")))
			{
				command->argument.bool_argument = false;
				return true;
			}
			else
			{
				return false;
			}
		}
		case COMMAND_HEARTBEAT:
		case COMMAND_PORT:
		case COMMAND_KNX_MULTICAST_PORT:
		{
			uint64_t value = atoi(argument);
			command->argument.uint32_argument = value;
			return true;
		}
		case COMMAND_RTC:
		{
			uint64_t rtc = strtoul(argument, NULL, 10);
			command->argument.uint32_argument = rtc;
			return true;
		}
		case COMMAND_ID:
		case COMMAND_SIGNATURE:
		case COMMAND_URL:
		case COMMAND_SSID:
		case COMMAND_PASS:
		case COMMAND_STATIC_IP:
		case COMMAND_STATIC_MASK:
		case COMMAND_STATIC_GATEWAY:
		case COMMAND_STATIC_DNS:
		case COMMAND_KNX_MULTICAST_ADDRESS:
		{
			strncpy(command->argument.string_argument, argument, COMMAND_ARGUMENT_MAX_LENGTH);
			return true;
		}
		case COMMAND_AUTH:
		{
			if(!strcmp_P(argument, PSTR("NONE")))
			{
				command->argument.uint32_argument = WIFI_SECURITY_UNSECURED;
				return true;
			}
			else if(!strcmp_P(argument, PSTR("WEP")))
			{
				command->argument.uint32_argument = WIFI_SECURITY_WEP;
				return true;
			}
			else if(!strcmp_P(argument, PSTR("WPA2")))
			{
				command->argument.uint32_argument = WIFI_SECURITY_WPA2;
				return true;
			}
			else
			{
				return false;
			}
		}
		case COMMAND_READINGS:
		case COMMAND_SYSTEM:
		{
			if(!strcmp_P(argument, PSTR("CLEAR")))
			{
				command->argument.bool_argument = true;
				return true;
			}
			else
			{
				return false;
			}
		}
		case COMMAND_ALARM:
		{
			return parse_alarm_argument(command, argument);
		}
		case COMMAND_SET:
		{
			return parse_set_argument(command, argument);
		}
		case COMMAND_KNX_PHYSICAL_ADDRESS:
		{
			return parse_knx_physical_address_argument(command, argument);
		}
		case COMMAND_KNX_GROUP_ADDRESS:
		{
			return parse_knx_group_address_argument(command, argument);
		}
		default:
		{
			memset(&command->argument, 0, sizeof(command_argument_t));
			return false;
		}
	}
}

static bool parse_command(char* command_string, uint16_t command_string_length, command_t* command)
{
	LOG_PRINT(1, PSTR("Processing command string %s of length %u\r\n"), command_string, command_string_length);
	
	if(command_string[command_string_length - 1] != COMMAND_TERMINATOR)
	{
		LOG(1, "Command does not end with command terminator");
		return false;
	}
	
	uint8_t number_of_commands = sizeof(command_table) / sizeof(command_table_t);
	uint8_t i;
	for (i = 1; i < number_of_commands; i++)
	{
		/* command name from command table */
		char command_token[COMMAND_NAME_MAX_LENGTH];
		strcpy_P(command_token, command_table[i].token);
		uint8_t command_token_length = strlen(command_token);

		if(strncmp(command_string, command_token, command_token_length))
		{
			continue;
		}

		/* command type */
		command->type = pgm_read_byte(&command_table[i].type);

		/* command without argument */
		if(command_string[command_token_length] == COMMAND_TERMINATOR)
		{
			command->has_argument = false;
			memset(&command->argument, 0, sizeof(command_argument_t));
			LOG(1, "Command parsed sucesfully");
			return true;
		}

		/* command with argument */
		if(command_string[command_token_length] == COMMAND_ARGUMENT_SEPARATOR)
		{
			/* extract argument */
			char* argument_start_position = command_string + command_token_length + 1;
			command_string[command_string_length - 1] = '\0';

			LOG_PRINT(2, PSTR("Argument %s of length %u\r\n"), argument_start_position, strlen(argument_start_position));

			if(parse_commad_argument(command, argument_start_position))
			{
				command->has_argument = true;
				LOG(1, "Command parsed sucesfully");
				return true;
			}
			else
			{
				LOG(1, "Command argument is invalid");
			}
		}
	}
	
	LOG(1, "Command could not be parsed or is not reckognized");
	return false;
}

bool extract_command_from_string_buffer(circular_buffer_t* command_string_buffer, command_t* command)
{
	uint16_t command_string_length = 0;
	char command_character = '\0';
	while(circular_buffer_peek(command_string_buffer, command_string_length++, &command_character) && (command_character != COMMAND_TERMINATOR));
	
	if(command_character != COMMAND_TERMINATOR)
	{
		//circular_buffer_clear(command_string_buffer);
		return false;
	}

	char command_string[COMMAND_MAX_LENGTH];
	circular_buffer_pop_array(command_string_buffer, command_string_length, command_string);
	
	if(!parse_command(command_string, command_string_length, command))
	{
		command->type = COMMAND_BAD;
	}

	return true;
}

uint8_t extract_commands_from_string_buffer(circular_buffer_t* command_string_buffer, circular_buffer_t* command_buffer)
{
	command_t command;
	uint8_t commands_extracted = 0;
	while(extract_command_from_string_buffer(command_string_buffer, &command))
	{
		circular_buffer_add(command_buffer, &command);
		commands_extracted++;
	}
	
	return commands_extracted;
}
