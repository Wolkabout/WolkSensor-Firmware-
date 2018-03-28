#include "platform_specific.h"
#include "sensor_readings_buffer.h"
#include "system_buffer.h"
#include "wifi_communication_module.h"
#include "mqtt_communication_protocol.h"
#include "wifi_communication_module_dependencies.h"
#include "tcp_communication_module_dependencies.h"
#include "udp_communication_module_dependencies.h"
#include "config.h"
#include "chrono.h"
#include "logger.h"
#include "system.h"
#include "actuators.h"
#include "commands_dependencies.h"
#include "util_conversions.h"

static char tmp[128];

bool append_bad_request(circular_buffer_t* message_buffer)
{
	sprintf_P(tmp, PSTR("BAD_REQUEST;"));
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	
	return true;
}

bool append_done(circular_buffer_t* message_buffer)
{
	sprintf_P(tmp, PSTR("DONE;"));
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	
	return true;
}

bool append_busy(circular_buffer_t* message_buffer)
{
	sprintf_P(tmp, PSTR("BUSY;"));
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	
	return true;
}

static uint16_t serialize_sensor_reading(sensor_readings_t* sensor_reading, char* buffer)
{
	int16_t size = 0;

	size = sprintf_P(buffer, PSTR("R:%lu,"), sensor_reading->timestamp);
	
	int i;
	for(i = 0; i < NUMBER_OF_SENSORS; i++)
	{
		if(sensor_reading->values[i] != SENSOR_VALUE_NOT_SET)
		{
			size += sprintf_P(buffer + size, PSTR("%c:%.2f,"), sensors[i].id, sensor_reading->values[i]);
		}
	}
	
	sprintf_P(buffer + size - 1, PSTR("|"));

	return size;
}

uint16_t serialize_sensor_readings(circular_buffer_t* sensor_readings_buffer, uint16_t start_position, circular_buffer_t* message_buffer)
{
	LOG_PRINT(2, PSTR("Serializing sensor readings from %u, size before %u\r\n"), start_position, circular_buffer_size(sensor_readings_buffer));
	
	uint16_t serialized_readings = 0;

	sensor_readings_t sensor_reading;

	bool serializing = circular_buffer_peek(sensor_readings_buffer, start_position + serialized_readings, &sensor_reading);
	while(serializing)
	{
		uint16_t size = serialize_sensor_reading(&sensor_reading, tmp);
		if(circular_buffer_add_array(message_buffer, tmp, size))
		{
			serialized_readings++;
			serializing = circular_buffer_peek(sensor_readings_buffer, start_position + serialized_readings, &sensor_reading);
		}
		else
		{
			serializing = false;
		}
	}
	
	LOG_PRINT(1, PSTR("Serialized %u sensor readings, buffer size %u\r\n"), serialized_readings, circular_buffer_size(sensor_readings_buffer));

	return serialized_readings;
}

uint16_t append_sensor_readings(circular_buffer_t* sensor_readings_buffer, uint16_t start_position, circular_buffer_t* message_buffer, bool split)
{
	if(start_position == 0)
	{
		uint16_t header_length = sprintf_P(tmp, PSTR("READINGS "));
		if(!circular_buffer_add_array(message_buffer, tmp, header_length))
		{
			return 0;
		}
	}
	
	uint16_t serialized_readings = serialize_sensor_readings(sensor_readings_buffer, start_position, message_buffer);
	
	if(!split || (start_position + serialized_readings) == circular_buffer_size(sensor_readings_buffer))
	{
		circular_buffer_drop_from_end(message_buffer, 1); /* remove last | */
		circular_buffer_add(message_buffer, ";");
	}
	
	return serialized_readings;
}

static uint16_t serialize_system_error(system_error_t* system_error, char* buffer)
{
	uint16_t size = 0;
	size = sprintf_P(buffer + size, PSTR("%01X"), system_error->type);
	
	switch(system_error->type)
	{
		case SYSTEM_RESET:
		{
			size += sprintf_P(buffer + size, PSTR("%02X"), system_error->data.system_reset_reason);
			break;
		}
		case SYSTEM_BROWNOUT:
		default:
		{
			break;
		}
	}
	
	return size;
}

static uint16_t serialize_wifi_communication_module_error(wifi_communication_module_data_t* wifi_communication_module_data, char* buffer)
{
	if(wifi_communication_module_data->error != 0)
	{
		uint16_t size = sprintf_P(buffer, PSTR("%01X%02X%"), COMMUNICATION_MODULE_WIFI, wifi_communication_module_data->error);
		size += wifi_communication_module_dependencies.serialize_wifi_platform_specific_error_code(wifi_communication_module_data->platform_specific_error_code, buffer + size);
		
		return size;
	}
	
	return 0;
}

static uint16_t serialize_udp_communication_module_error(udp_communication_module_data_t* udp_communication_module_data, char* buffer)
{
	if(udp_communication_module_data->error != 0)
	{
		uint16_t size = sprintf_P(buffer, PSTR("%01X%02X"), COMMUNICATION_MODULE_UDP, udp_communication_module_data->error);
		size += udp_communication_module_dependencies.serialize_platform_specific_error_code(udp_communication_module_data->platform_specific_error_code, buffer + size);
		
		return size;
	}

	return 0;
}

static uint16_t serialize_tcp_communication_module_error(tcp_communication_module_data_t* tcp_communication_module_data, char* buffer)
{
	if(tcp_communication_module_data->error != 0)
	{
		uint16_t size = sprintf_P(buffer, PSTR("%01X%02X"), COMMUNICATION_MODULE_TCP, tcp_communication_module_data->error);
		size += tcp_communication_module_dependencies.serialize_platform_specific_error_code(tcp_communication_module_data->platform_specific_error_code, buffer + size);
		
		return size;
	}

	return 0;
}

static uint16_t serialize_wifi_communication_module_data(wifi_communication_module_data_t* wifi_communication_module_data, char* buffer)
{
	uint16_t size = serialize_wifi_communication_module_error(wifi_communication_module_data, buffer);
	
	uint16_t total_online_time = 0;
	
	if(wifi_communication_module_data->connect_to_ap_time)
	{
		total_online_time += wifi_communication_module_data->connect_to_ap_time;
		size += sprintf_P(buffer + size, PSTR(",A:%u"), wifi_communication_module_data->connect_to_ap_time);
	}
	
	if(wifi_communication_module_data->acquire_ip_address_time)
	{
		total_online_time += wifi_communication_module_data->acquire_ip_address_time;
		size += sprintf_P(buffer + size, PSTR(",D:%u"), wifi_communication_module_data->acquire_ip_address_time);
	}
	
	if(wifi_communication_module_data->connect_to_server_time)
	{
		total_online_time += wifi_communication_module_data->connect_to_server_time;
		size += sprintf_P(buffer + size, PSTR(",S:%u"), wifi_communication_module_data->connect_to_server_time);
	}
	
	if(wifi_communication_module_data->data_exchange_time)
	{
		total_online_time += wifi_communication_module_data->data_exchange_time;
		size += sprintf_P(buffer + size, PSTR(",Q:%u"), wifi_communication_module_data->data_exchange_time);
	}
	
	if(wifi_communication_module_data->disconnect_time)
	{
		total_online_time += wifi_communication_module_data->disconnect_time;
		size += sprintf_P(buffer + size, PSTR(",L:%u"), wifi_communication_module_data->disconnect_time);
	}
	
	if(total_online_time)
	{
		size += sprintf_P(buffer + size, PSTR(",C:%u"), total_online_time);
	}

	return size;
}

uint16_t serialize_communication_module_error(communication_module_type_data_t* communication_module_type_data, char* buffer)
{
	switch(communication_module_type_data->type)
	{
		case COMMUNICATION_MODULE_WIFI:
		{
			return serialize_wifi_communication_module_error(&communication_module_type_data->data.wifi_communication_module_data, buffer);
		}
		case COMMUNICATION_MODULE_TCP:
		{
			return serialize_tcp_communication_module_error(&communication_module_type_data->data.tcp_communication_module_data, buffer);
		}
		default:
		{
			return 0;
		}
	}
}

uint16_t serialize_communication_module_data(communication_module_type_data_t* communication_module_type_data, char* buffer)
{
	uint16_t size = 0;
	
	switch(communication_module_type_data->type)
	{
		case COMMUNICATION_MODULE_WIFI:
		{
			size += serialize_wifi_communication_module_data(&communication_module_type_data->data.wifi_communication_module_data, buffer);
			break;
		}
		case COMMUNICATION_MODULE_TCP:
		{
			size += serialize_tcp_communication_module_error(&communication_module_type_data->data.tcp_communication_module_data, buffer);
			break;
		}
		case COMMUNICATION_MODULE_UDP:
		{
			size += serialize_udp_communication_module_error(&communication_module_type_data->data.udp_communication_module_data, buffer);
			break;
		}
		default:
		{
			break;
		}
	}
	
	return size;
}

static uint16_t serialize_mqtt_communication_protocol_error(mqtt_communication_protocol_data_t* mqtt_communication_protocol_data, char* buffer)
{
	if(mqtt_communication_protocol_data->error != 0)
	{
		return sprintf_P(buffer, PSTR("%01X%02X"), COMMUNICATION_PROTOCOL_MQTT, mqtt_communication_protocol_data->error);
	}

	return 0;
}

static uint16_t serialize_mqtt_communication_protocol_data(mqtt_communication_protocol_data_t* mqtt_communication_protocol_data, char* buffer)
{
	return serialize_mqtt_communication_protocol_error(mqtt_communication_protocol_data, buffer);
}

static uint16_t serialize_communication_protocol_data(communication_protocol_type_data_t* communication_protocol_type_data, char* buffer)
{
	uint16_t size = 0;
	switch(communication_protocol_type_data->type)
	{
		case COMMUNICATION_PROTOCOL_MQTT:
		{
			size += serialize_mqtt_communication_protocol_data(&communication_protocol_type_data->data.mqtt_communication_protocol_data, buffer);
			break;
		}
		default:
		{
			break;
		}
	}

	size += serialize_communication_module_data(&communication_protocol_type_data->communication_module_type_data, buffer + size);
	
	return size;
}

uint16_t serialize_communication_protocol_error(communication_protocol_type_data_t* communication_protocol_type_data, char* buffer)
{
	uint16_t size = sprintf_P(buffer, PSTR("%01X"), COMMUNICATION_PROTOCOL_DATA);

	switch(communication_protocol_type_data->type)
	{
		case COMMUNICATION_PROTOCOL_MQTT:
		{
			size += serialize_mqtt_communication_protocol_error(&communication_protocol_type_data->data.mqtt_communication_protocol_data, buffer + size);
			break;
		}
		default:
		{
			break;
		}
	}
	
	size += serialize_communication_module_error(&communication_protocol_type_data->communication_module_type_data, buffer + size);
	
	return size;
}

static uint16_t serialize_communication_and_battery_data(communication_and_battery_data_t* communication_and_battery_data, char* buffer)
{
	uint16_t size = serialize_communication_protocol_data(&communication_and_battery_data->communication_protocol_type_data, buffer);
	
	size += sprintf_P(buffer + size, PSTR(",B:%u"), communication_and_battery_data->battery_min_voltage);

	return size += sprintf_P(buffer + size, PSTR(",V:%d.%d.%d"), FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_PATCH);
}

static uint16_t serialize_system_item(system_t* system_item, char* buffer)
{
	uint16_t size = sprintf_P(buffer, PSTR("R:%lu"), system_item->timestamp);
	
	switch(system_item->type)
	{
		case SYSTEM_ERROR:
		{
			size += sprintf_P(buffer + size, PSTR(",E:%01X"), SYSTEM_ERROR);
			size += serialize_system_error(&system_item->data.system_error, buffer + size);
			break;
		}
		case COMMUNICATION_AND_BATTERY_DATA:
		{
			if(!is_communication_protocol_success(&system_item->data.communication_and_battery_data.communication_protocol_type_data))
			{
				size += sprintf_P(buffer + size, PSTR(",E:%01X"), COMMUNICATION_PROTOCOL_DATA);
			}
			size += serialize_communication_and_battery_data(&system_item->data.communication_and_battery_data, buffer + size);
			break;
		}
		case COMMUNICATION_PROTOCOL_DATA:
		{
			if(!is_communication_protocol_success(&system_item->data.communication_protocol_type_data))
			{
				size += sprintf_P(buffer + size, PSTR(",E:%01X"), COMMUNICATION_PROTOCOL_DATA);
			}
			size += serialize_communication_protocol_data(&system_item->data.communication_protocol_type_data, buffer + size);
			break;
		}
		default:
		{
			break;
		}
	}
	
	return size;
}

uint16_t serialize_system_data(circular_buffer_t* system_buffer, uint16_t start, circular_buffer_t* message_buffer)
{
	LOG_PRINT(2, PSTR("Serializing system items from %u, size before %u\r\n"), start, circular_buffer_size(system_buffer));
	
	uint16_t serialized_system_items = 0;
	
	system_t system_item;

	bool serializing = circular_buffer_peek(system_buffer, start + serialized_system_items, &system_item);
	while(serializing)
	{
		uint16_t size = serialize_system_item(&system_item, tmp);

		if(circular_buffer_add_array(message_buffer, tmp, size))
		{
			serialized_system_items++;
			serializing = circular_buffer_peek(system_buffer, start + serialized_system_items, &system_item);
			
			char separator = '|';
			circular_buffer_add(message_buffer, &separator);
		}
		else
		{
			serializing = false;
		}
	}
	
	LOG_PRINT(1, PSTR("Serialized system items %u, buffer size %u\r\n"), serialized_system_items, circular_buffer_size(system_buffer));
	
	return serialized_system_items;
}

uint16_t append_system_info(circular_buffer_t* system_info_buffer, uint16_t start, circular_buffer_t* message_buffer, bool split)
{
	if(start == 0)
	{
		uint16_t size = sprintf_P(tmp, PSTR("SYSTEM "));
		if(!circular_buffer_add_array(message_buffer, tmp, size))
		{
			return 0;
		}
	}
	
	uint16_t serialized_system_items = serialize_system_data(system_info_buffer, start, message_buffer);
	
	if(!split || (start + serialized_system_items) == circular_buffer_size(system_info_buffer))	
	{
		circular_buffer_drop_from_end(message_buffer, 1); /* remove last | */
		circular_buffer_add(message_buffer, ";");
	}
	
	return serialized_system_items;
}

bool append_mac_address(unsigned char* mac, circular_buffer_t* message_buffer)
{
	unsigned char MAC[12+1];
	uint8_t i;
	for (i = 0; i < 6; i++)
	{
		sprintf_P(MAC+2*i, PSTR("%02X"), mac[i]);
	}

	sprintf_P(tmp, PSTR("MAC %s;"), MAC);
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	
	return true;
}

bool append_heartbeat(uint16_t heartbeat, circular_buffer_t* message_buffer)
{
	sprintf_P(tmp, PSTR("HEARTBEAT %u;"), system_heartbeat);
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_rtc(uint32_t rtc, circular_buffer_t* message_buffer)
{	
	sprintf_P(tmp, PSTR("RTC %lu;"), rtc);
	
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	
	return true;
}

bool append_version(const char* version, circular_buffer_t* message_buffer)
{
	sprintf_P(tmp, PSTR("VERSION %s;"), version);
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_status(char* status, circular_buffer_t* message_buffer)
{	
	sprintf_P(tmp, PSTR("STATUS %s;"), status);
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_id(char* id, circular_buffer_t* message_buffer)
{
	sprintf_P(tmp, PSTR("ID %s;"), device_id);
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_signature(char* signature, circular_buffer_t* message_buffer)
{
	sprintf_P(tmp, PSTR("SIGNATURE %s;"), (*device_preshared_key) ? "****" : "");
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_url(char* url, circular_buffer_t* message_buffer)
{
	sprintf_P(tmp, PSTR("URL %s;"), hostname);
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_port(uint16_t port, circular_buffer_t* message_buffer)
{
	sprintf_P(tmp, PSTR("PORT %u;"), server_port);
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_ssid(char* ssid, circular_buffer_t* message_buffer)
{
	sprintf_P(tmp, PSTR("SSID %s;"), wifi_ssid);
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_pass(char* pass, circular_buffer_t* message_buffer)
{
	sprintf_P(tmp, PSTR("PASS %s;"), wifi_password);

	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_auth(uint8_t auth, circular_buffer_t* message_buffer)
{
	if (wifi_auth_type == WIFI_SECURITY_UNSECURED)
	{
		sprintf_P(tmp, PSTR("AUTH %s;"), "NONE");
	}
	else if (wifi_auth_type == WIFI_SECURITY_WEP)
	{
		sprintf_P(tmp, PSTR("AUTH %s;"), "WEP");
	}
	else if (wifi_auth_type == WIFI_SECURITY_WPA)
	{
		sprintf_P(tmp, PSTR("AUTH %s;"), "WPA");
	}
	else if (wifi_auth_type == WIFI_SECURITY_WPA2)
	{
		sprintf_P(tmp, PSTR("AUTH %s;"), "WPA2");
	}

	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_movement_enabled(bool enabled, circular_buffer_t* message_buffer)
{
	if (movement_status)
	{
		sprintf_P(tmp, PSTR("MOVEMENT ON;"));
	}
	else
	{
		sprintf_P(tmp, PSTR("MOVEMENT OFF;"));
	}
	
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_atmo_enabled(bool enabled, circular_buffer_t* message_buffer)
{
	if (atmo_status)
	{
		sprintf_P(tmp, PSTR("ATMO ON;"));
	}
	else
	{
		sprintf_P(tmp, PSTR("ATMO OFF;"));
	}
	
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_static_ip(char* ip, circular_buffer_t* message_buffer)
{
	if (strcmp_P(wifi_static_ip, PSTR("")) == 0)
	{
		sprintf_P(tmp, PSTR("STATIC_IP OFF;"));
	}
	else
	{
		sprintf_P(tmp, PSTR("STATIC_IP %s;"), wifi_static_ip);
	}
	
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_static_mask(char* mask, circular_buffer_t* message_buffer)
{
	sprintf_P(tmp, PSTR("STATIC_MASK %s;"), wifi_static_mask);
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_static_gateway(char* gateway, circular_buffer_t* message_buffer)
{
	sprintf_P(tmp, PSTR("STATIC_GATEWAY %s;"), wifi_static_gateway);
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_static_dns(char* dns, circular_buffer_t* message_buffer)
{
	sprintf_P(tmp, PSTR("STATIC_DNS %s;"), wifi_static_dns);
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_alarms(sensor_alarms_t* alarms, uint16_t length, circular_buffer_t* buffer)
{
	uint8_t size = sprintf_P(tmp, PSTR("ALARM "));
	if(!circular_buffer_add_array(buffer, tmp, size))
	{
		return false;
	}
	
	uint8_t i;
	for(i = 0; i < NUMBER_OF_SENSORS; i++)
	{
		size = sprintf_P(tmp, PSTR("%c:"), sensors[i].id);
		if(sensors_alarms[i].alarm_low.enabled)
		{
			size += sprintf_P(tmp + size, PSTR("%d,"), sensors_alarms[i].alarm_low.value);
		}
		else
		{
			size += sprintf_P(tmp + size, PSTR("OFF,"));
		}
		
		if(sensors_alarms[i].alarm_high.enabled)
		{
			size += sprintf_P(tmp + size, PSTR("%d|"), sensors_alarms[i].alarm_high.value);
		}
		else
		{
			size += sprintf_P(tmp + size, PSTR("OFF|"));
		}
		
		if(!circular_buffer_add_array(buffer, tmp, size))
		{
			return false;
		}
	}
	
	circular_buffer_drop_from_end(buffer, 1); /* remove last | */
	circular_buffer_add(buffer, ";");
	
	return true;
}

bool append_actuator_state(actuator_t* actuator, actuator_state_t* actuator_state, circular_buffer_t* buffer)
{
	uint16_t size = sprintf_P(tmp, PSTR("STATUS "));
	circular_buffer_add_array(buffer, tmp, size);
	
	size = sprintf_P(tmp, PSTR("%s:"), actuator->id);
	
	
	switch(actuator->type)
	{
		case ACTUATOR_TYPE_SWITCH:
		{
			size += sprintf_P(tmp + size, PSTR("SW,%s"), actuator_state->value.switch_value ? "ON" : "OFF");
			break;
		}
		case ACTUATOR_TYPE_DC_MOTOR:
		{
			size += sprintf_P(tmp + size, PSTR("DCM,%d"), actuator_state->value.dc_motor_value);
			break;
		}
		case ACTUATOR_TYPE_SERVO:
		{
			size += sprintf_P(tmp + size, PSTR("SRV,%d"), actuator_state->value.servo_value);
			break;
		}
	}
	
	switch(actuator_state->status)
	{
		case ACTUATOR_STATUS_OK:
		{
			size += sprintf_P(tmp + size, PSTR(",OK|"));
			break;
		}
		case ACTUATOR_STATUS_IN_PROGRESS:
		{
			size += sprintf_P(tmp + size, PSTR(",IN_PROGRESS|"));
			break;
		}
		case ACTUATOR_STATUS_FAILED:
		{
			size += sprintf_P(tmp + size, PSTR(",FAILED|"));
			break;
		}
	}
	
	circular_buffer_add_array(buffer, tmp, size);
	
	circular_buffer_drop_from_end(buffer, 1); /* remove last | */
	circular_buffer_add(buffer, ";");
	
	return true;
}

bool append_detected_wifi_networks(wifi_network_t* networks, uint8_t networks_size, circular_buffer_t* response_buffer)
{
	uint16_t length = sprintf_P(tmp, PSTR("READINGS "));
	circular_buffer_add_array(response_buffer, tmp, length);
	
	uint8_t i;
	for(i = 0; i < networks_size; i++)
	{
		length = sprintf_P(tmp, PSTR("MAC:%02X%02X%02X%02X%02X%02X,RSSI:%d%c"), networks[i].bssid[0], networks[i].bssid[1], networks[i].bssid[2], networks[i].bssid[3], networks[i].bssid[4], networks[i].bssid[5], networks[i].rssi, (i == networks_size - 1) ? ';' : '|');
		//length = sprintf_P(tmp, PSTR("I:%s,M:%02X%02X%02X%02X%02X%02X,S:%d%c"), networks[i].ssid, networks[i].bssid[0], networks[i].bssid[1], networks[i].bssid[2], networks[i].bssid[3], networks[i].bssid[4], networks[i].bssid[5], networks[i].rssi, (i == networks_size - 1) ? ';' : '|');
		circular_buffer_add_array(response_buffer, tmp, length);	
	}
	
	return true;
}

bool append_location_status(bool location_status, circular_buffer_t* response_buffer)
{
	if (location_status)
	{
		sprintf_P(tmp, PSTR("LOCATION ON;"));
	}
	else
	{
		sprintf_P(tmp, PSTR("LOCATION OFF;"));
	}
	
	circular_buffer_add_array(response_buffer, tmp, strlen(tmp));
	return true;
}

bool append_ssl_status(bool ssl_status, circular_buffer_t* response_buffer)
{
	if (ssl_status)
	{
		sprintf_P(tmp, PSTR("SSL ON;"));
	}
	else
	{
		sprintf_P(tmp, PSTR("SSL OFF;"));
	}
	
	circular_buffer_add_array(response_buffer, tmp, strlen(tmp));
	return true;
}

bool append_mqtt_username(char* id, circular_buffer_t* response_buffer)
{
	sprintf_P(tmp, PSTR("MQTT_USERNAME %s;"), id);
	circular_buffer_add_array(response_buffer, tmp, strlen(tmp));
	return true;
}

bool append_mqtt_password(char* password, circular_buffer_t* response_buffer)
{
	sprintf_P(tmp, PSTR("MQTT_PASSWORD %s;"), password);
	circular_buffer_add_array(response_buffer, tmp, strlen(tmp));
	return true;
}

bool append_temp_offset(uint16_t offset, circular_buffer_t* message_buffer)
{
	sprintf_P(tmp, PSTR("TEMP_OFFSET %g;"), atmo_offset[0]);
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_humidity_offset(uint16_t offset, circular_buffer_t* message_buffer)
{
	sprintf_P(tmp, PSTR("HUMIDITY_OFFSET %g;"), atmo_offset[2]);
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_pressure_offset(uint16_t offset, circular_buffer_t* message_buffer)
{
	sprintf_P(tmp, PSTR("PRESSURE_OFFSET %g;"), atmo_offset[1]);
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}

bool append_offset_factory(char* offset_factory, circular_buffer_t* message_buffer)
{
	if(strcmp_P(offset_factory, PSTR("RESET")) == 0)
	{
		sprintf_P(tmp, PSTR("OFFSET_FACTORY %s;"), "RESET");
	}
	else
	{
		static char array[6];

		sprintf_P(tmp, PSTR("OFFSET_FACTORY P:%g,"), atmo_offset_factory[1]);
		sprintf_P(array, PSTR("T:%g,"), atmo_offset_factory[0]);
		strcat(tmp, array);
		sprintf_P(array, PSTR("H:%g;"), atmo_offset_factory[2]);
		strcat(tmp, array);

		LOG_PRINT(1, PSTR("tmp: %s \n\r"), tmp);
	}
	circular_buffer_add_array(message_buffer, tmp, strlen(tmp));
	return true;
}
