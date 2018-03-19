#include "wolksensor.h"
#include "logger.h"
#include "sensors.h"
#include "command_parser.h"
#include "commands.h"
#include "commands_dependencies.h"
#include "chrono.h"
#include "event_buffer.h"
#include "wolksensor_dependencies.h"
#include "system_buffer.h"
#include "communication_module.h"
#include "communication_protocol.h"
#include "system.h"
#include "config.h"
#include "protocol.h"
#include "sensor_readings_buffer.h"
#include "global_dependencies.h"
#include "sensors.h"


typedef enum
{
	STATE_IDLE = 0,
		STATE_BROWNOUT,
		STATE_NORMAL,
	STATE_ACQUISITION,
	STATE_DATA_EXCHANGE,
		STATE_SEND,
		STATE_RECEIVE,
		STATE_DISCONNECT,
		STATE_STOP_COMMUNICATION_MODULE
}
wolksensor_states_t;

typedef enum
{
	EVENT_MOVEMENT = 0,
	EVENT_HEARTBEAT,
	EVENT_ACQUIRE,
	EVENT_COMMAND_RECEIVED,
	EVENT_USB_CONNECTED,
	EVENT_USB_DISCONNECTED,
	EVENT_ALARM,
	EVENT_COMMUNICATION_PROTOCOL_PROCESS,
	EVENT_COMMUNICATION_PROTOCOL_DONE,
	EVENT_RESET
}
wolksensor_events_t;

static state_machine_state_t state_machine;
static state_machine_state_t states[9];

static circular_buffer_t events_buffer;
static event_t events_buffer_storage[EVENTS_BUFFER_SIZE];

static circular_buffer_t command_string_buffer;
static char command_string_buffer_storage[COMMAND_STRING_BUFFER_SIZE];

static circular_buffer_t commands_buffer;
static command_t commands_buffer_storage[COMMANDS_BUFFER_SIZE];

static circular_buffer_t command_response_buffer;
static char command_response_buffer_storage[COMMAND_RESPONSE_BUFFER_SIZE];

static uint16_t current_heartbeat = 0;
static uint16_t	heartbeat_timer = 0;

static uint16_t no_connection_heartbeat = 10;
static uint8_t no_connection_count = 0;

static bool brownout = false;

static uint8_t sound_alarm_retries = 0;

static uint16_t sent_system_items = 0;
static uint16_t sent_sensor_readings = 0;

static uint16_t battery_voltage = 0;

static communication_and_battery_data_t communication_and_battery_data;

static communication_protocol_process_handle_t communication_protocol_process_handle;

wolksensor_dependencies_t wolksensor_dependencies; 

static bool wolksensor_handler(state_machine_state_t* state, event_t* event);
static bool state_idle(state_machine_state_t* state, event_t* event);
static bool state_brownout(state_machine_state_t* state, event_t* event);
static bool state_normal(state_machine_state_t* state, event_t* event);
static bool state_acquisition(state_machine_state_t* state, event_t* event);
static bool state_data_exchange(state_machine_state_t* state, event_t* event);
static bool state_send(state_machine_state_t* state, event_t* event);
static bool state_receive(state_machine_state_t* state, event_t* event);
static bool state_disconnect(state_machine_state_t* state, event_t* event);
static bool state_stop_communication_module(state_machine_state_t* state, event_t* event);

static void init_state(wolksensor_states_t id, const char* human_readable_name, state_machine_state_t* parent, uint8_t initial_state, state_machine_state_handler handler)
{
	state_machine_init_state(id, human_readable_name, parent, states, initial_state, handler);
}

static void transition(wolksensor_states_t new_state_id)
{
	state_machine_transition(states, &state_machine, new_state_id);
}

static void init_events_buffer(void)
{
	circular_buffer_init(&events_buffer, events_buffer_storage, EVENTS_BUFFER_SIZE, sizeof(event_t), true, true);
}

void init_commands_buffer(void)
{
	circular_buffer_init(&commands_buffer, commands_buffer_storage, COMMANDS_BUFFER_SIZE, sizeof(command_t), true, true);
}

static void init_command_string_buffer(void)
{
	circular_buffer_init(&command_string_buffer, command_string_buffer_storage, COMMAND_STRING_BUFFER_SIZE, sizeof(char), true, true);
}

static void init_command_response_buffer(void)
{
	circular_buffer_init(&command_response_buffer, command_response_buffer_storage, COMMAND_RESPONSE_BUFFER_SIZE, sizeof(char), false, true);
}

static void command_data_listener(char *data, uint16_t length)
{
	circular_buffer_add_array(&command_string_buffer, data, length);
	
	uint16_t i;
	for(i = 0; i < length; i++)
	{
		if(data[i] == COMMAND_TERMINATOR)
		{
			add_event_type(&events_buffer, EVENT_COMMAND_RECEIVED);
		}
	}
}

static void send_command_response_string(const char* response)
{
	global_dependencies.send_response(response, strlen(response));
}

static bool process_wolksensor_event(void)
{
	event_t event;
	if(pop_event(&events_buffer, &event))
	{
		state_machine_process_event(states, &state_machine, &event);
		return true;
	}
	
	return false;
}

bool wolksensor_process(void)
{
	return process_wolksensor_event();
}

static void minute_expired_listener(void)
{
	add_event_type(&events_buffer, EVENT_ACQUIRE);
	
	if (current_heartbeat)
	{
		heartbeat_timer++;
		LOG_PRINT(2, PSTR("Heartbeat timer %u\r\n"), heartbeat_timer);
		
		if (heartbeat_timer == current_heartbeat)
		{
			LOG(1, "Heartbeat!");
			add_event_type(&events_buffer, EVENT_HEARTBEAT);
			heartbeat_timer = 0;
		}
	}
}

static void start_heartbeat(uint16_t period)
{
	if(current_heartbeat == period)
	{
		LOG_PRINT(1, PSTR("Heartbeat is %u already\r\n"), period);
	}
	else
	{
		LOG_PRINT(1, PSTR("Staring heartbeat %u\r\n"), period);
		
		LOG_PRINT(1, PSTR("Old heartbeat timer %u\r\n"), heartbeat_timer);
		
		if(heartbeat_timer != 0)
		{
			heartbeat_timer = period - heartbeat_timer % period;
		}
		
		LOG_PRINT(1, PSTR("New heartbeat timer %u\r\n"), heartbeat_timer);
		
		current_heartbeat = period;
	}
}

static void exchange_data(void)
{
	add_event_type(&events_buffer, EVENT_HEARTBEAT);
}

static void acquisition(void)
{
	add_event_type(&events_buffer, EVENT_ACQUIRE);
}

static void reset(void)
{
	add_event_type(&events_buffer, EVENT_RESET);
}

static void get_status(char* status, uint16_t status_length)
{
	if(state_machine.current_state == STATE_DATA_EXCHANGE)
	{
		communication_module.get_status(status, status_length);	
	}
	else
	{
		state_machine_state_t* state = &state_machine;
		while(state->current_state != -1)
		{
			state = &states[state->current_state];
		}
		
		get_state_human_readable_name(states, state, status, status_length);
	}
}

static void process_alarms(sensor_state_t* sensors_states, uint8_t sensors_count)
{
	bool new_alarms_present = check_sensor_alarm_updates(sensors_states, sensors_count);
	if(new_alarms_present)
	{
		LOG(1, "Sounding new alarms");
		sound_alarm_retries = 0;
		add_event_type(&events_buffer, EVENT_ALARM);
	}
	else if(sensors_have_unsounded_alarms() && (sound_alarm_retries < MAX_ALARM_RETRIES))
	{
		LOG(1, "Retry sounding old alarms");
		sound_alarm_retries++;
		add_event_type(&events_buffer, EVENT_ALARM);
	}
}

static void sensors_states_listener(sensor_state_t* sensors_states, uint8_t sensors_count)
{
	LOG_PRINT(1, PSTR("Received %u sensor states\r\n"), sensors_count);
	
	float sensors_values[NUMBER_OF_SENSORS];
	
	uint8_t i;
	for(i = 0; i < NUMBER_OF_SENSORS; i++)
	{
		sensors_values[i] = SENSOR_VALUE_NOT_SET;
	}
	
	for(i = 0; i < sensors_count; i++)
	{
		int8_t index_od_sensor = get_index_of_sensor(sensors_states[i].id);
		if(index_od_sensor < 0)
		{
			LOG_PRINT(1, PSTR("Unknown sensor state received %c\r\n"), sensors_states[i].id);
			continue;
		}
		
		sensors_values[index_od_sensor] = sensors_states[i].value;
	}

	// do not save unless at least one sensor value is set
	for(i = 0; i < NUMBER_OF_SENSORS; i++)
	{
		if(sensors_values[i] != SENSOR_VALUE_NOT_SET)
		{
			store_sensor_readings(sensors_values);
			process_alarms(sensors_states, sensors_count);
			break;
		}
	}
}

static void battery_voltage_listener(uint16_t voltage)
{
	if((battery_voltage == 0) || (voltage < battery_voltage))
	{
		battery_voltage = voltage;
	}
}

static void usb_state_change_listener(bool usb_state)
{
	if(usb_state)
	{
		LOG(1, "USB connected");
		
		add_event_type(&events_buffer, EVENT_USB_CONNECTED);
	}
	else
	{
		LOG(1, "USB disconnected");
		
		add_event_type(&events_buffer, EVENT_USB_DISCONNECTED);
	}
}

void init_wolksensor(start_type_t start_type)
{
	LOG(1, "Wolksensor init");
	
	// dependencies
	wolksensor_dependencies.add_minute_expired_listener(minute_expired_listener);
	wolksensor_dependencies.add_usb_state_change_listener(usb_state_change_listener);
	wolksensor_dependencies.add_command_data_received_listener(command_data_listener);
	wolksensor_dependencies.add_battery_voltage_listener(battery_voltage_listener);
	wolksensor_dependencies.add_sensors_states_listener(sensors_states_listener);
	
	// plug into commands
	commands_dependencies.exchange_data  = exchange_data;
	commands_dependencies.acquisition = acquisition;
	commands_dependencies.reset = reset;
	commands_dependencies.start_heartbeat = start_heartbeat;
	commands_dependencies.get_application_status = get_status;
	
	// parameters
	load_system_heartbeat();
	load_atmo_status();

	load_offset_status();
	load_offset_factory_status();
	
	load_movement_status();
	// for movement sensor set predefined alarm
	sensors_init();
	uint8_t movement_sensor_index = get_index_of_sensor('M');
	if(movement_sensor_index >= 0)
	{
		sensors_alarms[movement_sensor_index].alarm_high.enabled = true;
		sensors_alarms[movement_sensor_index].alarm_high.value = 1;
	}
	
	// buffers
	init_sensor_readings_buffer(start_type == POWER_ON);
	init_system_buffer(start_type == POWER_ON);
		
	init_events_buffer();
	init_commands_buffer();
	init_command_string_buffer();
	init_command_response_buffer();
	
	state_machine.id = -1;
	state_machine.human_readable_name = NULL;
	state_machine.parent = NULL;
	state_machine.current_state = STATE_IDLE;
	state_machine.handler = wolksensor_handler;
	 
	init_state(STATE_IDLE, NULL, &state_machine, start_type == BROWNOUT_RESET ? STATE_BROWNOUT : STATE_NORMAL, state_idle);
		init_state(STATE_BROWNOUT, PSTR("BROWNOUT"), &states[STATE_IDLE], -1, state_brownout);
		init_state(STATE_NORMAL, PSTR("IDLE"), &states[STATE_IDLE], -1, state_normal);
	init_state(STATE_ACQUISITION, PSTR("ACQUISITION"), &state_machine, -1, state_acquisition);
	init_state(STATE_DATA_EXCHANGE, NULL, &state_machine, STATE_SEND, state_data_exchange);
		init_state(STATE_SEND, NULL, &states[STATE_DATA_EXCHANGE], -1, state_send);
		init_state(STATE_RECEIVE, NULL, &states[STATE_DATA_EXCHANGE], -1, state_receive);
		init_state(STATE_DISCONNECT, NULL, &states[STATE_DATA_EXCHANGE], -1, state_disconnect);
		init_state(STATE_STOP_COMMUNICATION_MODULE, NULL, &states[STATE_DATA_EXCHANGE], -1, state_stop_communication_module);
	
	chrono_init(start_type == POWER_ON);
	
	if(brownout)
	{
		LOG(1, "Scheduling system heartbeat");
		start_heartbeat(system_heartbeat);
	}
	else
	{
		LOG(1, "Scheduling heartbeat in minute");
		start_heartbeat(1);
	}
	
	transition(STATE_IDLE);
	
	add_event_type(&events_buffer, EVENT_ACQUIRE);
}

static bool wolksensor_handler(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE: 
		{
			LOG(1, "Entering wolksensor state machine");
			
			return true;
		}
		case EVENT_USB_CONNECTED:
		{
			LOG(1, "Usb ON, clearing no connections, starting USB heartbeat");
					
			no_connection_count = 0;
			no_connection_heartbeat = system_heartbeat;
					
			start_heartbeat(system_heartbeat);
					
			circular_buffer_clear(&command_string_buffer); // TODO Reconsider...
					
			return true;
		}
		case EVENT_USB_DISCONNECTED:
		{
			LOG(1, "Usb OFF");

			return true;
		}
		case EVENT_ACQUIRE:
		{
			add_event_type(&events_buffer, EVENT_ACQUIRE);
		
			return true;
		}
		case EVENT_HEARTBEAT:
		{
			add_event_type(&events_buffer, EVENT_HEARTBEAT);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			return false;
		}
		default:
		{
			return true;
		}
	}
}

static void execute_commands(bool allow_write)
{
	command_t command;
	while(circular_buffer_pop(&commands_buffer, &command))
	{
		if(!allow_write && (command.has_argument || command.type == COMMAND_RELOAD))
		{
			circular_buffer_clear(&command_response_buffer);
			append_busy(&command_response_buffer);
			global_dependencies.send_response(command_response_buffer.storage, circular_buffer_size(&command_response_buffer));
		}
		else
		{
			command_execution_result_t command_execution_result;
			do
			{
				circular_buffer_clear(&command_response_buffer);
				command_execution_result = execute_command(&command, &command_response_buffer);
				global_dependencies.send_response(command_response_buffer.storage, circular_buffer_size(&command_response_buffer));
				
			}
			while (command_execution_result == COMMAND_EXECUTED_PARTIALLY);
		}
	}
}

static bool state_idle(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering idle state"); 
			
			state->current_state = brownout ? STATE_BROWNOUT : STATE_NORMAL;
			
			return true;
		}
		case EVENT_ACQUIRE:
		{
			LOG(1, "Acquire event occured in idle state");
			
			LOG_PRINT(2, PSTR("Atmo sensor status %u\r\n"), atmo_status);
			
			if(atmo_status && !sensor_readings_buffer_full())
			{
				transition(STATE_ACQUISITION);
			}

			return true;
		}
		case EVENT_COMMAND_RECEIVED:
		{
			LOG(1, "Command received in idle state");
			
			extract_commands_from_string_buffer(&command_string_buffer, &commands_buffer);
			
			execute_commands(true);
			
			return true;
		}
		case EVENT_RESET:
		{
			wolksensor_dependencies.system_reset();
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving idle state"); 
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_normal(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering normal state");
			 
			send_command_response_string("STATUS IDLE;");
			
			brownout = false;
			
			return true;
		}
		case EVENT_ALARM:
		{
			LOG(1, "Alarm occured in normal state");
			
			transition(STATE_DATA_EXCHANGE);
			
			return true;
		}
		case EVENT_HEARTBEAT:
		{
			LOG(1,"Heartbeat occured in normal state");
 	 		// HAMMERTIME!
			transition(STATE_DATA_EXCHANGE);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving normal state"); 
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_brownout(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering brownout state"); 
			
			send_command_response_string("STATUS BROWNOUT;");
			
			brownout = true;
			
			return true;
		}
		case EVENT_ALARM:
		case EVENT_HEARTBEAT:
		{
			// DO NOT HANDLE WE ARE LOW ON POWER!
			return true;
		}
		case EVENT_USB_CONNECTED:
		{
			LOG(1, "Usb ON, setting idle state to normal");
			
			transition(STATE_NORMAL);
			
			return false;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving brownout state"); 
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_acquisition(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering acquisition state"); 
			
			send_command_response_string("STATUS ACQUISITION;");
			
			// find all value on demand sensors sensors
			char value_on_demand_sensors_ids[NUMBER_OF_SENSORS];
			uint8_t value_on_demand_sensors_count = get_value_on_demand_sensors_ids(value_on_demand_sensors_ids);
			
			if(wolksensor_dependencies.get_sensors_states(value_on_demand_sensors_ids, value_on_demand_sensors_count))
			{
				LOG(1, "Sensors values read");
			}
			else
			{
				LOG(1, "Unable to read sensors values");
			}

			transition(STATE_IDLE);

			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving acquisition state"); 
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static void adjust_heartbeat_on_error(void)
{
	if(wolksensor_dependencies.get_usb_state())
	{
		start_heartbeat(system_heartbeat); 
		return;
	}
	
	if(sensors_have_unsounded_alarms() && (sound_alarm_retries <= MAX_ALARM_RETRIES))
	{
		LOG(1, "Sounding alarms, do not change heartbeat");
		return; // if we are handling alarms
	}
	
	LOG(1, "Doing no connection heartbeat")
	if(no_connection_count == 0)
	{
		no_connection_heartbeat = system_heartbeat;
	}
	
	no_connection_count++;
	LOG_PRINT(2, PSTR("No connection count %u\r\n"), no_connection_count);
	
	if((no_connection_count % 2 == 0) && (no_connection_heartbeat < MAX_NO_CONNECTION_HEARTBEAT))
	{
		uint16_t new_no_connection_heartbeat = no_connection_heartbeat * 2;
		no_connection_heartbeat = new_no_connection_heartbeat < MAX_NO_CONNECTION_HEARTBEAT ?  new_no_connection_heartbeat : MAX_NO_CONNECTION_HEARTBEAT;
		
		LOG_PRINT(2, PSTR("Start no connecion heartbeat %u\r\n"), no_connection_heartbeat)
		start_heartbeat(no_connection_heartbeat);
	}
}

static bool state_data_exchange(state_machine_state_t* state, event_t* event)
{	
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"Entering data exchange state");
			
			state->current_state = STATE_SEND;
			
			memset(&communication_and_battery_data, 0, sizeof(communication_and_battery_data_t));
			
			wolksensor_dependencies.enable_battery_voltage_monitor();
			
			return true;
		}
		case EVENT_COMMAND_RECEIVED:
		{
			LOG(1, "Command received in data exchange state");
			
			extract_commands_from_string_buffer(&command_string_buffer, &commands_buffer);
			
			execute_commands(false);
			
			return true;
		}
		case EVENT_COMMUNICATION_PROTOCOL_PROCESS:
		{
			if(communication_protocol_process_handle())
			{
				add_event_type(&events_buffer, EVENT_COMMUNICATION_PROTOCOL_PROCESS);
			}
			else
			{
				add_event_type(&events_buffer, EVENT_COMMUNICATION_PROTOCOL_DONE);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			
			LOG(1,"Leaving data exchange state");
			
			wolksensor_dependencies.disable_battery_voltage_monitor();
						
			communication_and_battery_data.battery_min_voltage = battery_voltage;
			add_communication_and_battery_data(&communication_and_battery_data);
			LOG_PRINT(1, PSTR("\n\rBattery Voltage: %d\n\r"), battery_voltage);
						
			if(is_communication_protocol_success(&communication_and_battery_data.communication_protocol_type_data))
			{
				clear_sounded_alarms();
				
				no_connection_count = 0;
				no_connection_heartbeat = system_heartbeat;
				
				start_heartbeat(system_heartbeat);
			}
			else
			{
				char error[64];
				memset(error, 0, 64);
				
				sprintf_P(error, PSTR("STATUS ERROR:"));
				send_command_response_string(error);
				
				memset(error, 0, 64);
				uint16_t error_length = serialize_communication_protocol_error(&communication_and_battery_data.communication_protocol_type_data, error);
				sprintf_P(error + error_length, PSTR(";"));
				send_command_response_string(error);
				
				if(communication_and_battery_data.battery_min_voltage < COMMUNICATION_MODULE_MINIMUM_REQUIRED_VOLTAGE)
				{
					brownout = true;
					
					system_error_t system_error;
					system_error.type = SYSTEM_BROWNOUT;
					add_system_error(&system_error);
				}
				
				adjust_heartbeat_on_error();
			}
			
			battery_voltage = 0;
			return false;
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
			LOG(1, "Entering wolksensor send state");
			
			sent_system_items = 0;
			sent_sensor_readings = 0;
			
			communication_protocol_process_handle = communication_protocol.send_sensor_readings_and_system_data(&sensor_readings_buffer, &system_buffer, &sent_sensor_readings, &sent_system_items);
			add_event_type(&events_buffer, EVENT_COMMUNICATION_PROTOCOL_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_PROTOCOL_DONE:
		{
			LOG(1,"Sending finished");
			
			// copy result data
			communication_protocol_type_data_t send_result = communication_protocol.get_communication_result();
			append_communication_protocol_type_data(&send_result, &communication_and_battery_data.communication_protocol_type_data);
			
			// check errors
			if(is_communication_protocol_success(&send_result))
			{
				LOG(1,"Sending succeded");
				
				transition(STATE_RECEIVE);
			}
			else
			{
				LOG(1,"Sending failed");
				
				transition(STATE_STOP_COMMUNICATION_MODULE);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving wolksensor send state");
				
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
			LOG(1,"Entering wolksensor receive state");
			
			communication_protocol_process_handle = communication_protocol.receive_commands(&commands_buffer);
			add_event_type(&events_buffer, EVENT_COMMUNICATION_PROTOCOL_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_PROTOCOL_DONE:
		{
			LOG(1, "Receive finished");
			
			communication_protocol_type_data_t receive_result = communication_protocol.get_communication_result();
			append_communication_protocol_type_data(&receive_result, &communication_and_battery_data.communication_protocol_type_data);
			
			if(is_communication_protocol_success(&receive_result))
			{
				LOG(1,"Receiving succeded");
				
				if(circular_buffer_size(&commands_buffer) > 0)
				{
					LOG(1, "Commands received from server");
					
					remove_system_data(sent_system_items);
					remove_sensor_readings(sent_sensor_readings);
					
					execute_commands(true);
					
					transition(sensor_readings_count() > 0 ? STATE_SEND : STATE_DISCONNECT);
				}
				else
				{
					LOG(1, "No commands received from server");
					
					transition(STATE_DISCONNECT);
				}
			}
			else
			{
				LOG(1,"Receiving failed");
				
				transition(STATE_STOP_COMMUNICATION_MODULE);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving wolksensor receive state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_disconnect(state_machine_state_t* state, event_t* event)
{	
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering wolksensor disconnect communication protocol state");
			
			communication_protocol_process_handle = communication_protocol.disconnect();
			add_event_type(&events_buffer, EVENT_COMMUNICATION_PROTOCOL_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_PROTOCOL_DONE:
		{
			LOG(1,"Disconnect finished");
			
			communication_protocol_type_data_t disconnect_result = communication_protocol.get_communication_result();
			append_communication_protocol_type_data(&disconnect_result, &communication_and_battery_data.communication_protocol_type_data);
			
			if(is_communication_protocol_success(&disconnect_result))
			{
				LOG(1, "Communication protocol disconnected");
			}
			else
			{
				LOG(1, "Unable to disconnect communication protocol.");
			}
			
			transition(STATE_STOP_COMMUNICATION_MODULE);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving wolksensor disconnect communication protocol state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_stop_communication_module(state_machine_state_t* state, event_t* event)
{
	static communication_module_process_handle_t handle;
	
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering wolksensor stop communication module state");
			
			handle = communication_module.stop();
			add_event_type(&events_buffer, EVENT_COMMUNICATION_PROTOCOL_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_PROTOCOL_PROCESS:
		{
			if(handle())
			{
				add_event_type(&events_buffer, EVENT_COMMUNICATION_PROTOCOL_PROCESS);
			}
			else
			{
				add_event_type(&events_buffer, EVENT_COMMUNICATION_PROTOCOL_DONE);
			}
			
			return true;
		}
		case EVENT_COMMUNICATION_PROTOCOL_DONE:
		{	
			communication_module_type_data_t stop_result = communication_module.get_communication_result();
			
			if(is_communication_module_success(&stop_result))
			{
				LOG(1, "Communication module stopped");
			}
			else
			{
				LOG(1, "Error while stopping communication module");
			}
			
			append_communication_module_type_data(&stop_result, &communication_and_battery_data.communication_protocol_type_data.communication_module_type_data);
			
			transition(STATE_IDLE);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving wolksensor stop communication module state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}
