#include "logger.h"
#include "knx.h"
#include "config.h"
#include "event_buffer.h"
#include "state_machine.h"
#include "communication_module.h"
#include "wifi_communication_module.h"
#include "wifi_communication_module_dependencies.h"

#define EVENTS_BUFFER_SIZE 10
#define KNX_BUFFER_SIZE 256

typedef enum
{
	KNX_ROUTING_MESSAGE = 0,
	KNX_TUNNELING_CONNECT_RESPONSE,
	KNX_TUNNELING_ACK,
	KNX_TUNNELING_VALUE_CONFIRMATION,
	KNX_TUNNELING_DISCONNECT_RESPONSE
}
knx_message_type_t;

typedef struct
{
	uint8_t channel;
}
knx_tunneling_connect_response_t;

typedef union
{
	knx_tunneling_connect_response_t knx_tunneling_connect_response;
}
knx_message_data_t;

typedef struct
{
	knx_message_type_t type;
	knx_message_data_t data;
}
knx_message_t;

typedef enum
{
	STATE_KNX_ROUTING = 0,
		STATE_KNX_ROUTING_SEND,
	STATE_KNX_CONNECTING,
		STATE_KNX_CONNECT_TO_NETWORK,
		STATE_KNX_SEND_CONNECT_REQUEST,
		STATE_KNX_RECEIVE_CONNECT_RESPONSE,
	STATE_KNX_TUNNELING,
		STATE_KNX_TUNELING_SEND,
		STATE_KNX_TUNELING_RECEIVE_ACK,
	STATE_KNX_DISCONNECTING,
		STATE_KNX_SEND_DISCONNECT_REQUEST,
		STATE_KNX_RECEIVE_DISCONNECT_RESPONSE
}
knx_states_t;

typedef enum
{
	EVENT_SEND = 0,
	EVENT_RECEIVE_ACK,
	EVENT_DISCONNECT,
	EVENT_COMMUNICATION_MODULE_PROCESS,
	EVENT_COMMUNICATION_MODULE_DONE
}
knx_events_t;

static state_machine_state_t state_machine;
static state_machine_state_t states[12];

static circular_buffer_t events_buffer;
static event_t events_buffer_storage[EVENTS_BUFFER_SIZE];

static circular_buffer_t* sending_sensor_readings_buffer = NULL;
static uint16_t* sensor_readings_sent = NULL;

static communication_protocol_type_data_t communication_protocol_type_data;

static circular_buffer_t knx_buffer;
static uint8_t knx_buffer_storage[KNX_BUFFER_SIZE];

static communication_module_process_handle_t communication_module_process_handle = NULL;

static uint8_t channel = 0; 
static uint8_t sequence_counter = 0;

static bool knx_handler(state_machine_state_t* state, event_t* event);

static bool state_routing(state_machine_state_t* state, event_t* event);
	static bool state_routing_send(state_machine_state_t* state, event_t* event);
static bool state_connecting(state_machine_state_t* state, event_t* event);
	static bool state_connect_to_network(state_machine_state_t* state, event_t* event);
	static bool state_send_connect_request(state_machine_state_t* state, event_t* event);
	static bool state_receive_connect_response(state_machine_state_t* state, event_t* event);
static bool state_tunneling(state_machine_state_t* state, event_t* event);
	static bool state_tunneling_send(state_machine_state_t* state, event_t* event);
	static bool state_tunneling_receive_ack(state_machine_state_t* state, event_t* event);
static bool state_disconnecting(state_machine_state_t* state, event_t* event);
	static bool state_send_disconnect_request(state_machine_state_t* state, event_t* event);
	static bool state_receive_disconnect_response(state_machine_state_t* state, event_t* event);

static void init_state(knx_states_t id, const char* human_readable_name, state_machine_state_t* parent, int8_t initial_state, state_machine_state_handler handler)
{
	state_machine_init_state(id, human_readable_name, parent, states, initial_state, handler);
}

static void transition(knx_states_t new_state_id)
{
	state_machine_transition(states, &state_machine, new_state_id);
}

static void init_events_buffer(void)
{
	circular_buffer_init(&events_buffer, events_buffer_storage, EVENTS_BUFFER_SIZE, sizeof(event_t), true, true);
}

static void init_knx_buffer(void)
{
	circular_buffer_init(&knx_buffer, knx_buffer_storage, KNX_BUFFER_SIZE, sizeof(uint8_t), true, true);
}

static bool knx_process(void)
{
	event_t event;
	if(pop_event(&events_buffer, &event))
	{
		state_machine_process_event(states, &state_machine, &event);
		return true;
	}
	
	return false;
}

static void clear_communication_protocol_data(void)
{
	memset(&communication_protocol_type_data, 0, sizeof(communication_protocol_type_data_t));
	communication_protocol_type_data.type = COMMUNICATION_PROTOCOL_KNX;
}
	
void knx_init(void)
{
	init_events_buffer();
	init_knx_buffer();
	
	load_knx_physical_address();
	load_knx_group_address();
	
	load_knx_multicast_address();
	load_knx_multicast_port();
	
	/* init state machine */
	state_machine.id = -1;
	state_machine.human_readable_name = NULL;
	state_machine.parent = NULL;
	state_machine.current_state = -1;
	state_machine.handler = knx_handler;
	
	init_state(STATE_KNX_ROUTING, NULL, &state_machine, -1, state_routing);
		init_state(STATE_KNX_ROUTING_SEND, NULL, &states[STATE_KNX_ROUTING], -1, state_routing_send);
	init_state(STATE_KNX_CONNECTING, NULL, &state_machine, -1, state_connecting);
		init_state(STATE_KNX_CONNECT_TO_NETWORK, NULL, &states[STATE_KNX_CONNECTING], -1, state_connect_to_network);
		init_state(STATE_KNX_SEND_CONNECT_REQUEST, NULL, &states[STATE_KNX_CONNECTING], -1, state_send_connect_request);
		init_state(STATE_KNX_RECEIVE_CONNECT_RESPONSE, NULL, &states[STATE_KNX_CONNECTING], -1, state_receive_connect_response);
	init_state(STATE_KNX_TUNNELING, NULL, &state_machine, -1, state_tunneling);
		init_state(STATE_KNX_TUNELING_SEND, NULL, &states[STATE_KNX_TUNNELING], -1, state_tunneling_send);
		init_state(STATE_KNX_TUNELING_RECEIVE_ACK, NULL, &states[STATE_KNX_TUNNELING], -1, state_tunneling_receive_ack);
	init_state(STATE_KNX_DISCONNECTING, NULL, &state_machine, -1, state_disconnecting);
		init_state(STATE_KNX_SEND_DISCONNECT_REQUEST, NULL, &states[STATE_KNX_DISCONNECTING], -1, state_send_disconnect_request);
		init_state(STATE_KNX_RECEIVE_DISCONNECT_RESPONSE, NULL, &states[STATE_KNX_DISCONNECTING], -1, state_receive_disconnect_response);
		
	transition(STATE_KNX_ROUTING);
}

static void set_knx_error(knx_communication_protocol_error_type_t error_type, uint8_t state)
{
	communication_protocol_type_data.data.knx_communication_protocol_data.error = error_type | state;
}

bool is_knx_physical_address_set(void)
{
	return (knx_physical_address[0] != 0) || (knx_physical_address[1] != 0);
}

bool is_knx_group_address_set(void)
{
	return (knx_group_address[0] != 0) || (knx_group_address[1] != 0);
}

communication_protocol_process_handle_t knx_protocol_send_sensor_readings_and_system_data(circular_buffer_t* sensor_readings_buffer, circular_buffer_t* system_buffer, uint16_t* sent_sensor_readings, uint16_t* sent_system_items)
{
	LOG(1, "KNX send sensor readings and system data");
	
	clear_communication_protocol_data();
	
	sending_sensor_readings_buffer = sensor_readings_buffer;
	sensor_readings_sent = sent_sensor_readings;
	
	add_event_type(&events_buffer, EVENT_SEND);
	
	return knx_process;
}

communication_protocol_process_handle_t knx_protocol_receive_commands(circular_buffer_t* commands_buffer)
{
	LOG(1, "KNX receive commands");
	
	clear_communication_protocol_data();
	
	add_event_type(&events_buffer, EVENT_RECEIVE_ACK);
	
	return knx_process;
}

communication_protocol_process_handle_t knx_protocol_disconnect(void)
{
	LOG(1, "KNX disconnect");
	
	clear_communication_protocol_data();
	
	add_event_type(&events_buffer, EVENT_DISCONNECT);
	
	return knx_process;
}

communication_protocol_type_data_t get_knx_communication_result(void)
{
	return communication_protocol_type_data;
}

static bool knx_tunneling_parameters_set(void)
{
	return *server_ip != 0 && server_port != 0;
}

static bool knx_routing_parameters_set(void)
{
	return *knx_multicast_address != 0 && knx_multicast_port != 0;
}

static uint16_t encode_endpoint(uint8_t* buffer, uint32_t ip_address, uint16_t port, bool nat)
{
	buffer[0] = 8; // structure size
	buffer[1] = 0x01; // protocol code UDP
	
	// If not NAT encode IP address and port number
	if (!nat)
	{
		buffer[2]  = (ip_address) & 0x000000FF;
		buffer[3]  = (ip_address >> 8) & 0x000000FF;
		buffer[4]  = (ip_address >> 16) & 0x000000FF;
		buffer[5]  = (ip_address >> 24) & 0x000000FF;
		
		buffer[6] = port >> 8;
		buffer[7] = port & 0x00FF;
	}

	return 8;
}

static uint16_t encode_tunneling_conection_request_information(uint8_t* buffer)
{
	buffer[0] = 4; // length
	buffer[1] = 0x04; // tunneling connection
	buffer[2] = 0x02; // link layers
	buffer[3] = 0x00; // TODO Check about this

	return 4;
}

static uint16_t create_knx_connect_message(uint8_t* buffer, uint32_t ip_address, uint16_t port, bool nat)
{		
	uint16_t control_endpoint_size = encode_endpoint(buffer + 6, ip_address, port, nat);
	uint16_t data_endpoint_size = encode_endpoint(buffer + 6 + control_endpoint_size, ip_address, port, nat);
	uint16_t tunneling_conection_request_information_size = encode_tunneling_conection_request_information(buffer + 6 + control_endpoint_size + data_endpoint_size);
	
	uint16_t total_size = 6 + control_endpoint_size + data_endpoint_size + tunneling_conection_request_information_size;
	
	buffer[0] = 0x06; // header size
	buffer[1] = 0x10; // protocol version
	
	// connect request
	buffer[2] = 0x02; // service type
	buffer[3] = 0x05; // service type
	
	// total length
	buffer[4] = total_size >> 8;
	buffer[5] = total_size & 0x00FF;
	
	return total_size;
}

static void encode_knx_float_16(int16_t sensor_value, uint8_t* knx_encoded_float_value)
{
	float float_value = (float)sensor_value;
	float_value *= 10; // not 100 since our value is already multiplied with 10
	int exp = 0;
	while (abs(float_value) > 2047)
	{
		float_value /= 2;
		exp++;
	}
	
	int mantisse = (int)float_value;
	knx_encoded_float_value[0] = (uint8_t)((((mantisse >> 8) & 8) << 4) | (exp << 3) | ((mantisse >> 8) & 7));
	knx_encoded_float_value[1] = (uint8_t)mantisse;
}

uint16_t create_knx_cEMI_temperature_message(uint8_t* buffer, int16_t temperature, uint8_t physical_address[2])
{		
	buffer[0] = 0xBC;
	buffer[1] = 0xE0;
	buffer[2] = knx_physical_address[0];
	buffer[3] = knx_physical_address[1];
	buffer[4] = knx_group_address[0];
	buffer[5] = knx_group_address[1];
	buffer[6] = 0x03;
	buffer[7] = 0x00;
	buffer[8] = 0x80;
	encode_knx_float_16(temperature, &buffer[9]);
	
	return 11;
}

uint16_t create_knx_routing_temperature_message(uint8_t* buffer, int16_t temperature, uint8_t physical_address[2])
{
	buffer[0] = 0x06; // header size
	buffer[1] = 0x10; // protocol version
	buffer[2] = 0x05; // service type
	buffer[3] = 0x30; // service type
	buffer[4] = 0x00; // total size
	buffer[5] = 0x13; // total size
	
	buffer[6] = 0x29; // type
	buffer[7] = 0x00; // reserved
	
	uint16_t temperature_message_size = create_knx_cEMI_temperature_message(buffer + 8, temperature, physical_address);
	
	return temperature_message_size + 8;
}

static uint16_t create_knx_tunneling_temperature_message(uint8_t* buffer, int16_t temperature, uint8_t physical_address[2], uint8_t communication_channel, uint8_t counter)
{
	uint16_t temperature_message_size = create_knx_cEMI_temperature_message(buffer + 12, temperature, physical_address);
	
	buffer[0] = 0x06; // header size
	buffer[1] = 0x10; // protocol version
	buffer[2] = 0x04; // service type
	buffer[3] = 0x20; // service type
	buffer[4] = (temperature_message_size + 6) >> 8; // total size
	buffer[5] = (temperature_message_size + 6) & 0x00FF; // total size
	
	buffer[6] = 0x04; // header size
	buffer[7] = communication_channel; // communication channel
	buffer[8] = counter; // sequence counter
	buffer[9] = 0x00; // reserved
	
	buffer[10] = 0x11; // type
	buffer[11] = 0x00; // reserved
	
	return temperature_message_size + 6 + 6;
}

static uint16_t create_knx_disconnect_message(uint8_t* buffer, uint8_t channel, uint32_t ip_address, uint16_t port)
{
	buffer[0] = 0x06; // header size
	buffer[1] = 0x10; // protocol version
	buffer[2] = 0x02; // service type
	buffer[3] = 0x09; // service type
	buffer[4] = 0x00; // total size
	buffer[5] = 0x10; // total size
	
	buffer[6] = channel; // channel
	buffer[7] = 0x00; // reserved
	
	buffer[8] = 0x08; // channel
	buffer[9] = 0x01; // reserved
	
	buffer[10]  = (ip_address) & 0x000000FF;
	buffer[11]  = (ip_address >> 8) & 0x000000FF;
	buffer[12]  = (ip_address >> 16) & 0x000000FF;
	buffer[13]  = (ip_address >> 24) & 0x000000FF;
	
	buffer[14] = port >> 8;
	buffer[15] = port & 0x00FF;
	
	return 16;
}

static bool parse_knx_message(circular_buffer_t* buffer, knx_message_t* knx_message)
{
	uint8_t message_type[] = {0, 0};		
	if(!circular_buffer_peek_array(buffer, 2, 2, message_type))
	{
		return false;
	}
	
	LOG_PRINT(1, PSTR("Received KNX message type %02X%02X\r\n"), message_type[0],  message_type[1]);
	
	if(message_type[0] == 0x05 && message_type[1] == 0x30)
	{
		knx_message->type = KNX_ROUTING_MESSAGE;
		
		uint8_t routing_message_size = 0;
		circular_buffer_peek(buffer, 5, &routing_message_size);
		
		circular_buffer_drop_from_beggining(buffer, routing_message_size);
		
		return true;
	}
	
	if(message_type[0] == 0x02 && message_type[1] == 0x06)
	{
		knx_message->type = KNX_TUNNELING_CONNECT_RESPONSE;
		
		circular_buffer_peek(buffer, 6, &knx_message->data.knx_tunneling_connect_response.channel);
		
		circular_buffer_drop_from_beggining(buffer, 20);
		
		return true;
	}
	
	if(message_type[0] == 0x02 && message_type[1] == 0x0A)
	{
		knx_message->type = KNX_TUNNELING_DISCONNECT_RESPONSE;
		
		circular_buffer_drop_from_beggining(buffer, 8);
		
		return true;
	}
	
	if(message_type[0] == 0x04 && message_type[1] == 0x21)
	{
		knx_message->type = KNX_TUNNELING_ACK;
		
		uint8_t routing_message_size = 0;
		circular_buffer_peek(buffer, 5, &routing_message_size);
		
		circular_buffer_drop_from_beggining(buffer, routing_message_size);
		
		return true;
	}
	
	if(message_type[0] == 0x04 && message_type[1] == 0x20)
	{
		knx_message->type = KNX_TUNNELING_VALUE_CONFIRMATION;
		
		uint8_t routing_message_size = 0;
		circular_buffer_peek(buffer, 5, &routing_message_size);
		
		circular_buffer_drop_from_beggining(buffer, routing_message_size);
		
		return true;
	}
	
	return false;
}

static bool knx_handler(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			return true;
		}
		case EVENT_COMMUNICATION_MODULE_PROCESS:
		{
			if(communication_module_process_handle())
			{
				add_event_type(&events_buffer, EVENT_COMMUNICATION_MODULE_PROCESS);
			}
			else
			{
				add_event_type(&events_buffer, EVENT_COMMUNICATION_MODULE_DONE);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_routing(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering knx routing state");
			
			circular_buffer_clear(&events_buffer);
			
			state->current_state = -1;
			
			return true;
		}
		case EVENT_SEND:
		{
			LOG(1, "Send received in knx routing state");
			
			if(knx_tunneling_parameters_set())
			{
				LOG(1, "Knx tunneling");
				
				strcpy(destination_address, server_ip);
				destination_port = server_port;
				
				bind_port = server_port;
				
				add_event_type(&events_buffer, EVENT_SEND);
				transition(STATE_KNX_CONNECTING);
			}
			else if(knx_routing_parameters_set())
			{
				LOG(1, "Knx routing");
				
				strcpy(destination_address, knx_multicast_address);
				destination_port = knx_multicast_port;
				
				bind_port = 0;
				
				transition(STATE_KNX_ROUTING_SEND);
			}
			else
			{
				LOG(1, "Knx connection parameters missing");
				
				strcpy(destination_address, knx_multicast_address);
				destination_port = server_port;
				
				set_knx_error(ERROR_KNX_PARAMETERS_MISSING, state->id);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving knx routing state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_routing_send(state_machine_state_t* state, event_t* event)
{	
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering knx routing send state");
			
			sensor_readings_t sensor_reading;
			if(circular_buffer_peek(sending_sensor_readings_buffer, 0, &sensor_reading))
			{
				circular_buffer_clear(&knx_buffer);
				
				uint16_t message_size = create_knx_routing_temperature_message(knx_buffer_storage, sensor_reading.values[0], knx_physical_address);
				
				communication_module_process_handle = wifi_communication_module_send_to(knx_buffer_storage, message_size);
				add_event_type(&events_buffer, EVENT_COMMUNICATION_MODULE_PROCESS);
			}
			else
			{
				LOG(1, "KNX no messsages to send");
				
				transition(STATE_KNX_ROUTING);
			}
			
			return true;
		}
		case EVENT_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t send_result = communication_module.get_communication_result();
			append_communication_module_type_data(&send_result, &communication_protocol_type_data.communication_module_type_data);
			
			if(is_communication_module_success(&send_result))
			{
				LOG(1, "Knx routing message sent");
				
				*sensor_readings_sent = 1;
				
				transition(STATE_KNX_ROUTING);
			}
			else
			{
				LOG(1, "Error sending knx routing message");
				
				set_knx_error(ERROR_SENDING_KNX_MESSAGE, state->id);
				
				*sensor_readings_sent = 0;
				
				transition(STATE_KNX_ROUTING);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving knx routing send state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_connecting(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering knx connecting state");
			
			state->current_state = STATE_KNX_CONNECT_TO_NETWORK;
			
			return true;
		}
		case EVENT_SEND:
		{
			add_event_type(&events_buffer, EVENT_SEND);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving knx connecting state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_connect_to_network(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering knx connect to network state");

			communication_module_process_handle = wifi_communication_module_connect();
			add_event_type(&events_buffer, EVENT_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t connect_result = get_wifi_communication_result();
			append_communication_module_type_data(&connect_result, &communication_protocol_type_data.communication_module_type_data);
			
			if(is_communication_module_success(&connect_result))
			{
				LOG(1, "KNX connected to network");
				
				transition(STATE_KNX_SEND_CONNECT_REQUEST);
			}
			else
			{
				LOG(1, "Error connecting to KNX network");
				
				set_knx_error(ERROR_CONNECTING_TO_KNX_NETWORK, state->id);
				
				transition(STATE_KNX_ROUTING);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving knx connect to network state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_send_connect_request(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering knx send connect request state");
			
			circular_buffer_clear(&knx_buffer);
			
			char ip_address[15];
			uint32_t ip_adress_hex = wifi_communication_module_dependencies.get_ip_address(ip_address);
			
			uint16_t message_size = create_knx_connect_message(knx_buffer_storage, ip_adress_hex, server_port, knx_nat);
			
			communication_module_process_handle = wifi_communication_module_send_to(knx_buffer_storage, message_size);
			add_event_type(&events_buffer, EVENT_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_MODULE_DONE:
		{	
			communication_module_type_data_t send_result = get_wifi_communication_result();
			append_communication_module_type_data(&send_result, &communication_protocol_type_data.communication_module_type_data);
			
			if(is_communication_module_success(&send_result))
			{
				LOG(1, "KNX connect message sent");
				
				transition(STATE_KNX_RECEIVE_CONNECT_RESPONSE);
			}
			else
			{
				LOG(1, "Error sending KNX connect message");
				
				set_knx_error(ERROR_SENDING_KNX_MESSAGE, state->id);
				
				transition(STATE_KNX_ROUTING);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving knx send connect request state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_receive_connect_response(state_machine_state_t* state, event_t* event)
{	
	static uint16_t received_data_size;
	
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering knx receive connect response state");
			
			circular_buffer_clear(&knx_buffer);
			received_data_size = 0;
			
			communication_module_process_handle = wifi_communication_module_receive_from(knx_buffer_storage, KNX_BUFFER_SIZE, &received_data_size);
			add_event_type(&events_buffer, EVENT_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_MODULE_DONE:
		{		
			communication_module_type_data_t receive_result = get_wifi_communication_result();
			append_communication_module_type_data(&receive_result, &communication_protocol_type_data.communication_module_type_data);
			
			if(is_communication_module_success(&receive_result))
			{		
				circular_buffer_add_array(&knx_buffer, knx_buffer_storage, received_data_size);
				knx_message_t knx_message;
				
				while(parse_knx_message(&knx_buffer, &knx_message))
				{
					if(knx_message.type == KNX_TUNNELING_CONNECT_RESPONSE)
					{
						LOG(1, "Knx connect response received");
						
						channel = knx_message.data.knx_tunneling_connect_response.channel;
						sequence_counter = 0;
						
						transition(STATE_KNX_TUNNELING);
						
						return true;
					}
				}
				
				LOG(1, "Knx connect response not received");
				
				channel = 0;
				
				set_knx_error(ERROR_INCORRECT_KNX_MESSAGE_RECEIVED, state->id);
				
				transition(STATE_KNX_ROUTING);
			}
			else
			{
				LOG(1, "Error receiving knx connect response");
				
				channel = 0;
				
				set_knx_error(ERROR_RECEIVING_KNX_MESSAGE, state->id);
				
				transition(STATE_KNX_ROUTING);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving knx receive connect response state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_tunneling(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering knx tunneling state");
			
			state->current_state = -1;
			
			return true;
		}
		case EVENT_SEND:
		{
			LOG(1, "Send received in knx tunneling state");
			
			transition(STATE_KNX_TUNELING_SEND);
			
			return true;
		}
		case EVENT_RECEIVE_ACK:
		{
			LOG(1, "Receive ack received in knx tunneling state");
			
			transition(STATE_KNX_TUNELING_RECEIVE_ACK);
			
			return true;
		}
		case EVENT_DISCONNECT:
		{
			LOG(1, "Disconnect received in knx tunneling state");
			
			transition(STATE_KNX_DISCONNECTING);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving knx tunneling state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_tunneling_send(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering knx tunneling send state");
			
			sensor_readings_t sensor_reading;
			if(circular_buffer_peek(sending_sensor_readings_buffer, 0, &sensor_reading))
			{
				circular_buffer_clear(&knx_buffer);
				
				uint16_t message_size = create_knx_tunneling_temperature_message(knx_buffer_storage, sensor_reading.values[0], knx_physical_address, channel, sequence_counter++);
				
				communication_module_process_handle = wifi_communication_module_send_to(knx_buffer_storage, message_size);
				add_event_type(&events_buffer, EVENT_COMMUNICATION_MODULE_PROCESS);
			}
			else
			{
				LOG(1, "KNX no messages to send");
				
				transition(STATE_KNX_TUNNELING);
			}
		
			return true;
		}
		case EVENT_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t send_result = get_wifi_communication_result();
			append_communication_module_type_data(&send_result, &communication_protocol_type_data.communication_module_type_data);
			
			if(is_communication_module_success(&send_result))
			{
				LOG(1, "Knx tunneling message sent");
				
				*sensor_readings_sent = 1;
				
				transition(STATE_KNX_TUNNELING);
			}
			else
			{
				LOG(1, "Error sending knx tunneling message");
				
				set_knx_error(ERROR_SENDING_KNX_MESSAGE, state->id);
				
				transition(STATE_KNX_ROUTING);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving knx tunneling send state");
		
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_tunneling_receive_ack(state_machine_state_t* state, event_t* event)
{
	static uint16_t received_data_size;
	
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering knx receive ack state");
			
			circular_buffer_clear(&knx_buffer);
			
			communication_module_process_handle = wifi_communication_module_receive_from(knx_buffer_storage, KNX_BUFFER_SIZE, &received_data_size);
			add_event_type(&events_buffer, EVENT_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t receive_result = get_wifi_communication_result();
			append_communication_module_type_data(&receive_result, &communication_protocol_type_data.communication_module_type_data);
			
			if(is_communication_module_success(&receive_result))
			{
				circular_buffer_add_array(&knx_buffer, knx_buffer_storage, received_data_size);
				knx_message_t knx_message;
				
				while(parse_knx_message(&knx_buffer, &knx_message))
				{
					if(knx_message.type == KNX_TUNNELING_ACK)
					{
						LOG(1, "Knx tunneling ack received");
						
						transition(STATE_KNX_TUNNELING);
						
						return true;
					}
				}
				
				LOG(1, "Knx tunneling ack not received");
				
				set_knx_error(ERROR_INCORRECT_KNX_MESSAGE_RECEIVED, state->id);
				
				transition(STATE_KNX_ROUTING);
			}
			else
			{
				LOG(1, "Error receiving knx ack");
				
				set_knx_error(ERROR_RECEIVING_KNX_MESSAGE, state->id);
				
				transition(STATE_KNX_ROUTING);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving knx receive ack state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_disconnecting(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering knx disconnecting state");
			
			state->current_state = STATE_KNX_SEND_DISCONNECT_REQUEST;
		
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving knx disconnecting state");
		
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_send_disconnect_request(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering knx send disconnect request state");
			
			circular_buffer_clear(&knx_buffer);
			
			char ip_address[15];
			uint32_t ip_adress_hex = wifi_communication_module_dependencies.get_ip_address(ip_address);
			
			uint16_t message_size = create_knx_disconnect_message(knx_buffer_storage, channel, ip_adress_hex, server_port);
			
			communication_module_process_handle = wifi_communication_module_send_to(knx_buffer_storage, message_size);
			add_event_type(&events_buffer, EVENT_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t send_result = get_wifi_communication_result();
			append_communication_module_type_data(&send_result, &communication_protocol_type_data.communication_module_type_data);
			
			if(is_communication_module_success(&send_result))
			{
				LOG(1, "Knx disconnect request sent");
				
				transition(STATE_KNX_RECEIVE_DISCONNECT_RESPONSE);				
			}
			else
			{
				LOG(1, "Error sending knx disconnect request");
				
				set_knx_error(ERROR_SENDING_KNX_MESSAGE, state->id);
				
				transition(STATE_KNX_ROUTING);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving knx send disconnect request state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_receive_disconnect_response(state_machine_state_t* state, event_t* event)
{
	static uint16_t received_data_size;
	
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering knx receive disconnect response state");
			
			circular_buffer_clear(&knx_buffer);
			
			communication_module_process_handle = wifi_communication_module_receive_from(knx_buffer_storage, KNX_BUFFER_SIZE, &received_data_size);
			add_event_type(&events_buffer, EVENT_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t receive_result = get_wifi_communication_result();
			append_communication_module_type_data(&receive_result, &communication_protocol_type_data.communication_module_type_data);
			
			if(is_communication_module_success(&receive_result))
			{
				circular_buffer_add_array(&knx_buffer, knx_buffer_storage, received_data_size);
				knx_message_t knx_message;
				
				while(parse_knx_message(&knx_buffer, &knx_message))
				{
					if(knx_message.type == KNX_TUNNELING_DISCONNECT_RESPONSE)
					{
						LOG(1, "Knx disconnect response received, knx disconnected");
						
						transition(STATE_KNX_ROUTING);
						
						return true;
					}
				}
				
				LOG(1, "Knx disconnect response not received");
				
				set_knx_error(ERROR_INCORRECT_KNX_MESSAGE_RECEIVED, state->id);
			}
			else
			{
				LOG(1, "Error receiving knx disconnect response");
				
				set_knx_error(ERROR_RECEIVING_KNX_MESSAGE, state->id);
			}
			
			transition(STATE_KNX_ROUTING);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving knx receive disconnect response state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}
