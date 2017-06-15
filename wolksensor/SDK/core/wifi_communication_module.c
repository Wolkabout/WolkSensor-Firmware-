#include "wifi_communication_module.h"
#include "wifi_communication_module_dependencies.h"
#include "state_machine.h"
#include "logger.h"
#include "event_buffer.h"
#include "config.h"
#include "global_dependencies.h"
#include "commands_dependencies.h"
#include "tcp_communication_module.h"
#include "tcp_communication_module_dependencies.h"
#include "udp_communication_module.h"
#include "udp_communication_module_dependencies.h"
#include "communication_module.h"

#define WIFI_COMMUNICATION_MODULE_EVENTS_BUFFER_SIZE 10

#define WIFI_CONNECTING_TIMEOUT 20
#define WIFI_SEND_TIMEOUT 3
#define WIFI_RECEIVE_TIMEOUT 3
#define WIFI_DISCONECT_TIMEOUT 3 
#define WIFI_RESET_TIMEOUT 3 

typedef enum
{
	EVENT_WIFI_DISCONNECT = 0,
	EVENT_WIFI_CONNECT,
	EVENT_WIFI_DISCONNECT_SOCKET_CLOSED,
	EVENT_WIFI_SEND,
	EVENT_WIFI_SEND_TO,
	EVENT_WIFI_RECEIVE,
	EVENT_WIFI_RECEIVE_FROM,
	EVENT_WIFI_STOP,
	EVENT_WIFI_STOP_SOCKET_CLOSED,
	EVENT_WIFI_CONNECTING,
	EVENT_WIFI_CONNECTED,
	EVENT_WIFI_ACQUIRING_IP_ADDRESS,
	EVENT_WIFI_IP_ADDRESS_ACQUIRED,
	EVENT_WIFI_DISCONNECTING,
	EVENT_WIFI_DISCONNECTED,
	EVENT_WIFI_SEQUENCE_TIMEOUT,
	EVENT_WIFI_ERROR,
	EVENT_WIFI_CLOSE_SOCKET,
	EVENT_WIFI_COMMUNICATION_MODULE_PROCESS,
	EVENT_WIFI_COMMUNICATION_MODULE_DONE
}
wifi_communication_module_events_t;

wifi_communication_module_dependencies_t wifi_communication_module_dependencies; 

state_machine_state_t wifi_communication_module_state_machine;
state_machine_state_t wifi_communication_module_states[16];

static circular_buffer_t wifi_communication_module_event_buffer;
static event_t wifi_communication_module_event_buffer_storage[WIFI_COMMUNICATION_MODULE_EVENTS_BUFFER_SIZE];

static uint8_t* data = NULL;
static uint16_t data_size = 0;

static uint8_t* buffer = NULL;
static uint16_t buffer_size = 0;
static uint16_t* received_data_size = NULL;

static volatile uint8_t wifi_sequence_timeout_timer = 0;

static volatile uint16_t tick_counter = 0;

static wifi_communication_module_data_t wifi_communication_module_data;

static uint32_t platform_specific_error_code = 0;

// forward declaration of state machine state handlers
static bool wifi_communication_module_handler(state_machine_state_t* state, event_t* event);

static communication_module_process_handle_t communication_module_process_handle = NULL;
static communication_module_process_handle_t tcp_process_handle = NULL;
static communication_module_process_handle_t udp_process_handle = NULL;

void (*tcp_second_expired_listener)(void) = NULL;
void (*tcp_milisecond_expired_listener)(void) = NULL;
void (*tcp_platform_specific_error_code_listener)(uint32_t operation_code) = NULL;

void (*udp_second_expired_listener)(void) = NULL;
void (*udp_milisecond_expired_listener)(void) = NULL;
void (*udp_platform_specific_error_code_listener)(uint32_t operation_code) = NULL;

static bool state_stopped(state_machine_state_t* state, event_t* event);
static bool state_starting(state_machine_state_t* state, event_t* event);
static bool state_started(state_machine_state_t* state, event_t* event);
static bool state_disconnected(state_machine_state_t* state, event_t* event);
static bool state_connecting(state_machine_state_t* state, event_t* event);
static bool state_connecting_to_ap(state_machine_state_t* state, event_t* event);
static bool state_acquiring_ip_address(state_machine_state_t* state, event_t* event);
static bool state_connected(state_machine_state_t* state, event_t* event);
static bool state_send(state_machine_state_t* state, event_t* event);
static bool state_send_to(state_machine_state_t* state, event_t* event);
static bool state_receive(state_machine_state_t* state, event_t* event);
static bool state_receive_from(state_machine_state_t* state, event_t* event);
static bool state_closing_socket(state_machine_state_t* state, event_t* event);
static bool state_disconnecting(state_machine_state_t* state, event_t* event);
static bool state_reset(state_machine_state_t* state, event_t* event);
static bool state_stopping(state_machine_state_t* state, event_t* event);

static void init_wifi_communication_module_event_buffer(void)
{
	circular_buffer_init(&wifi_communication_module_event_buffer, wifi_communication_module_event_buffer_storage, WIFI_COMMUNICATION_MODULE_EVENTS_BUFFER_SIZE, sizeof(event_t), true, true);
}

void add_wifi_communication_module_event_type(uint8_t event_type)
{
	add_event_type(&wifi_communication_module_event_buffer, event_type);
}

void add_wifi_communication_module_event(event_t* event)
{
	add_event(&wifi_communication_module_event_buffer, event);
}

bool pop_wifi_communication_module_event(event_t* event)
{
	return pop_event(&wifi_communication_module_event_buffer, event);
}

static bool wifi_communication_module_process_event(void)
{
	event_t event;
	if(pop_wifi_communication_module_event(&event))
	{
		state_machine_process_event(wifi_communication_module_states, &wifi_communication_module_state_machine, &event);
		return true;
	}
	
	return false;
}

static void init_state(wifi_communication_module_states_t id, const char* human_readable_name, state_machine_state_t* parent, int8_t initial_state, state_machine_state_handler handler)
{
	state_machine_init_state(id, human_readable_name, parent, wifi_communication_module_states, initial_state, handler);
}

static void transition(wifi_communication_module_states_t new_state_id)
{
	state_machine_transition(wifi_communication_module_states, &wifi_communication_module_state_machine, new_state_id);
}

static void schedule_wifi_sequence_timeout(uint16_t period)
{
	wifi_sequence_timeout_timer = period;
}

static void stopwatch_start(void)
{
	tick_counter = 1;
}

static uint16_t stopwatch_stop(void)
{
	uint16_t time = tick_counter;
	tick_counter = 0;
	return time;
}

static void second_expired_listener(void)
{
	if (wifi_sequence_timeout_timer && (--wifi_sequence_timeout_timer == 0))
	{
		LOG(1, "Wifi sequence timeout!");
		
		add_wifi_communication_module_event_type(EVENT_WIFI_SEQUENCE_TIMEOUT);
	}
	
	if(tcp_second_expired_listener) tcp_second_expired_listener();
	if(udp_second_expired_listener) udp_second_expired_listener();
}

static void milisecond_expired_listener(void)
{
	if(tick_counter) tick_counter++;
	
	if(tcp_milisecond_expired_listener) tcp_milisecond_expired_listener();
	if(udp_milisecond_expired_listener) udp_milisecond_expired_listener();
}

static void wifi_connected_listener(void)
{
	add_wifi_communication_module_event_type(EVENT_WIFI_CONNECTED);
}

static void wifi_ip_address_acquired_listener(void)
{
	add_wifi_communication_module_event_type(EVENT_WIFI_IP_ADDRESS_ACQUIRED);
}

static void wifi_disconnected_listener(void)
{
	add_wifi_communication_module_event_type(EVENT_WIFI_DISCONNECTED);
}

static void wifi_module_error_listener(void)
{
	add_wifi_communication_module_event_type(EVENT_WIFI_ERROR);
}

static void send_command_response_string(const char* response)
{
	global_dependencies.send_response(response, strlen(response));
}

static bool is_static_ip_set(void)
{
	return strcmp(wifi_static_ip, "") != 0 && strcmp(wifi_static_mask, "") != 0 && strcmp(wifi_static_gateway, "") != 0 && strcmp(wifi_static_dns, "") != 0;
}

void add_udp_second_expired_listener(void (*listener)(void))
{
	udp_second_expired_listener = listener;
}

void add_udp_milisecond_expired_listener(void (*listener)(void))
{
	udp_milisecond_expired_listener = listener;
}

void add_udp_platform_specific_error_code_listener(void (*listener)(int operation_code))
{
	udp_platform_specific_error_code_listener = listener;
}

void add_tcp_platform_specific_error_code_listener(void (*listener)(int operation_code))
{
	tcp_platform_specific_error_code_listener = listener;
}

void add_tcp_second_expired_listener(void (*listener)(void))
{
	tcp_second_expired_listener = listener;
}

void add_tcp_milisecond_expired_listener(void (*listener)(void))
{
	tcp_milisecond_expired_listener = listener;
}

static void wifi_platform_specific_error_code_listener(uint32_t error_code)
{
	platform_specific_error_code = error_code;
	if(tcp_platform_specific_error_code_listener) tcp_platform_specific_error_code_listener(error_code);
	if(udp_platform_specific_error_code_listener) udp_platform_specific_error_code_listener(error_code);
}

static void load_wifi_parameters(void)
{
	LOG(1, "Load wifi connection parameters from config");
	
	load_wifi_ssid();
	load_wifi_password();
	load_wifi_auth_type();
	load_wifi_static_ip();
	load_wifi_static_mask();
	load_wifi_static_gateway();
	load_wifi_static_dns();
}

static void set_wifi_communication_module_error(wifi_communication_module_error_type_t error_type, uint8_t state)
{
	if(!wifi_communication_module_data.error)
	{
		wifi_communication_module_data.error = error_type | state;
		wifi_communication_module_data.platform_specific_error_code = platform_specific_error_code;
	}
}

void init_wifi_communication_module(void)
{
	LOG(1, "Wifi communication module init");
	
	// dependencies
	wifi_communication_module_dependencies.add_milisecond_expired_listener(milisecond_expired_listener);
	wifi_communication_module_dependencies.add_second_expired_listener(second_expired_listener);
		
	wifi_communication_module_dependencies.add_wifi_connected_listener(wifi_connected_listener);
	wifi_communication_module_dependencies.add_wifi_ip_address_acquired_listener(wifi_ip_address_acquired_listener);
	wifi_communication_module_dependencies.add_wifi_disconnected_listener(wifi_disconnected_listener);
	wifi_communication_module_dependencies.add_wifi_error_listener(wifi_module_error_listener);
	wifi_communication_module_dependencies.add_wifi_platform_specific_error_code_listener(wifi_platform_specific_error_code_listener);
		
	// buffers
	init_wifi_communication_module_event_buffer();
	
	// parameters
	load_wifi_parameters();
	
	// state machine
	wifi_communication_module_state_machine.id = -1;
	wifi_communication_module_state_machine.human_readable_name = NULL;
	wifi_communication_module_state_machine.parent = NULL;
	wifi_communication_module_state_machine.current_state = STATE_WIFI_STOPPED;
	wifi_communication_module_state_machine.handler = wifi_communication_module_handler;
	
	init_state(STATE_WIFI_STOPPED, PSTR("STOPPED"), &wifi_communication_module_state_machine, -1, state_stopped);
	init_state(STATE_WIFI_STARTING, PSTR("STARTING"), &wifi_communication_module_state_machine, -1, state_starting);
	init_state(STATE_WIFI_STARTED, PSTR("STARTED"),  &wifi_communication_module_state_machine, STATE_WIFI_DISCONNECTED, state_started);
		init_state(STATE_WIFI_DISCONNECTED, PSTR("DISCONNECTED"), &wifi_communication_module_states[STATE_WIFI_STARTED], -1, state_disconnected);
		init_state(STATE_WIFI_CONNECTING, PSTR("CONNECTING"), &wifi_communication_module_states[STATE_WIFI_STARTED], STATE_WIFI_CONNECTING_TO_AP, state_connecting);
			init_state(STATE_WIFI_CONNECTING_TO_AP, PSTR("CONNECTING_TO_AP"),  &wifi_communication_module_states[STATE_WIFI_CONNECTING], -1, state_connecting_to_ap);
			init_state(STATE_WIFI_ACQUIRING_IP_ADDRESS, PSTR("ACQUIRING_IP_ADDRESS"),  &wifi_communication_module_states[STATE_WIFI_CONNECTING], -1, state_acquiring_ip_address);
		init_state(STATE_WIFI_CONNECTED, PSTR("CONNECTED"), &wifi_communication_module_states[STATE_WIFI_STARTED], -1, state_connected);
			init_state(STATE_WIFI_SEND, PSTR("SEND"),  &wifi_communication_module_states[STATE_WIFI_CONNECTED], -1, state_send);
			init_state(STATE_WIFI_SEND_TO, PSTR("SEND_TO"),  &wifi_communication_module_states[STATE_WIFI_CONNECTED], -1, state_send_to);
			init_state(STATE_WIFI_RECEIVE, PSTR("RECEIVE"),  &wifi_communication_module_states[STATE_WIFI_CONNECTED], -1, state_receive);
			init_state(STATE_WIFI_RECEIVE_FROM, PSTR("RECEIVE_FROM"),  &wifi_communication_module_states[STATE_WIFI_CONNECTED], -1, state_receive_from);
			init_state(STATE_WIFI_CLOSING_SOCKET, PSTR("CLOSING_SOCKET"),  &wifi_communication_module_states[STATE_WIFI_CONNECTED], -1, state_closing_socket);
		init_state(STATE_WIFI_DISCONNECTING, PSTR("DISCONNECTING"), &wifi_communication_module_states[STATE_WIFI_STARTED], -1, state_disconnecting);
		init_state(STATE_WIFI_RESET, PSTR("RESET"), &wifi_communication_module_states[STATE_WIFI_STARTED], -1, state_reset);
	init_state(STATE_WIFI_STOPPING, PSTR("STOPPING"), &wifi_communication_module_state_machine, -1, state_stopping);
	
	// init TCP communication
	tcp_communication_module_dependencies.open_socket = wifi_communication_module_dependencies.wifi_open_socket;
	tcp_communication_module_dependencies.close_socket = wifi_communication_module_dependencies.wifi_close_socket;
	tcp_communication_module_dependencies.receive = wifi_communication_module_dependencies.wifi_receive;
	tcp_communication_module_dependencies.send = wifi_communication_module_dependencies.wifi_send;
	tcp_communication_module_dependencies.serialize_platform_specific_error_code = wifi_communication_module_dependencies.serialize_wifi_platform_specific_error_code;
	
	tcp_communication_module_dependencies.add_second_expired_listener = add_tcp_second_expired_listener;
	tcp_communication_module_dependencies.add_milisecond_expired_listener = add_tcp_milisecond_expired_listener;
	tcp_communication_module_dependencies.add_socket_closed_listener = wifi_communication_module_dependencies.add_wifi_socket_closed_listener;
	tcp_communication_module_dependencies.add_platform_specific_error_code_listener = add_tcp_platform_specific_error_code_listener;
	
	init_tcp_communication_module();
	
	// init UDP communication
	udp_communication_module_dependencies.open_udp_socket = wifi_communication_module_dependencies.wifi_open_udp_socket;
	udp_communication_module_dependencies.close_socket = wifi_communication_module_dependencies.wifi_close_socket;
	udp_communication_module_dependencies.send_to = wifi_communication_module_dependencies.wifi_send_to;
	udp_communication_module_dependencies.receive_from = wifi_communication_module_dependencies.wifi_receive_from;
	udp_communication_module_dependencies.serialize_platform_specific_error_code = wifi_communication_module_dependencies.serialize_wifi_platform_specific_error_code;
	
	udp_communication_module_dependencies.add_second_expired_listener = add_udp_second_expired_listener;
	udp_communication_module_dependencies.add_milisecond_expired_listener = add_udp_milisecond_expired_listener;
	udp_communication_module_dependencies.add_platform_specific_error_code_listener = add_udp_platform_specific_error_code_listener;
	
	init_udp_communication_module();
	
	// plug into commands
	commands_dependencies.communication_module_close_socket = wifi_communication_module_close_socket;
	commands_dependencies.is_static_ip_set = is_static_ip_set;
	commands_dependencies.wifi_communication_module_disconnect = wifi_communication_module_disconnect;
}

communication_module_process_handle_t wifi_communication_module_connect(void)
{
	LOG(1, "Wifi connect");

	memset(&wifi_communication_module_data, 0, sizeof(wifi_communication_module_data_t));
	
	add_wifi_communication_module_event_type(EVENT_WIFI_CONNECT);
	
	return wifi_communication_module_process_event;
}

communication_module_process_handle_t wifi_communication_module_send(uint8_t* data_in, uint16_t data_in_size)
{
	LOG(1, "Wifi send");
	
	data = data_in;
	data_size = data_in_size;

	memset(&wifi_communication_module_data, 0, sizeof(wifi_communication_module_data_t));
	
	add_wifi_communication_module_event_type(EVENT_WIFI_SEND);
	
	return wifi_communication_module_process_event;
}

communication_module_process_handle_t wifi_communication_module_send_to(uint8_t* data_in, uint16_t data_in_size)
{
	LOG(1, "Wifi send to");
	
	data = data_in;
	data_size = data_in_size;

	memset(&wifi_communication_module_data, 0, sizeof(wifi_communication_module_data_t));
	
	add_wifi_communication_module_event_type(EVENT_WIFI_SEND_TO);
	
	return wifi_communication_module_process_event;
}

communication_module_process_handle_t wifi_communication_module_receive(uint8_t* buffer_out, uint16_t buffer_out_size, uint16_t* received_data_size_out)
{
	LOG(1, "Wifi receive");

	buffer = buffer_out;
	buffer_size = buffer_out_size;
	received_data_size = received_data_size_out;
	
	memset(&wifi_communication_module_data, 0, sizeof(wifi_communication_module_data_t));
	
	add_wifi_communication_module_event_type(EVENT_WIFI_RECEIVE);
	
	return wifi_communication_module_process_event;
}

communication_module_process_handle_t wifi_communication_module_receive_from(uint8_t* buffer_out, uint16_t buffer_out_size, uint16_t* received_data_size_out)
{
	LOG(1, "Wifi receive from");

	buffer = buffer_out;
	buffer_size = buffer_out_size;
	received_data_size = received_data_size_out;
	
	memset(&wifi_communication_module_data, 0, sizeof(wifi_communication_module_data_t));
	
	add_wifi_communication_module_event_type(EVENT_WIFI_RECEIVE_FROM);
	
	return wifi_communication_module_process_event;
}

communication_module_process_handle_t wifi_communication_module_close_socket(void)
{
	LOG(1, "Wifi close socket");
	
	memset(&wifi_communication_module_data, 0, sizeof(wifi_communication_module_data_t));
	
	add_wifi_communication_module_event_type(EVENT_WIFI_CLOSE_SOCKET);
	
	return wifi_communication_module_process_event;
}

communication_module_process_handle_t wifi_communication_module_disconnect(void)
{
	LOG(1, "Wifi disconnect");
	
	memset(&wifi_communication_module_data, 0, sizeof(wifi_communication_module_data_t));
	
	add_wifi_communication_module_event_type(EVENT_WIFI_DISCONNECT);
	
	return wifi_communication_module_process_event;
}

communication_module_process_handle_t wifi_communication_module_stop(void)
{
	LOG(1, "Wifi stop");
	
	memset(&wifi_communication_module_data, 0, sizeof(wifi_communication_module_data_t));
	
	add_wifi_communication_module_event_type(EVENT_WIFI_STOP);
	
	return wifi_communication_module_process_event;
}

static void remove_send_and_receive_events(void)
{
	uint16_t number_of_events = circular_buffer_size(&wifi_communication_module_event_buffer);
	event_t event;
	
	uint8_t i;
	for(i = 0; i < number_of_events; i++)
	{
		if(pop_event(&wifi_communication_module_event_buffer, &event))
		{
			switch(event.type)
			{
				case EVENT_WIFI_SEND:
				case EVENT_WIFI_SEND_TO:
				case EVENT_WIFI_RECEIVE:
				case EVENT_WIFI_RECEIVE_FROM:
				case EVENT_WIFI_CONNECT:
				{
					break; // do not return them into buffer
				}
				default:
				{
					add_wifi_communication_module_event_type(event.type);
					break;
				}
			}
		}
	}
}

static bool wifi_communication_module_handler(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"Entering wifi communication module");
			
			return true;
		}
		case EVENT_WIFI_COMMUNICATION_MODULE_PROCESS:
		{
			if(communication_module_process_handle())
			{
				add_wifi_communication_module_event_type(EVENT_WIFI_COMMUNICATION_MODULE_PROCESS);
			}
			else
			{
				add_wifi_communication_module_event_type(EVENT_WIFI_COMMUNICATION_MODULE_DONE);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving wifi communication module");
			
			return false;
		}
		default:
		{
			return true;
		}
	}
}

static bool state_stopped(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"Entering wifi stopped state");
			
			return true;
		}
		case EVENT_WIFI_CONNECT:
		{
			LOG(1, "Wifi connect received in stopped state");

			add_wifi_communication_module_event_type(EVENT_WIFI_CONNECT); // retain so we know what to do further
			transition(STATE_WIFI_STARTING);
			
			return true;
		}
		case EVENT_WIFI_SEND:
		{
			LOG(1, "Wifi send received in stopped state");

			add_wifi_communication_module_event_type(EVENT_WIFI_SEND); // retain so we know what to do further
			transition(STATE_WIFI_STARTING);
			
			return true;
		}
		case EVENT_WIFI_SEND_TO:
		{
			LOG(1, "Wifi send to received in stopped state");

			add_wifi_communication_module_event_type(EVENT_WIFI_SEND_TO); // retain so we know what to do further
			transition(STATE_WIFI_STARTING);
			
			return true;
		}
		case EVENT_WIFI_RECEIVE:
		{
			LOG(1, "Wifi receive received in stopped state");

			add_wifi_communication_module_event_type(EVENT_WIFI_RECEIVE); // retain so we know what to do further
			transition(STATE_WIFI_STARTING);
			
			return true;
		}
		case EVENT_WIFI_RECEIVE_FROM:
		{
			LOG(1, "Wifi receive from received in stopped state");

			add_wifi_communication_module_event_type(EVENT_WIFI_RECEIVE_FROM); // retain so we know what to do further
			transition(STATE_WIFI_STARTING);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving wifi stopped state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_starting(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"Entering wifi starting state")
			
			if(wifi_communication_module_dependencies.wifi_start())
			{
				LOG(1, "Wifi started");
				
				transition(STATE_WIFI_STARTED);
			}
			else
			{
				LOG(1, "Unable to start wifi");
				
				set_wifi_communication_module_error(WIFI_OPERATION_FAILED, state->id);
				
				circular_buffer_clear(&wifi_communication_module_event_buffer);
				
				transition(STATE_WIFI_STOPPED);
			}
			
			return true;
		}
		case EVENT_WIFI_CONNECT:
		case EVENT_WIFI_SEND:
		case EVENT_WIFI_SEND_TO:
		case EVENT_WIFI_RECEIVE:
		case EVENT_WIFI_RECEIVE_FROM:
		{
			add_wifi_communication_module_event_type(event->type); // retain while starting

			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving wifi starting state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_started(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering wifi started state");
			
			state->current_state = STATE_WIFI_DISCONNECTED;
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving wifi started state");

			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_reset(state_machine_state_t* state, event_t* event)
{	
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"Entering wifi reset state");
			
			tcp_process_handle = tcp_communication_module_close_socket();
			udp_process_handle = udp_communication_module_close_socket();
			
			add_wifi_communication_module_event_type(EVENT_WIFI_COMMUNICATION_MODULE_PROCESS);
		
			return true;
		}
		case EVENT_WIFI_COMMUNICATION_MODULE_PROCESS:
		{
			if(tcp_process_handle() || udp_process_handle())
			{
				add_wifi_communication_module_event_type(EVENT_WIFI_COMMUNICATION_MODULE_PROCESS);
			}
			else
			{
				add_wifi_communication_module_event_type(EVENT_WIFI_COMMUNICATION_MODULE_DONE);
			}
			
			return true;
		}
		case EVENT_WIFI_COMMUNICATION_MODULE_DONE:
		{
			if(!wifi_communication_module_dependencies.wifi_reset())
			{
				LOG(1,"Unable to reset wifi, reset the device");
				
				set_wifi_communication_module_error(WIFI_OPERATION_FAILED, state->id);
			}
			
			transition(STATE_WIFI_DISCONNECTED);
			
			return true;
		}
		case EVENT_WIFI_STOP_SOCKET_CLOSED:
		case EVENT_WIFI_STOP:
		{
			add_wifi_communication_module_event_type(EVENT_WIFI_STOP); // preserve stop
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving wifi reset state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_disconnected(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"Entering wifi disconnected state");
			
			return true;
		}
		case EVENT_WIFI_STOP:
		{
			LOG(1, "Wifi stop in disconnected state");
			
			transition(STATE_WIFI_STOPPING);
			
			return true;
		}
		case EVENT_WIFI_CONNECT:
		{
			LOG(1, "Wifi connect in disconnected state");
			
			transition(STATE_WIFI_CONNECTING);
			
			return true;
		}
		case EVENT_WIFI_SEND:
		{
			LOG(1, "Wifi send in disconnected state");
			
			if((*wifi_ssid != 0 && *wifi_password != 0) || (*wifi_ssid != 0 && *wifi_password == 0))
			{
				add_wifi_communication_module_event_type(EVENT_WIFI_SEND);
				transition(STATE_WIFI_CONNECTING);
			}
			else
			{
				LOG(1, "Wifi parameters not set");
				
				set_wifi_communication_module_error(WIFI_PARAMETERS_MISSING, state->id);
			}
			
			return true;
		}
		case EVENT_WIFI_SEND_TO:
		{
			LOG(1, "Wifi send to in disconnected state");

			if((*wifi_ssid != 0 && *wifi_password != 0) || (*wifi_ssid != 0 && *wifi_password == 0))
			{
				add_wifi_communication_module_event_type(EVENT_WIFI_SEND_TO);
				transition(STATE_WIFI_CONNECTING);
			}
			else
			{
				LOG(1, "Wifi parameters not set");
				
				set_wifi_communication_module_error(WIFI_PARAMETERS_MISSING, state->id);
			}
			
			return true;
		}
		case EVENT_WIFI_RECEIVE:
		{
			LOG(1, "Wifi receive in disconnected state");

			if((*wifi_ssid != 0 && *wifi_password != 0) || (*wifi_ssid != 0 && *wifi_password == 0))
			{
				add_wifi_communication_module_event_type(EVENT_WIFI_RECEIVE);
				transition(STATE_WIFI_CONNECTING);
			}
			else
			{
				LOG(1, "Wifi parameters not set");
				
				set_wifi_communication_module_error(WIFI_PARAMETERS_MISSING, state->id);
			}
			
			return true;
		}
		case EVENT_WIFI_RECEIVE_FROM:
		{
			LOG(1, "Wifi receive from in disconnected state");

			if((*wifi_ssid != 0 && *wifi_password != 0) || (*wifi_ssid != 0 && *wifi_password == 0))
			{
				add_wifi_communication_module_event_type(EVENT_WIFI_RECEIVE_FROM);
				transition(STATE_WIFI_CONNECTING);
			}
			else
			{
				LOG(1, "Wifi parameters not set");
				
				set_wifi_communication_module_error(WIFI_PARAMETERS_MISSING, state->id);
			}
			
			return true;
		}
		case EVENT_WIFI_ERROR:
		{
			LOG(1, "Wifi error in disconnected state");
			
			set_wifi_communication_module_error(WIFI_ERROR, state->id);
			
			remove_send_and_receive_events();
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving wifi disconnected state");
			
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
			LOG(1, "Entering wifi connecting state");
			
			state->current_state = STATE_WIFI_CONNECTING_TO_AP;
			
			schedule_wifi_sequence_timeout(WIFI_CONNECTING_TIMEOUT);
			
			return true;
		}
		case EVENT_WIFI_SEND:
		case EVENT_WIFI_SEND_TO:
		case EVENT_WIFI_RECEIVE:
		case EVENT_WIFI_RECEIVE_FROM:
		{
			add_wifi_communication_module_event_type(event->type); // retain while connecting
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving wifi connecting state");
			
			schedule_wifi_sequence_timeout(0);
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_connecting_to_ap(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"Entering wifi connecting to AP state");
			
			stopwatch_start();		
			
			send_command_response_string("STATUS CONNECTING_TO_AP;");
			
			if(wifi_communication_module_dependencies.wifi_connect(wifi_ssid, wifi_password, wifi_auth_type))
			{
				LOG(1, "Wifi connecting to AP...");
				
				add_wifi_communication_module_event_type(EVENT_WIFI_CONNECTING);
			}
			else
			{
				LOG(1, "Unable to open connection to AP");
				
				set_wifi_communication_module_error(WIFI_OPERATION_FAILED, state->id);
				
				circular_buffer_clear(&wifi_communication_module_event_buffer);
				
				transition(STATE_WIFI_RESET);
			}
			
			return true;
		}
		case EVENT_WIFI_CONNECTING: // to keep state machine running
		case EVENT_WIFI_IP_ADDRESS_ACQUIRED: // keep alive since it can happen event is received before we got to next state
		{
			add_wifi_communication_module_event_type(event->type); 
			
			return true;
		}
		case EVENT_WIFI_CONNECTED:
		{
			LOG(1,"Wifi connected to AP");
			
			transition(STATE_WIFI_ACQUIRING_IP_ADDRESS);
			
			return true;
		}
		case EVENT_WIFI_DISCONNECTED:
		{
			LOG(1, "Wifi disconnected trying to connect to AP");
			
			set_wifi_communication_module_error(WIFI_DISCONNECTED, state->id);
			
			circular_buffer_clear(&wifi_communication_module_event_buffer);
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_WIFI_SEQUENCE_TIMEOUT:
		{
			LOG(1, "Timeouted while trying to connect to AP");
			
			set_wifi_communication_module_error(WIFI_TIMEOUT, state->id);
			
			circular_buffer_clear(&wifi_communication_module_event_buffer);
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_WIFI_ERROR:
		{
			LOG(1, "Wifi error while trying to connect to AP");
			
			set_wifi_communication_module_error(WIFI_ERROR, state->id);
			
			circular_buffer_clear(&wifi_communication_module_event_buffer);
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving wifi connecting to AP state");
			
			wifi_communication_module_data.connect_to_ap_time += stopwatch_stop(); 
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool verify_static_ip(void)
{
	char current_ip[MAX_WIFI_STATIC_IP_SIZE];
	memset(current_ip, 0, MAX_WIFI_STATIC_IP_SIZE);
	
	wifi_communication_module_dependencies.get_ip_address(current_ip);
	LOG_PRINT(1, PSTR("Current IP: %s\r\n"), current_ip);
	
	LOG_PRINT(1, PSTR("Requested static IP: %s\r\n"), wifi_static_ip);
	
	return strcmp(current_ip, wifi_static_ip) == 0;
}

static bool state_acquiring_ip_address(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering wifi acquiring IP address state");
			
			stopwatch_start();
			
			send_command_response_string("STATUS ACQUIRING_IP_ADDRESS;");
			
			add_wifi_communication_module_event_type(EVENT_WIFI_ACQUIRING_IP_ADDRESS);

			return true;
		}
		case EVENT_WIFI_ACQUIRING_IP_ADDRESS:
		{
			add_wifi_communication_module_event_type(EVENT_WIFI_ACQUIRING_IP_ADDRESS); // to keep state machine running
			
			return true;
		}
		case EVENT_WIFI_IP_ADDRESS_ACQUIRED:
		{
			LOG(1,"Wifi IP address acquired");
			
			if(is_static_ip_set())
			{
				LOG(1,"Verifying static IP");
				
				if(verify_static_ip())
				{
					LOG(1, "Static IP is OK");
					
					transition(STATE_WIFI_CONNECTED);
				}
				else
				{
					LOG(1, "Static IP error");
					
					set_wifi_communication_module_error(WIFI_OPERATION_FAILED, state->id);
					
					circular_buffer_clear(&wifi_communication_module_event_buffer);
					
					transition(STATE_WIFI_RESET);
				}
			}
			else
			{
				transition(STATE_WIFI_CONNECTED);
			}
			
			return true;
		}
		case EVENT_WIFI_DISCONNECTED:
		{
			LOG(1, "Wifi disconnected while acquiring IP address");
			
			set_wifi_communication_module_error(WIFI_DISCONNECTED, state->id);
			
			circular_buffer_clear(&wifi_communication_module_event_buffer);
			
			transition(STATE_WIFI_RESET);
				
			return true;
		}
		case EVENT_WIFI_SEQUENCE_TIMEOUT:
		{
			LOG(1, "Timed out while trying to acquire IP address.");
			
			set_wifi_communication_module_error(WIFI_TIMEOUT, state->id);
			
			circular_buffer_clear(&wifi_communication_module_event_buffer);
			
			transition(STATE_WIFI_RESET);
				
			return true;
		}
		case EVENT_WIFI_ERROR:
		{
			LOG(1, "Wifi module error while trying to acquire IP address");
			
			set_wifi_communication_module_error(WIFI_ERROR, state->id);
			
			circular_buffer_clear(&wifi_communication_module_event_buffer);
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving wifi acquiring IP address state");
			
			wifi_communication_module_data.acquire_ip_address_time = stopwatch_stop(); 
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_connected(state_machine_state_t* state, event_t* event)
{	
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering wifi connected state");
			
			state->current_state = -1;
			
			return true;
		}
		case EVENT_WIFI_SEND:
		{
			LOG(1, "Wifi send received in connected state");

			transition(STATE_WIFI_SEND);

			return true;
		}
		case EVENT_WIFI_SEND_TO:
		{
			LOG(1, "Wifi send to received in connected state");
			
			transition(STATE_WIFI_SEND_TO);

			return true;
		}
		case EVENT_WIFI_RECEIVE:
		{
			LOG(1, "Wifi receive received in connected state");
			
			transition(STATE_WIFI_RECEIVE);

			return true;
		}
		case EVENT_WIFI_RECEIVE_FROM:
		{
			LOG(1, "Wifi receive from received in connected state");
			
			transition(STATE_WIFI_RECEIVE_FROM);

			return true;
		}
		case EVENT_WIFI_CLOSE_SOCKET:
		{
			LOG(1, "Wifi close socket received in connected state");
			
			transition(STATE_WIFI_CLOSING_SOCKET);
			
			return true;
		}
		case EVENT_WIFI_DISCONNECT:
		{
			LOG(1, "Wifi disconnect received in connected state");
			
			add_wifi_communication_module_event_type(EVENT_WIFI_DISCONNECT_SOCKET_CLOSED);
			transition(STATE_WIFI_DISCONNECTING);
			
			return true;
		}
		case EVENT_WIFI_DISCONNECT_SOCKET_CLOSED:
		{
			LOG(1, "Wifi disconnect socket closed");
			
			transition(STATE_WIFI_DISCONNECTING);
			
			return true;
		}
		case EVENT_WIFI_STOP:
		{
			LOG(1, "Wifi stop received in connected state");
			
			add_wifi_communication_module_event_type(EVENT_WIFI_STOP_SOCKET_CLOSED);
			transition(STATE_WIFI_CLOSING_SOCKET);
			
			return true;
		}
		case EVENT_WIFI_STOP_SOCKET_CLOSED:
		{
			LOG(1, "Wifi stop socket closed");
			
			add_wifi_communication_module_event_type(EVENT_WIFI_STOP);
			transition(STATE_WIFI_DISCONNECTING);
			
			return true;
		}
		case EVENT_WIFI_DISCONNECTED:
		{
			LOG(1, "Wifi disconnected while in connected state.");
			
			set_wifi_communication_module_error(WIFI_DISCONNECTED, state->id);
			
			remove_send_and_receive_events();
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_WIFI_ERROR:
		{
			LOG(1, "Wifi error while in connected state.");
			
			set_wifi_communication_module_error(WIFI_ERROR, state->id);
			
			remove_send_and_receive_events();
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving wifi connected state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

wifi_communication_module_error_type_t tcp_to_wifi_error_type(tcp_communication_module_error_type_t tcp_communication_module_error_type)
{
	switch(tcp_communication_module_error_type)
	{
		case TCP_SUCCESS:
		{
			return WIFI_SUCCESS;
		}
		case TCP_OPERATION_FAILED:
		{
			return WIFI_OPERATION_FAILED;
		}
		case TCP_TIMEOUT:
		{
			return WIFI_TIMEOUT;
		}
		case TCP_SOCKET_CLOSED:
		{
			return WIFI_SOCKET_CLOSED;
		}
		case TCP_PARAMETERS_MISSING:
		{
			return WIFI_PARAMETERS_MISSING;
		}
		default:
		{
			return WIFI_SUCCESS; 
		}
	}
}

static bool state_send(state_machine_state_t* state, event_t* event)
{	
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"Entering wifi send state");
			
			stopwatch_start();
			
			communication_module_process_handle = tcp_communication_module_send(data, data_size);
			
			add_wifi_communication_module_event_type(EVENT_WIFI_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_WIFI_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t send_result = get_tcp_communication_module_result();
			
			wifi_communication_module_data.connect_to_server_time = send_result.data.tcp_communication_module_data.connect_to_server_time;
			
			if(is_communication_module_success(&send_result))
			{
				LOG(1, "Wifi sent data succesfully");
				
				transition(STATE_WIFI_CONNECTED);
			}
			else
			{
				LOG(1, "Wifi send data failed");
				
				set_wifi_communication_module_error(tcp_to_wifi_error_type(send_result.data.tcp_communication_module_data.error & 0xf0), state->id);
				
				circular_buffer_clear(&wifi_communication_module_event_buffer);
				
				transition(STATE_WIFI_RESET);
			}
			
			return true;
		}
		case EVENT_WIFI_DISCONNECTED:
		{
			LOG(1, "Wifi disconnected while in send state.");
			
			set_wifi_communication_module_error(WIFI_DISCONNECTED, state->id);
			
			circular_buffer_clear(&wifi_communication_module_event_buffer);
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_WIFI_ERROR:
		{
			LOG(1, "Wifi error while in send state.");
			
			set_wifi_communication_module_error(WIFI_ERROR, state->id);
			
			circular_buffer_clear(&wifi_communication_module_event_buffer);
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving wifi send state");
			
			wifi_communication_module_data.data_exchange_time = stopwatch_stop(); 
				
			return false;
		}
		default:
		{
			return false;
		}
	}
}

wifi_communication_module_error_type_t udp_to_wifi_error_type(udp_communication_module_error_type_t udp_communication_module_error_type)
{
	switch(udp_communication_module_error_type)
	{
		case UDP_SUCCESS:
		{
			return WIFI_SUCCESS;
		}
		case UDP_OPERATION_FAILED:
		{
			return WIFI_OPERATION_FAILED;
		}
		case UDP_PARAMETERS_MISSING:
		{
			return WIFI_PARAMETERS_MISSING;
		}
		default:
		{
			return WIFI_SUCCESS;
		}
	}
}

static bool state_send_to(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"Entering wifi send to state");
			
			stopwatch_start();
			
			communication_module_process_handle = udp_communication_module_send(data, data_size);
			
			add_wifi_communication_module_event_type(EVENT_WIFI_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_WIFI_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t send_result = get_udp_communication_module_result();
			
			if(is_communication_module_success(&send_result))
			{
				LOG(1, "Wifi sent to data succesfully");
				
				transition(STATE_WIFI_CONNECTED);
			}
			else
			{
				LOG(1, "Wifi send to data failed");
				
				set_wifi_communication_module_error(udp_to_wifi_error_type(send_result.data.udp_communication_module_data.error & 0xf0), state->id);
				
				circular_buffer_clear(&wifi_communication_module_event_buffer);
				
				transition(STATE_WIFI_RESET);
			}
			
			return true;
		}
		case EVENT_WIFI_DISCONNECTED:
		{
			LOG(1, "Wifi disconnected while in send to state.");
			
			set_wifi_communication_module_error(WIFI_DISCONNECTED, state->id);
			
			circular_buffer_clear(&wifi_communication_module_event_buffer);
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_WIFI_ERROR:
		{
			LOG(1, "Wifi error while in send to state.");
			
			set_wifi_communication_module_error(WIFI_ERROR, state->id);
			
			circular_buffer_clear(&wifi_communication_module_event_buffer);
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving wifi send to state");
			
			wifi_communication_module_data.data_exchange_time = stopwatch_stop();
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_receive(state_machine_state_t* state, event_t* event)
{		
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"Entering wifi receive state");
			
			stopwatch_start();
			
			communication_module_process_handle = tcp_communication_module_receive(buffer, buffer_size, received_data_size);
			
			add_wifi_communication_module_event_type(EVENT_WIFI_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_WIFI_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t receive_result = get_tcp_communication_module_result();
			
			wifi_communication_module_data.connect_to_server_time = receive_result.data.tcp_communication_module_data.connect_to_server_time;
			
			if(is_communication_module_success(&receive_result))
			{
				if(*received_data_size > 0)
				{
					LOG(1, "Wifi received data");
				}
				else
				{
					LOG(1, "Wifi no data received");
				}
				
				transition(STATE_WIFI_CONNECTED);
			}
			else
			{
				LOG(1, "Wifi receive data failed");
				
				set_wifi_communication_module_error(tcp_to_wifi_error_type(receive_result.data.tcp_communication_module_data.error & 0xf0), state->id);
				
				circular_buffer_clear(&wifi_communication_module_event_buffer);
				
				transition(STATE_WIFI_RESET);
			}
			
			return true;
		}
		case EVENT_WIFI_DISCONNECTED:
		{
			LOG(1, "Wifi disconnected while in receiving state.");
			
			set_wifi_communication_module_error(WIFI_DISCONNECTED, state->id);
			
			circular_buffer_clear(&wifi_communication_module_event_buffer);
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_WIFI_ERROR:
		{
			LOG(1, "Wifi error while in receiving state.");
			
			set_wifi_communication_module_error(WIFI_ERROR, state->id);
			
			circular_buffer_clear(&wifi_communication_module_event_buffer);
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving wifi receive state");
			
			wifi_communication_module_data.data_exchange_time += stopwatch_stop(); 
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_receive_from(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"Entering wifi receive from state");
			
			stopwatch_start();
			
			communication_module_process_handle = udp_communication_module_receive(buffer, buffer_size, received_data_size);
			
			add_wifi_communication_module_event_type(EVENT_WIFI_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_WIFI_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t receive_result = get_udp_communication_module_result();
			
			wifi_communication_module_data.connect_to_server_time = receive_result.data.udp_communication_module_data.connect_to_server_time;
			
			if(is_communication_module_success(&receive_result))
			{
				if(*received_data_size > 0)
				{
					LOG(1, "Wifi received from data");
				}
				else
				{
					LOG(1, "Wifi no data received from");
				}
				
				transition(STATE_WIFI_CONNECTED);
			}
			else
			{
				LOG(1, "Wifi receive from data failed");
				
				set_wifi_communication_module_error(udp_to_wifi_error_type(receive_result.data.udp_communication_module_data.error & 0xf0), state->id);
				
				circular_buffer_clear(&wifi_communication_module_event_buffer);
				
				transition(STATE_WIFI_RESET);
			}
			
			return true;
		}
		case EVENT_WIFI_DISCONNECTED:
		{
			LOG(1, "Wifi disconnected while in receiving from state.");
			
			set_wifi_communication_module_error(WIFI_DISCONNECTED, state->id);
			
			circular_buffer_clear(&wifi_communication_module_event_buffer);
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_WIFI_ERROR:
		{
			LOG(1, "Wifi error while in receiving from state.");
			
			set_wifi_communication_module_error(WIFI_ERROR, state->id);
			
			circular_buffer_clear(&wifi_communication_module_event_buffer);
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving wifi receive from state");
			
			wifi_communication_module_data.data_exchange_time += stopwatch_stop();
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_closing_socket(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering wifi closing socket state");
			
			tcp_process_handle = tcp_communication_module_close_socket();
			udp_process_handle = udp_communication_module_close_socket();
			
			add_wifi_communication_module_event_type(EVENT_WIFI_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_WIFI_COMMUNICATION_MODULE_PROCESS:
		{
			if(tcp_process_handle() || udp_process_handle())
			{
				add_wifi_communication_module_event_type(EVENT_WIFI_COMMUNICATION_MODULE_PROCESS);
			}
			else
			{
				add_wifi_communication_module_event_type(EVENT_WIFI_COMMUNICATION_MODULE_DONE);
			}
			
			return true;
		}
		case EVENT_WIFI_COMMUNICATION_MODULE_DONE:
		{
			LOG(1, "Socket closing finished");
			
			communication_module_type_data_t tcp_close_socket_result = get_tcp_communication_module_result();
			communication_module_type_data_t udp_close_socket_result = get_udp_communication_module_result();
			
			if(is_communication_module_success(&tcp_close_socket_result) && is_communication_module_success(&udp_close_socket_result))
			{
				LOG(1, "Socket closed sucesfully");
				
				transition(STATE_WIFI_CONNECTED);
			}
			else
			{ 
				LOG(1, "Socket could not be closed");
				
				//if socket closed error while we try to close it ignore
				if((tcp_close_socket_result.data.tcp_communication_module_data.error & 0xf0) == TCP_SOCKET_CLOSED)
				{
					LOG(1, "Socket was already closed when we tried to close, ignore error");
					
					transition(STATE_WIFI_CONNECTED);
				}
				else
				{
					set_wifi_communication_module_error(WIFI_OPERATION_FAILED, state->id);
					
					remove_send_and_receive_events();
					
					transition(STATE_WIFI_RESET);
				}

			}
			
			return true;
		}
		case EVENT_WIFI_DISCONNECT_SOCKET_CLOSED:
		case EVENT_WIFI_STOP_SOCKET_CLOSED:
		{
			add_wifi_communication_module_event_type(event->type);
			
			return true;
		}
		case EVENT_WIFI_DISCONNECTED:
		{
			LOG(1, "Wifi disconnected while in closing socket state.");
			
			set_wifi_communication_module_error(WIFI_DISCONNECTED, state->id);
			
			remove_send_and_receive_events();
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_WIFI_ERROR:
		{
			LOG(1, "Wifi error while in closing socket state.");
			
			set_wifi_communication_module_error(WIFI_ERROR, state->id);
			
			remove_send_and_receive_events();
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving wifi closing socket state");
			
			return false;
		}
		default:
		{
			return true;
		}
	}
}

static bool state_disconnecting(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering wifi disconnecting state");

			stopwatch_start();

			send_command_response_string("STATUS DISCONNECTING;");

			schedule_wifi_sequence_timeout(WIFI_DISCONECT_TIMEOUT);

			if(wifi_communication_module_dependencies.wifi_disconnect())
			{
				add_wifi_communication_module_event_type(EVENT_WIFI_DISCONNECTING); // // to keep state machine running
			}
			else
			{
				LOG(1, "Unable to disconnect wifi");
				
				set_wifi_communication_module_error(WIFI_OPERATION_FAILED, state->id);
				
				remove_send_and_receive_events();
				
				transition(STATE_WIFI_RESET);
			}

			return true;
		}
		case EVENT_WIFI_STOP:
		case EVENT_WIFI_DISCONNECTING:
		{
			add_wifi_communication_module_event_type(event->type);
			
			return true;
		}
		case EVENT_WIFI_DISCONNECTED:
		{
			LOG(1, "Wifi disconnected");
			
			transition(STATE_WIFI_DISCONNECTED);
			
			return true;
		}
		case EVENT_WIFI_SEQUENCE_TIMEOUT:
		{
			LOG(1, "Wifi timeout while trying disconnect from AP.");
			
			set_wifi_communication_module_error(WIFI_TIMEOUT, state->id);
			
			remove_send_and_receive_events();
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_WIFI_ERROR:
		{
			LOG(1, "Wifi error while disconnecting from AP");
			
			set_wifi_communication_module_error(WIFI_ERROR, state->id);
			
			remove_send_and_receive_events();
			
			transition(STATE_WIFI_RESET);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving wifi disconnecting state");

			schedule_wifi_sequence_timeout(0);

			wifi_communication_module_data.disconnect_time = stopwatch_stop();
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_stopping(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering wifi stopping state");
	
			if(!wifi_communication_module_dependencies.wifi_stop())
			{
				LOG(1, "Unable to stop wifi, what to do now?");
				
				set_wifi_communication_module_error(WIFI_OPERATION_FAILED, state->id);
			}
			
			transition(STATE_WIFI_STOPPED);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving wifi stop state");

			return false;
		}
		default:
		{
			return false;
		}
	}
}

communication_module_type_data_t get_wifi_communication_result(void)
{
	communication_module_type_data_t communication_module_type_data;
	communication_module_type_data.type = COMMUNICATION_MODULE_WIFI;
	memcpy(&communication_module_type_data.data.wifi_communication_module_data, &wifi_communication_module_data, sizeof(wifi_communication_module_data_t));
	return communication_module_type_data;
}

void get_wifi_communication_module_status(char* status, uint16_t status_length)
{
	state_machine_state_t* state = &wifi_communication_module_state_machine;
	while(state->current_state != -1)
	{
		state = &wifi_communication_module_states[state->current_state];
	}
	
	get_state_human_readable_name(wifi_communication_module_states, state, status, status_length);
}

