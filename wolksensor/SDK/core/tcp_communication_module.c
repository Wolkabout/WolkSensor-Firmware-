#include "tcp_communication_module.h"
#include "tcp_communication_module_dependencies.h"
#include "state_machine.h"
#include "logger.h"
#include "event_buffer.h"
#include "config.h"
#include "global_dependencies.h"
#include "state_machine.h"
#include "commands_dependencies.h"

#define EVENTS_BUFFER_SIZE 10
#define OPEN_SOCKET_MAX_RETRIES 3
#define RECEIVE_TIMEOUT 3

typedef enum
{
	STATE_SOCKET_CLOSED = 0,
	STATE_OPENING_SOCKET,
	STATE_SOCKET_OPENED,
		STATE_SEND,
		STATE_RECEIVE,
	STATE_CLOSING_SOCKET,
}
tcp_communication_module_states_t;

typedef enum
{
	EVENT_SEND = 0,
	EVENT_RECEIVE,
	EVENT_OPEN_SOCKET,
	EVENT_CLOSE_SOCKET,
	EVENT_SOCKET_CLOSED,
	EVENT_TIMEOUT
}
tcp_communication_module_events_t;

tcp_communication_module_dependencies_t tcp_communication_module_dependencies;

static state_machine_state_t state_machine;
static state_machine_state_t states[6];

static circular_buffer_t events_buffer;
static event_t events_buffer_storage[EVENTS_BUFFER_SIZE];

static uint8_t* data = NULL;
static uint16_t data_size = 0;

static uint8_t* buffer = NULL;
static uint16_t buffer_size = 0;
static uint16_t* received_data_size = NULL;

static int open_socket_id = -1;

static volatile uint8_t timeout_timer = 0;

static tcp_communication_module_data_t tcp_communication_module_data;

static uint32_t platform_specific_error_code = 0;

static volatile uint16_t tick_counter = 0;

// forward declaration of state machine state handlers
static bool tcp_communication_module_handler(state_machine_state_t* state, event_t* event);

static bool state_socket_closed(state_machine_state_t* state, event_t* event);
static bool state_opening_socket(state_machine_state_t* state, event_t* event);
static bool state_socket_opened(state_machine_state_t* state, event_t* event);
static bool state_send(state_machine_state_t* state, event_t* event);
static bool state_receive(state_machine_state_t* state, event_t* event);
static bool state_closing_socket(state_machine_state_t* state, event_t* event);

static void init_events_buffer(void)
{
	circular_buffer_init(&events_buffer, events_buffer_storage, EVENTS_BUFFER_SIZE, sizeof(event_t), true, true);
}

static bool process_event(void)
{
	event_t event;
	if(pop_event(&events_buffer, &event))
	{
		state_machine_process_event(states, &state_machine, &event);
		return true;
	}
	
	return false;
}

static void init_state(tcp_communication_module_states_t id, const char* human_readable_name, state_machine_state_t* parent, int8_t initial_state, state_machine_state_handler handler)
{
	state_machine_init_state(id, human_readable_name, parent, states, initial_state, handler);
}

static void transition(tcp_communication_module_states_t new_state_id)
{
	state_machine_transition(states, &state_machine, new_state_id);
}

static void send_command_response_string(const char* response)
{
	global_dependencies.send_response(response, strlen(response));
}

static void load_parameters(void)
{
	LOG(1, "Load connection parameters from config");

	load_server_ip();
	load_server_port();
	load_ssl_status();
}

static void schedule_timeout(uint16_t period)
{
	timeout_timer = period;
}

static void second_expired_listener(void)
{
	if (timeout_timer && (--timeout_timer == 0))
	{
		LOG(1, "TCP timeout");

		add_event_type(&events_buffer, EVENT_TIMEOUT);
	}
}

static void millisecond_expired_listener(void)
{
	if(tick_counter) tick_counter++;
}

static void socket_closed_listener(void)
{
	LOG(1, "TCP socket closed");

	add_event_type(&events_buffer, EVENT_SOCKET_CLOSED);
}

static void platform_specific_error_code_listener(uint32_t error_code)
{
	platform_specific_error_code = error_code;
}

static void set_tcp_communication_module_error(tcp_communication_module_error_type_t error_type, uint8_t state)
{
	if(!tcp_communication_module_data.error)
	{
		tcp_communication_module_data.error = error_type | state;
		tcp_communication_module_data.platform_specific_error_code = platform_specific_error_code;
	}
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

void init_tcp_communication_module(void)
{
	LOG(1, "TCP communication module init");

	// dependencies	
	tcp_communication_module_dependencies.add_second_expired_listener(second_expired_listener);
	tcp_communication_module_dependencies.add_milisecond_expired_listener(millisecond_expired_listener);
	tcp_communication_module_dependencies.add_socket_closed_listener(socket_closed_listener);
	tcp_communication_module_dependencies.add_platform_specific_error_code_listener(platform_specific_error_code_listener);

	// plug into commands
	commands_dependencies.communication_module_close_socket = tcp_communication_module_close_socket;
		
	// buffers
	init_events_buffer();
	
	// parameters
	load_parameters();
	
	// state machine
	state_machine.id = -1;
	state_machine.human_readable_name = NULL;
	state_machine.parent = NULL;
	state_machine.current_state = STATE_SOCKET_CLOSED;
	state_machine.handler = tcp_communication_module_handler;
	
	init_state(STATE_SOCKET_CLOSED, PSTR("SOCKED_CLOSED"),  &state_machine, -1, state_socket_closed);
	init_state(STATE_OPENING_SOCKET, PSTR("OPENEING_SOCKET"),  &state_machine, -1, state_opening_socket);
	init_state(STATE_SOCKET_OPENED, PSTR("SOCKET_OPENED"),  &state_machine, -1, state_socket_opened);
		init_state(STATE_SEND, PSTR("SEND"), &states[STATE_SOCKET_OPENED], -1, state_send);
		init_state(STATE_RECEIVE, PSTR("RECEIVE"), &states[STATE_SOCKET_OPENED], -1, state_receive);
	init_state(STATE_CLOSING_SOCKET, PSTR("CLOSING_SOCKET"),  &state_machine, -1, state_closing_socket);

	transition(STATE_SOCKET_CLOSED);
}

communication_module_process_handle_t tcp_communication_module_send(uint8_t* data_in, uint16_t data_in_size)
{
	LOG(1, "TCP send");
	
	data = data_in;
	data_size = data_in_size;

	memset(&tcp_communication_module_data, 0, sizeof(tcp_communication_module_data_t));
	
	add_event_type(&events_buffer, EVENT_SEND);
	
	return process_event;
}

communication_module_process_handle_t tcp_communication_module_receive(uint8_t* buffer_out, uint16_t buffer_out_size, uint16_t* received_data_size_out)
{
	LOG(1, "TCP receive");

	buffer = buffer_out;
	buffer_size = buffer_out_size;
	received_data_size = received_data_size_out;
	
	memset(&tcp_communication_module_data, 0, sizeof(tcp_communication_module_data_t));
	
	add_event_type(&events_buffer, EVENT_RECEIVE);
	
	return process_event;
}

communication_module_process_handle_t tcp_communication_module_close_socket(void)
{
	LOG(1, "TCP close socket");
	
	memset(&tcp_communication_module_data, 0, sizeof(tcp_communication_module_data_t));

	add_event_type(&events_buffer, EVENT_CLOSE_SOCKET);
	
	return process_event;
}

static bool tcp_communication_module_handler(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"Entering TCP communication module");
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving TCP communication module");
			
			return false;
		}
		default:
		{
			return true;
		}
	}
}

static bool connection_parameters_set(void)
{
	return *server_ip != 0 && server_port != 0;
}

static bool state_socket_closed(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"Entering TCP socket closed state");

			return true;
		}
		case EVENT_SEND:
		{
			LOG(1, "Send in TCP socket closed state");

			if(connection_parameters_set())
			{
				add_event_type(&events_buffer, EVENT_SEND);
				transition(STATE_OPENING_SOCKET);
			}
			else
			{
				LOG(1, "TCP connection parameters missing");
				set_tcp_communication_module_error(TCP_PARAMETERS_MISSING, state->id);
			}

			return true;
		}
		case EVENT_RECEIVE:
		{
			LOG(1,"Receive in TCP socket closed state");

			if(connection_parameters_set())
			{
				add_event_type(&events_buffer, EVENT_RECEIVE);
				transition(STATE_OPENING_SOCKET);
			}
			else
			{
				LOG(1, "TCP connection parameters missing");
				set_tcp_communication_module_error(TCP_PARAMETERS_MISSING, state->id);
			}

			return true;
		}
		case EVENT_CLOSE_SOCKET:
		{
			LOG(1,"TCP socket already closed");
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving TCP socket closed state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_opening_socket(state_machine_state_t* state, event_t* event)
{
	static uint8_t open_socket_retries;
	
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"TCP opening socket state");

			send_command_response_string("STATUS CONNECTING_TO_SERVER;");
			
			stopwatch_start();

			open_socket_retries = 0;
			
			add_event_type(&events_buffer, EVENT_OPEN_SOCKET);

			return true;
		}
		case EVENT_OPEN_SOCKET:
		{
			LOG(1, "TCP opening socket");
			
			if((open_socket_id = tcp_communication_module_dependencies.open_socket(server_ip, server_port, ssl)) >= 0)
			{
				LOG(1, "TCP socket opened");
				
				transition(STATE_SOCKET_OPENED);
			}
			else
			{
				if(++open_socket_retries == OPEN_SOCKET_MAX_RETRIES)
				{
					LOG_PRINT(1, PSTR("TCP unable to open socket after %u retries\r\n"), open_socket_retries);
					
					set_tcp_communication_module_error(TCP_OPERATION_FAILED, state->id);
					
					circular_buffer_clear(&events_buffer);
					
					transition(STATE_SOCKET_CLOSED);
				}
				else // Retry
				{
					LOG(1,"TCP unable to open socket, retrying");
					
					add_event_type(&events_buffer, EVENT_OPEN_SOCKET);
				}
			}

			return true;
		}
		case EVENT_SEND:
		case EVENT_RECEIVE:
		{
			add_event_type(&events_buffer, event->type); // retain while opening socket

			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving TCP opening socket state");
			
			tcp_communication_module_data.connect_to_server_time = stopwatch_stop();

			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_socket_opened(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"Entering TCP socket opened state");

			state->current_state = -1;
			
			return true;
		}
		case EVENT_SEND:
		{
			LOG(1, "Send received in TCP socket opened state");

			transition(STATE_SEND);

			return true;
		}
		case EVENT_RECEIVE:
		{
			LOG(1, "Receive received in TCP socket opened state");
			
			transition(STATE_RECEIVE);

			return true;
		}
		case EVENT_CLOSE_SOCKET:
		{
			LOG(1, "Close socket received in TCP socket opened state");
			
			transition(STATE_CLOSING_SOCKET);
			
			return true;
		}
		case EVENT_SOCKET_CLOSED:
		{
			LOG(1, "Socket closed while in TCP socket opened state");
			
			set_tcp_communication_module_error(TCP_SOCKET_CLOSED, state->id);
			
			circular_buffer_clear(&events_buffer);
			
			transition(STATE_SOCKET_CLOSED);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving TCP socket opened state");
			
			return true;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_send(state_machine_state_t* state, event_t* event)
{	
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"Entering TCP send state");
			
			stopwatch_start();
			
			if(tcp_communication_module_dependencies.send(open_socket_id, data, data_size) == data_size)
			{
				LOG(1, "TCP sent data successfully");
				
				transition(STATE_SOCKET_OPENED);
			}
			else
			{
				LOG(1, "TCP send data failed");
				
				set_tcp_communication_module_error(TCP_OPERATION_FAILED, state->id);
				
				circular_buffer_clear(&events_buffer);
				
				transition(STATE_CLOSING_SOCKET);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving TCP send state");
			
			tcp_communication_module_data.data_exchange_time = stopwatch_stop();
				
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
			LOG(1,"Entering TCP receive state");
			
			stopwatch_start();
			
			schedule_timeout(RECEIVE_TIMEOUT);
			
			add_event_type(&events_buffer, EVENT_RECEIVE);
			
			return true;
		}
		case EVENT_RECEIVE:
		{
			int received = tcp_communication_module_dependencies.receive(open_socket_id, buffer, buffer_size);
			if(received > 0)
			{
				LOG(1, "TCP received data");

				*received_data_size = received;
				
				transition(STATE_SOCKET_OPENED);
			}
			else if(received == 0)
			{
				*received_data_size = 0;
				
				// try again
				add_event_type(&events_buffer, EVENT_RECEIVE);
			}
			else
			{
				LOG(1, "TCP receive data failed");
				
				*received_data_size = 0;
				set_tcp_communication_module_error(TCP_OPERATION_FAILED, state->id);

				circular_buffer_clear(&events_buffer);
				
				transition(STATE_SOCKET_CLOSED);
			}
			
			return true;
		}
		case EVENT_SOCKET_CLOSED:
		{
			LOG(1, "Socket closed while in TCP send state");

			set_tcp_communication_module_error(TCP_SOCKET_CLOSED, state->id);

			circular_buffer_clear(&events_buffer);

			transition(STATE_SOCKET_CLOSED);

			return true;
		}
		case EVENT_TIMEOUT:
		{
			LOG(1, "Timeout while trying to receive data.");
			
			transition(STATE_SOCKET_OPENED);
			
			circular_buffer_clear(&events_buffer);

			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving TCP receive state");
			
			schedule_timeout(0);
			
			tcp_communication_module_data.data_exchange_time = stopwatch_stop();
			
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
			LOG(1, "Entering TCP closing socket state");
			
			stopwatch_start();

			if(tcp_communication_module_dependencies.close_socket(open_socket_id))
			{
				LOG(1, "TCP socket closed successfully");
				
				open_socket_id = -1;
				
				transition(STATE_SOCKET_CLOSED);
			}
			else
			{
				LOG(1, "TCP socket could not be closed");
				
				set_tcp_communication_module_error(TCP_OPERATION_FAILED, state->id);
				
				circular_buffer_clear(&events_buffer);
				
				transition(STATE_SOCKET_CLOSED);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving TCP closing socket state");
			
			tcp_communication_module_data.disconnect_time = stopwatch_stop();
			
			return false;
		}
		default:
		{
			return true;
		}
	}
}

communication_module_type_data_t get_tcp_communication_module_result(void)
{
	communication_module_type_data_t communication_module_type_data;
	communication_module_type_data.type = COMMUNICATION_MODULE_TCP;
	memcpy(&communication_module_type_data.data.tcp_communication_module_data, &tcp_communication_module_data, sizeof(tcp_communication_module_data_t));

	return communication_module_type_data;
}

void get_tcp_communication_module_status(char* status, uint16_t status_length)
{
	state_machine_state_t* state = &state_machine;
	while(state->current_state != -1)
	{
		state = &states[state->current_state];
	}
	
	get_state_human_readable_name(states, state, status, status_length);
}
