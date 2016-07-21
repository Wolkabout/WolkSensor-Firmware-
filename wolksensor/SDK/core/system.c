#include "system.h"
#include "logger.h"

void append_knx_communication_protocol_data(knx_communication_protocol_data_t* operation_data, knx_communication_protocol_data_t* total_data)
{
	if(total_data->error == 0)
	{
		total_data->error = operation_data->error;
	}
}

void append_mqtt_communication_protocol_data(mqtt_communication_protocol_data_t* operation_data, mqtt_communication_protocol_data_t* total_data)
{
	if(total_data->error == 0)
	{
		total_data->error = operation_data->error;
	}
}

void append_communication_protocol_type_data(communication_protocol_type_data_t* operation_data, communication_protocol_type_data_t* total_data)
{
	total_data->type = operation_data->type;

	switch(total_data->type)
	{
		case COMMUNICATION_PROTOCOL_MQTT:
		{
			append_mqtt_communication_protocol_data(&operation_data->data.mqtt_communication_protocol_data, &total_data->data.mqtt_communication_protocol_data);
			break;
		}
		case COMMUNICATION_PROTOCOL_KNX:
		{
			append_knx_communication_protocol_data(&operation_data->data.knx_communication_protocol_data, &total_data->data.knx_communication_protocol_data);
			break;
		}
		default:
		{
			break;
		}
	}
	
	append_communication_module_type_data(&operation_data->communication_module_type_data, &total_data->communication_module_type_data);
}

bool is_mqtt_communication_protocol_success(mqtt_communication_protocol_data_t* mqtt_communication_protocol_data)
{
	return mqtt_communication_protocol_data->error == MQTT_SUCCESS;
}

bool is_knx_communication_protocol_success(knx_communication_protocol_data_t* knx_communication_protocol_data)
{
	return knx_communication_protocol_data->error == KNX_SUCCESS;
}

bool is_communication_protocol_success(communication_protocol_type_data_t* communication_protocol_type_data)
{
	bool communication_protocol_success = false;
	
	switch(communication_protocol_type_data->type)
	{
		case COMMUNICATION_PROTOCOL_MQTT:
		{
			communication_protocol_success = is_mqtt_communication_protocol_success(&communication_protocol_type_data->data.mqtt_communication_protocol_data);
			break;
		}
		case COMMUNICATION_PROTOCOL_KNX:
		{
			communication_protocol_success = is_knx_communication_protocol_success(&communication_protocol_type_data->data.knx_communication_protocol_data);
			break;
		}
		default:
		{
			communication_protocol_success = false;
			break;
		}
	}
	
	return communication_protocol_success && is_communication_module_success(&communication_protocol_type_data->communication_module_type_data);
}

void append_tcp_communication_module_data(tcp_communication_module_data_t* operation_data, tcp_communication_module_data_t* total_data)
{
	if(total_data->error == 0)
	{
		total_data->error = operation_data->error;
		total_data->platform_specific_error_code = operation_data->platform_specific_error_code;
	}
}

void append_udp_communication_module_data(udp_communication_module_data_t* operation_data, udp_communication_module_data_t* total_data)
{
	if(total_data->error == 0)
	{
		total_data->error = operation_data->error;
		total_data->platform_specific_error_code = operation_data->platform_specific_error_code;
	}
}

void append_wifi_communication_module_data(wifi_communication_module_data_t* operation_data, wifi_communication_module_data_t* total_data)
{
	if(total_data->error == 0)
	{
		total_data->error = operation_data->error;
		total_data->platform_specific_error_code = operation_data->platform_specific_error_code;
	}

	// copy communication data
	total_data->connect_to_ap_time += operation_data->connect_to_ap_time;
	total_data->acquire_ip_address_time += operation_data->acquire_ip_address_time;
	total_data->connect_to_server_time += operation_data->connect_to_server_time;
	total_data->data_exchange_time += operation_data->data_exchange_time;
	total_data->disconnect_time += operation_data->disconnect_time;
}

void append_communication_module_type_data(communication_module_type_data_t* operation_data, communication_module_type_data_t* total_data)
{
	total_data->type = operation_data->type;

	switch(total_data->type)
	{
		case COMMUNICATION_MODULE_WIFI:
		{
			append_wifi_communication_module_data(&operation_data->data.wifi_communication_module_data, &total_data->data.wifi_communication_module_data);
			break;
		}
		case COMMUNICATION_MODULE_TCP:
		{
			append_tcp_communication_module_data(&operation_data->data.tcp_communication_module_data, &total_data->data.tcp_communication_module_data);
			break;
		}
		case COMMUNICATION_MODULE_UDP:
		{
			append_udp_communication_module_data(&operation_data->data.tcp_communication_module_data, &total_data->data.tcp_communication_module_data);
			break;
		}
		default:
		{
			break;
		}
	}
}

bool is_wifi_communication_module_success(wifi_communication_module_data_t* wifi_communication_module_data)
{
	return wifi_communication_module_data->error == 0;
}

bool is_tcp_communication_module_success(tcp_communication_module_data_t* tcp_communication_module_data)
{
	return tcp_communication_module_data->error == 0;
}

bool is_udp_communication_module_success(udp_communication_module_data_t* udp_communication_module_data)
{
	return udp_communication_module_data->error == 0;
}

bool is_communication_module_success(communication_module_type_data_t* communication_module_type_data)
{
	switch(communication_module_type_data->type)
	{
		case COMMUNICATION_MODULE_WIFI:
		{
			return is_wifi_communication_module_success(&communication_module_type_data->data.wifi_communication_module_data);
		}
		case COMMUNICATION_MODULE_TCP:
		{
			return is_tcp_communication_module_success(&communication_module_type_data->data.tcp_communication_module_data);
		}
		case COMMUNICATION_MODULE_UDP:
		{
			return is_udp_communication_module_success(&communication_module_type_data->data.udp_communication_module_data);
		}
		default:
		{
			return false;
		}
	}
}

void append_communication_and_battery_data(communication_and_battery_data_t* operation_data, communication_and_battery_data_t* total_data)
{
	append_communication_protocol_type_data(&operation_data->communication_protocol_type_data, &total_data->communication_protocol_type_data);
	
	if(total_data->battery_min_voltage == 0 || total_data->battery_min_voltage > operation_data->battery_min_voltage)
	{
		total_data->battery_min_voltage = operation_data->battery_min_voltage;
	}
}
