#include "mqtt_communication_protocol.h"
#include "platform_specific.h"
#include "libemqtt.h"
#include "event_buffer.h"
#include "circular_buffer.h"
#include "logger.h"
#include "config.h"
#include "mqtt_communication_protocol_dependencies.h"
#include "communication_module.h"
#include "state_machine.h"
#include "protocol.h"
#include "command_parser.h"

#define MQTT_COMMUNICATION_PROTOCOL_EVENT_BUFFER_SIZE 10
#define MQTT_BUFFER_SIZE (MAX_BUFFER_SIZE + 3 + MAX_DEVICE_ID_SIZE + 2)

#define MQTT_KEEP_ALIVE_PERIOD 60 // sec 

typedef enum
{
	STATE_MQTT_DISCONNECTED = 0,
	STATE_MQTT_CONNECTING,
		STATE_MQTT_SEND_CONNECT,
		STATE_MQTT_RECEIVE_CONNACK,
		STATE_MQTT_SEND_SUBSCRIBE,
		STATE_MQTT_RECEIVE_SUBACK,
	STATE_MQTT_CONNECTED,
		STATE_MQTT_PUBLISH,
		STATE_MQTT_RECEIVE_PUBLISH,
		STATE_MQTT_PING,
			STATE_MQTT_SEND_PINREQ,
			STATE_MQTT_RECEIVE_PINGRESP,
	STATE_MQTT_DISCONNECTING
}
mqtt_communication_protocol_states_t;

typedef enum
{
	EVENT_MQTT_DISCONNECT = 0,
	EVENT_MQTT_PUBLISH,
	EVENT_MQTT_RECEIVE_CONNACK,
	EVENT_MQTT_RECEIVE_SUBACK,
	EVENT_MQTT_RECEIVE_PUBLISH,
	EVENT_MQTT_RECEIVE_PINGRESP,
	EVENT_COMMUNICATION_MODULE_PROCESS,
	EVENT_COMMUNICATION_MODULE_DONE
}
mqtt_communication_protocol_events_t;

typedef struct
{
	uint8_t type;
	uint16_t message_id;
	int8_t* topic;
	uint8_t topic_size;
	int8_t* data;
	uint16_t data_size;
}
mqtt_mesage_t;

static state_machine_state_t mqtt_communication_protocol_state_machine;
static state_machine_state_t mqtt_communication_protocol_states[13];

static circular_buffer_t mqtt_communication_protocol_event_buffer;
static event_t mqtt_communication_protocol_event_buffer_storage[MQTT_COMMUNICATION_PROTOCOL_EVENT_BUFFER_SIZE];

static mqtt_broker_handle_t broker;

static mqtt_mesage_t mqtt_message;

static uint16_t mqtt_buffer_position = 0;
static uint8_t mqtt_buffer[MQTT_BUFFER_SIZE];

static circular_buffer_t* sending_sensor_readings_buffer = NULL;
static uint16_t* sensor_readings_sent = NULL;

static circular_buffer_t* sending_system_buffer = NULL;
static uint16_t* system_items_sent = NULL;

static actuator_t* sending_actuator = NULL;
static actuator_state_t* sending_actuator_state = NULL;

static circular_buffer_t* received_commands_buffer = NULL;

static uint16_t message_id = 0;
static char topic[32];

static communication_module_process_handle_t communication_module_process_handle = NULL;

static communication_protocol_type_data_t communication_protocol_type_data;

mqtt_communication_protocol_dependencies_t mqtt_communication_protocol_dependencies;

static bool mqtt_communication_protocol_handler(state_machine_state_t* state, event_t* event);
static bool state_mqtt_disconnected(state_machine_state_t* state, event_t* event);
static bool state_mqtt_connecting(state_machine_state_t* state, event_t* event);
	static bool state_mqtt_send_connect(state_machine_state_t* state, event_t* event);
	static bool state_mqtt_receive_connnack(state_machine_state_t* state, event_t* event);
	static bool state_mqtt_send_subscribe(state_machine_state_t* state, event_t* event);
	static bool state_mqtt_receive_suback(state_machine_state_t* state, event_t* event);
static bool state_mqtt_connected(state_machine_state_t* state, event_t* event);	
	static bool state_mqtt_publish(state_machine_state_t* state, event_t* event);
	static bool state_mqtt_receive_publish(state_machine_state_t* state, event_t* event);
	static bool state_mqtt_ping(state_machine_state_t* state, event_t* event);
		static bool state_mqtt_send_pingreq(state_machine_state_t* state, event_t* event);
		static bool state_mqtt_receive_pingresp(state_machine_state_t* state, event_t* event);
static bool state_mqtt_disconnecting(state_machine_state_t* state, event_t* event);

static void init_mqtt_communication_protocol_event_buffer(void)
{
	circular_buffer_init(&mqtt_communication_protocol_event_buffer, mqtt_communication_protocol_event_buffer_storage, MQTT_COMMUNICATION_PROTOCOL_EVENT_BUFFER_SIZE, sizeof(event_t), true, true);
}

static void add_mqtt_communication_protocol_event_type(uint8_t event_type)
{
	add_event_type(&mqtt_communication_protocol_event_buffer, event_type);
}

static bool pop_mqtt_communication_protocol_event(event_t* event)
{
	return pop_event(&mqtt_communication_protocol_event_buffer, event);
}

static bool mqtt_communinication_protocol_process(void)
{
	event_t event;
	if(pop_mqtt_communication_protocol_event(&event))
	{
		state_machine_process_event(mqtt_communication_protocol_states, &mqtt_communication_protocol_state_machine, &event);
		return true;
	}
	
	return false;
}

void clear_mqtt_buffer(void)
{
	mqtt_buffer_position = 0;
	memset(mqtt_buffer, 0, MQTT_BUFFER_SIZE);
}

static void transition(mqtt_communication_protocol_states_t new_state_id)
{
	state_machine_transition(mqtt_communication_protocol_states, &mqtt_communication_protocol_state_machine, new_state_id);
}

static void init_state(mqtt_communication_protocol_states_t id, const char* human_readable_name, state_machine_state_t* parent, int8_t initial_state, state_machine_state_handler handler)
{
	state_machine_init_state(id, human_readable_name, parent, mqtt_communication_protocol_states, initial_state, handler);
}

void mqtt_protocol_init(void) 
{
	LOG(1, "MQTT communication protocol init");
	
	// dependencies
	
	// buffers
	init_mqtt_communication_protocol_event_buffer();
	clear_mqtt_buffer();
	
	// parameters
	load_device_id();
	load_device_preshared_key();
	load_location_status();
	//load_mqtt_username();
	//load_mqtt_password();
	
	/* init state machine */
	mqtt_communication_protocol_state_machine.id = -1;
	mqtt_communication_protocol_state_machine.human_readable_name = NULL;
	mqtt_communication_protocol_state_machine.parent = NULL;
	mqtt_communication_protocol_state_machine.current_state = STATE_MQTT_DISCONNECTED;
	mqtt_communication_protocol_state_machine.handler = mqtt_communication_protocol_handler;

	init_state(STATE_MQTT_DISCONNECTED, NULL, &mqtt_communication_protocol_state_machine, -1, state_mqtt_disconnected);
	init_state(STATE_MQTT_CONNECTING, NULL, &mqtt_communication_protocol_state_machine, STATE_MQTT_SEND_CONNECT, state_mqtt_connecting);
		init_state(STATE_MQTT_SEND_CONNECT, NULL, &mqtt_communication_protocol_states[STATE_MQTT_CONNECTING], -1, state_mqtt_send_connect);
		init_state(STATE_MQTT_RECEIVE_CONNACK, NULL, &mqtt_communication_protocol_states[STATE_MQTT_CONNECTING], -1, state_mqtt_receive_connnack);
		init_state(STATE_MQTT_SEND_SUBSCRIBE, NULL, &mqtt_communication_protocol_states[STATE_MQTT_CONNECTING], -1, state_mqtt_send_subscribe);
		init_state(STATE_MQTT_RECEIVE_SUBACK, NULL, &mqtt_communication_protocol_states[STATE_MQTT_CONNECTING], -1, state_mqtt_receive_suback);
	init_state(STATE_MQTT_CONNECTED, NULL, &mqtt_communication_protocol_state_machine, -1, state_mqtt_connected);
		init_state(STATE_MQTT_PUBLISH, NULL, &mqtt_communication_protocol_states[STATE_MQTT_CONNECTED], -1, state_mqtt_publish);
		init_state(STATE_MQTT_RECEIVE_PUBLISH, NULL, &mqtt_communication_protocol_states[STATE_MQTT_CONNECTED], -1, state_mqtt_receive_publish);
		init_state(STATE_MQTT_PING, NULL, &mqtt_communication_protocol_states[STATE_MQTT_CONNECTED], -1, state_mqtt_ping);
			init_state(STATE_MQTT_SEND_PINREQ, NULL, &mqtt_communication_protocol_states[STATE_MQTT_PING], -1, state_mqtt_send_pingreq);
			init_state(STATE_MQTT_RECEIVE_PINGRESP, NULL, &mqtt_communication_protocol_states[STATE_MQTT_PING], -1, state_mqtt_receive_pingresp);
	init_state(STATE_MQTT_DISCONNECTING, NULL, &mqtt_communication_protocol_state_machine, -1, state_mqtt_disconnecting);
}

static void clear_communication_protocol_data(void)
{
	memset(&communication_protocol_type_data, 0, sizeof(communication_protocol_type_data_t));
	communication_protocol_type_data.type = COMMUNICATION_PROTOCOL_MQTT;
}

static void set_mqtt_communication_protocol_error(mqtt_communication_protocol_error_type_t error_type, uint8_t state)
{
	communication_protocol_type_data.data.mqtt_communication_protocol_data.error = error_type | state;
}

communication_protocol_process_handle_t mqtt_protocol_send_sensor_readings_and_system_data(circular_buffer_t* sensor_readings_buffer, circular_buffer_t* system_buffer, uint16_t* sent_sensor_readings, uint16_t* sent_system_items)
{
	LOG(1, "Mqtt send sensor readings and system data");
	
	sending_sensor_readings_buffer = sensor_readings_buffer;
	sensor_readings_sent = sent_sensor_readings;
	
	sending_system_buffer = system_buffer;
	system_items_sent = sent_system_items;
	
	sending_actuator_state = NULL;
	
	clear_communication_protocol_data();
	
	add_mqtt_communication_protocol_event_type(EVENT_MQTT_PUBLISH);
	
	return mqtt_communinication_protocol_process;
}

communication_protocol_process_handle_t mqtt_protocol_send_actuator_state(actuator_t* actuator, actuator_state_t* actuator_state)
{
	LOG(1, "Mqtt send actuator state");
	
	sending_actuator = actuator;
	sending_actuator_state = actuator_state;
	
	sending_sensor_readings_buffer = NULL;
	sensor_readings_sent = NULL;
	sending_system_buffer = NULL;
	system_items_sent = NULL;
	
	clear_communication_protocol_data();
	
	add_mqtt_communication_protocol_event_type(EVENT_MQTT_PUBLISH);
	
	return mqtt_communinication_protocol_process;
}

communication_protocol_process_handle_t mqtt_protocol_receive_commands(circular_buffer_t* commands_buffer)
{
	LOG(1, "Mqtt receive commands");
	
	received_commands_buffer = commands_buffer;
	
	add_mqtt_communication_protocol_event_type(EVENT_MQTT_RECEIVE_PUBLISH);
	
	clear_communication_protocol_data();
	
	return mqtt_communinication_protocol_process;
}

communication_protocol_process_handle_t mqtt_protocol_disconnect(void)
{
	LOG(1, "Mqtt disconnect");
	
	clear_communication_protocol_data();
	
	add_mqtt_communication_protocol_event_type(EVENT_MQTT_DISCONNECT);
	
	return mqtt_communinication_protocol_process;
}

static bool mqtt_parse_message(void)
{
	uint8_t remaining_length_bytes = mqtt_num_rem_len_bytes(mqtt_buffer);
	uint8_t remaining_length = mqtt_parse_rem_len(mqtt_buffer);

	if (mqtt_buffer_position < (remaining_length + remaining_length_bytes + 1))
	{
		/* not the whole message yet */
		LOG(2, "Not the whole mqtt message yet");
		return false;
	}

	mqtt_message.type = MQTTParseMessageType(mqtt_buffer);

	switch(mqtt_message.type)
	{
		case MQTT_MSG_SUBACK:
		{
			LOG(1, "Mqtt message received: suback");
			
			mqtt_message.message_id = mqtt_parse_msg_id(mqtt_buffer);

			break;
		}
		case MQTT_MSG_PUBLISH:
		{
			LOG(1, "Mqtt message received: publish");
			
			mqtt_message.topic_size = mqtt_parse_pub_topic_ptr(mqtt_buffer, (const uint8_t**)&mqtt_message.topic);
			
			mqtt_message.data_size  = mqtt_parse_pub_msg_ptr(mqtt_buffer, (const uint8_t**)&mqtt_message.data);

			break;
		}
		default:
		{
			LOG_PRINT(1, PSTR("Mqtt message received: %u\r\n"), mqtt_message.type);
			/* for other messages we dot require additional info */
			break;
		}
	}

	return true;
}

static bool mqtt_communication_protocol_handler(state_machine_state_t* state, event_t* event)
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
				add_mqtt_communication_protocol_event_type(EVENT_COMMUNICATION_MODULE_PROCESS);
			}
			else
			{
				add_mqtt_communication_protocol_event_type(EVENT_COMMUNICATION_MODULE_DONE);
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

static bool mqtt_parameters_set(void)
{
	return (*device_id != 0) && (*device_preshared_key != 0);
}

static bool state_mqtt_disconnected(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering mqtt disconnected state");
			
			circular_buffer_clear(&mqtt_communication_protocol_event_buffer);
			
			return true;
		}
		case EVENT_MQTT_DISCONNECT:
		{
			LOG(1, "Ignoring mqtt disconnect in connected state");
			
			return true;
		}
		case EVENT_MQTT_PUBLISH:
		{
			LOG(1, "Mqtt send received in disconnected state");
			
			if(mqtt_parameters_set())
			{
				add_mqtt_communication_protocol_event_type(EVENT_MQTT_PUBLISH);
				transition(STATE_MQTT_CONNECTING);
			}
			else
			{
				LOG(1, "Mqtt protocol parameters not set");	
				
				set_mqtt_communication_protocol_error(ERROR_MQTT_PARAMETERS_MISSING, state->id);
			}
			
			return true;
		}
		case EVENT_MQTT_RECEIVE_PUBLISH:
		{
			LOG(1, "Mqtt receive received in disconnected state");
			
			if(mqtt_parameters_set())
			{
				add_mqtt_communication_protocol_event_type(EVENT_MQTT_RECEIVE_PUBLISH);
				transition(STATE_MQTT_CONNECTING);
			}
			else
			{
				LOG(1, "Mqtt protocol parameters not set");	
				
				set_mqtt_communication_protocol_error(ERROR_MQTT_PARAMETERS_MISSING, state->id);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving mqtt disconnected state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_mqtt_connecting(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering mqtt connecting state");
			
			state->current_state = STATE_MQTT_SEND_CONNECT;
		
			return true;
		}
		case EVENT_MQTT_DISCONNECT:
		{
			LOG(1, "Ignoring mqtt connect/disconnect while connecting");
			
			return true; // ignore them
		}
		case EVENT_MQTT_PUBLISH:
		case EVENT_MQTT_RECEIVE_PUBLISH:
		{
			add_mqtt_communication_protocol_event_type(event->type); // retain event while connecting
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving mqtt connecting state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_mqtt_send_connect(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"Entering mqtt send connect state");
			
			clear_mqtt_buffer();

			mqttlib_init(&broker, device_id);

			sprintf_P(topic, PSTR("will/%s/001"), device_id);

			mqtt_init_will(&broker, topic, "Connection break with WolkSensor", 0, 0);
			mqtt_set_alive(&broker, MQTT_KEEP_ALIVE_PERIOD); /* 60 sec keep alive to avoid sending pings at all */
			
			mqttlib_init_auth(&broker, device_id, device_preshared_key);
			
			LOG_PRINT(1, PSTR("MQTT username and password %s %s\r\n"), device_id, device_preshared_key);
			
			uint16_t connect_message_size = mqtt_connect(&broker, mqtt_buffer, MQTT_BUFFER_SIZE);
			
			communication_module_process_handle = communication_module.sendd(mqtt_buffer, connect_message_size);
			add_mqtt_communication_protocol_event_type(EVENT_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t send_result = communication_module.get_communication_result();
			append_communication_module_type_data(&send_result, &communication_protocol_type_data.communication_module_type_data);
			
			if(is_communication_module_success(&send_result))
			{
				LOG(1, "Mqtt connect message sent");
				
				transition(STATE_MQTT_RECEIVE_CONNACK);
			}
			else
			{
				LOG(1, "Unable to send mqtt connect message");
				
				set_mqtt_communication_protocol_error(ERROR_SENDING_MQTT_MESSAGE, state->id);
				
				transition(STATE_MQTT_DISCONNECTED);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving mqtt send connect state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_mqtt_receive_connnack(state_machine_state_t* state, event_t* event)
{	
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"Entering mqtt receive connack state");
			
			add_mqtt_communication_protocol_event_type(EVENT_MQTT_RECEIVE_CONNACK);
			
			return true;
		}
		case EVENT_MQTT_RECEIVE_CONNACK:
		{
			clear_mqtt_buffer();
			
			communication_module_process_handle = communication_module.receive(mqtt_buffer, MQTT_BUFFER_SIZE, &mqtt_buffer_position);
			add_mqtt_communication_protocol_event_type(EVENT_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t receive_result = communication_module.get_communication_result();
			append_communication_module_type_data(&receive_result, &communication_protocol_type_data.communication_module_type_data);
			
			if(is_communication_module_success(&receive_result))
			{				
				if(mqtt_parse_message())
				{
					if(mqtt_message.type == MQTT_MSG_CONNACK)
					{
						LOG(1, "Mqtt connack message received");
						
						transition(STATE_MQTT_SEND_SUBSCRIBE);
					}
					else
					{
						LOG(1, "Mqtt message received not connack, retrying");
						
						add_mqtt_communication_protocol_event_type(EVENT_MQTT_RECEIVE_CONNACK);
					}
				}
				else
				{
					LOG(1, "Mqtt connack message not received");
					
					set_mqtt_communication_protocol_error(ERROR_RECEIVING_MQTT_MESSAGE, state->id);
					
					transition(STATE_MQTT_DISCONNECTED);
				}
			}
			else
			{
				LOG(1, "Communication module error while waiting mqtt connack message");
				
				set_mqtt_communication_protocol_error(ERROR_RECEIVING_MQTT_MESSAGE, state->id);
				
				transition(STATE_MQTT_DISCONNECTED);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving mqtt receive connack state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_mqtt_send_subscribe(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering mqtt send subscribe state");
			
			clear_mqtt_buffer();
			
			sprintf_P(topic, PSTR("config/%s"), device_id);
			
			uint16_t subscribe_message_size = mqtt_subscribe(&broker, topic, &message_id, mqtt_buffer, MQTT_BUFFER_SIZE);
			
			communication_module_process_handle = communication_module.sendd(mqtt_buffer, subscribe_message_size);
			add_mqtt_communication_protocol_event_type(EVENT_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t send_result = communication_module.get_communication_result();
			append_communication_module_type_data(&send_result, &communication_protocol_type_data.communication_module_type_data);
			
			if(is_communication_module_success(&send_result))
			{
				LOG(1, "Mqtt subscribe message sent");
				
				transition(STATE_MQTT_RECEIVE_SUBACK);
			}
			else
			{
				LOG(1, "Unable to send mqtt subscribe message");
				
				set_mqtt_communication_protocol_error(ERROR_SENDING_MQTT_MESSAGE, state->id);
				
				transition(STATE_MQTT_DISCONNECTED);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving mqtt send subscribe state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_mqtt_receive_suback(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{	
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering mqtt receive suback state");
		
			add_mqtt_communication_protocol_event_type(EVENT_MQTT_RECEIVE_SUBACK);
		
			return true;
		}
		case EVENT_MQTT_RECEIVE_SUBACK: 
		{
			clear_mqtt_buffer();
		
			communication_module_process_handle = communication_module.receive(mqtt_buffer, MQTT_BUFFER_SIZE, &mqtt_buffer_position);
			add_mqtt_communication_protocol_event_type(EVENT_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t receive_result = communication_module.get_communication_result();
			append_communication_module_type_data(&receive_result, &communication_protocol_type_data.communication_module_type_data);
			
			if(is_communication_module_success(&receive_result))
			{			
				if(mqtt_parse_message())
				{	
					if(mqtt_message.type == MQTT_MSG_SUBACK)
					{
						LOG(1, "Mqtt suback message received");
						
						if(mqtt_message.message_id == message_id)
						{
							LOG(1,"Suback message id matches");
							
							transition(STATE_MQTT_CONNECTED);
						}
						else
						{
							LOG(1,"Suback message id does not match");
							
							set_mqtt_communication_protocol_error(ERROR_INCORRECT_MQTT_MESSAGE_RECEIVED, state->id);
							
							add_mqtt_communication_protocol_event_type(STATE_MQTT_DISCONNECTING);
						}
					}
					else
					{
						LOG(1, "Mqtt message received not suback, retrying");
						
						add_mqtt_communication_protocol_event_type(EVENT_MQTT_RECEIVE_SUBACK);
					}			
				}
				else
				{
					LOG(1, "Mqtt suback message not received");
					
					set_mqtt_communication_protocol_error(ERROR_RECEIVING_MQTT_MESSAGE, state->id);
					
					transition(STATE_MQTT_DISCONNECTING);
				}
			}
			else
			{
				LOG(1, "Communication module error while waiting mqtt suback message");
				
				set_mqtt_communication_protocol_error(ERROR_RECEIVING_MQTT_MESSAGE, state->id);
			
				transition(STATE_MQTT_DISCONNECTED);
			}
		
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving mqtt receive suback state");
		
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_mqtt_connected(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1,"Entering mqtt connected state");
			
			state->current_state = -1; // preventing this state to enters child state

			return true;
		}
		case EVENT_MQTT_DISCONNECT:
		{
			LOG(1, "Mqtt disconnect received in connected state");
			
			transition(STATE_MQTT_DISCONNECTING);
			
			return true;
		}
		case EVENT_MQTT_PUBLISH:
		{
			LOG(1, "Mqtt publish received in connected state");
			
			transition(STATE_MQTT_PUBLISH);
			
			return true;
		}
		case EVENT_MQTT_RECEIVE_PUBLISH:
		{
			LOG(1, "Mqtt receive publish received in connected state");
			
			transition(STATE_MQTT_RECEIVE_PUBLISH);
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving mqtt connected state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_mqtt_publish(state_machine_state_t* state, event_t* event)
{
	static uint16_t serialized_sensor_readings;
	static uint16_t serialized_system_items;
	
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering mqtt publish state");

			clear_mqtt_buffer();
			serialized_sensor_readings = 0;
			serialized_system_items = 0;
			
			sprintf_P(topic, PSTR("sensors/%s"), device_id);
			uint16_t header_size = 3 + strlen(topic) + 2;
			
			// payload
			circular_buffer_t message_buffer;
			circular_buffer_init(&message_buffer, mqtt_buffer + header_size, MQTT_BUFFER_SIZE - header_size, sizeof(char), false, true);
			
			if(sending_actuator != NULL && sending_actuator_state != NULL)
			{
				append_actuator_state(sending_actuator, sending_actuator_state, &message_buffer);
				
				LOG_PRINT(1, PSTR("Packed status message: %s\r\n"), message_buffer.storage);
			}
			else
			{
				append_rtc(rtc_get_ts(), &message_buffer);
				
				if(location && (commands_dependencies.get_surroundig_wifi_networks != NULL))
				{
					wifi_network_t networks[10];
					uint8_t networks_number = commands_dependencies.get_surroundig_wifi_networks(networks, 10);
					
					append_detected_wifi_networks(networks, networks_number, &message_buffer);
				}
				
				if(sending_system_buffer != NULL)
				{
					serialized_system_items = append_system_info(sending_system_buffer, 0, &message_buffer, false);
				}
				
				if(sending_sensor_readings_buffer != NULL)
				{
					serialized_sensor_readings = append_sensor_readings(sending_sensor_readings_buffer, 0, &message_buffer, false);
				}
				
				LOG_PRINT(1, PSTR("Packed readings message: %s\r\n"), message_buffer.storage);
				
				if(!ssl)
				{
					uint16_t encrypted_data_size = mqtt_communication_protocol_dependencies.encrypt(message_buffer.storage, circular_buffer_size(&message_buffer), device_preshared_key);
					message_buffer.tail = encrypted_data_size;
				}
			}
			
			uint16_t mqtt_message_size = header_size + circular_buffer_size(&message_buffer);
			
			uint8_t *header = mqtt_buffer;
			*header++ = MQTT_MSG_PUBLISH;
			*header++ = ((mqtt_message_size - 3) % 128) | 0x80;
			*header++ = (mqtt_message_size - 3) / 128;
			*header++ = (header_size - 5) >> 8;
			*header++ = (header_size - 5) & 0xFF;
			memcpy(header, topic, (header_size - 5));
			
			communication_module_process_handle = communication_module.sendd(mqtt_buffer, mqtt_message_size);
			add_mqtt_communication_protocol_event_type(EVENT_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t send_result = communication_module.get_communication_result();
			append_communication_module_type_data(&send_result, &communication_protocol_type_data.communication_module_type_data);
			
			if(is_communication_module_success(&send_result))
			{
				LOG(1, "Mqtt publish message sent");
				
				if(sensor_readings_sent != NULL) *sensor_readings_sent = serialized_sensor_readings;
				if(system_items_sent != NULL) *system_items_sent = serialized_system_items;
	
				transition(STATE_MQTT_CONNECTED);
			}
			else
			{
				LOG(1, "Unable to send mqtt publish message");
				
				set_mqtt_communication_protocol_error(ERROR_SENDING_MQTT_MESSAGE, state->id);
				
				if(sensor_readings_sent != NULL) *sensor_readings_sent = 0;
				if(system_items_sent != NULL) *system_items_sent = 0;
				
				transition(STATE_MQTT_DISCONNECTED);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving mqtt publish state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_mqtt_receive_publish(state_machine_state_t* state, event_t* event)
{	
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering mqtt receive publish state");
			
			add_mqtt_communication_protocol_event_type(EVENT_MQTT_RECEIVE_PUBLISH);
					
			return true;
		}
		case EVENT_MQTT_RECEIVE_PUBLISH:
		{
			clear_mqtt_buffer();
			
			communication_module_process_handle = communication_module.receive(mqtt_buffer, MQTT_BUFFER_SIZE, &mqtt_buffer_position);
			add_mqtt_communication_protocol_event_type(EVENT_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t receive_result = communication_module.get_communication_result();
			append_communication_module_type_data(&receive_result, &communication_protocol_type_data.communication_module_type_data);
			
			if(is_communication_module_success(&receive_result))
			{				
				if(mqtt_parse_message())
				{
					if(mqtt_message.type == MQTT_MSG_PUBLISH)
					{
						LOG(1, "Mqtt publish message received");
						LOG_PRINT(1, PSTR("Received data from mqtt server %s\r\n"), mqtt_message.data);
						
						if(mqtt_message.data_size > 0)
						{
							if(!ssl)
							{
								mqtt_communication_protocol_dependencies.decrypt(mqtt_message.data, mqtt_message.data_size, device_preshared_key);
							}
							
							circular_buffer_t command_string_buffer;
							circular_buffer_init(&command_string_buffer, mqtt_message.data, strlen(mqtt_message.data), sizeof(char), false, false);
							command_string_buffer.head = 0;
							command_string_buffer.tail = 0;
							command_string_buffer.empty = false;
							command_string_buffer.full = true;
							
							LOG_PRINT(1, PSTR("Received data: %s length %u\r\n"), command_string_buffer.storage, circular_buffer_size(&command_string_buffer));
							
							extract_commands_from_string_buffer(&command_string_buffer, received_commands_buffer);
						}
						
						transition(STATE_MQTT_CONNECTED);
					}
					else
					{
						LOG(1, "Mqtt message received not publish, retrying");
						
						add_mqtt_communication_protocol_event_type(EVENT_MQTT_RECEIVE_PUBLISH);
					}
				}
				else
				{
					transition(STATE_MQTT_PING);
				}
			}
			else
			{
				LOG(1, "Wifi communication module error while waiting mqtt publish message");
				
				set_mqtt_communication_protocol_error(ERROR_RECEIVING_MQTT_MESSAGE, state->id);
				
				transition(STATE_MQTT_DISCONNECTED);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1,"Leaving mqtt receive publish state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_mqtt_ping(state_machine_state_t* state, event_t* event)
{
	switch(event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering mqtt ping state");
			
			state->current_state = STATE_MQTT_SEND_PINREQ;
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving mqtt ping state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_mqtt_send_pingreq(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering mqtt send ping request state");
			
			clear_mqtt_buffer();
			
			uint16_t pingreq_message_size = mqtt_ping(&broker, mqtt_buffer, MQTT_BUFFER_SIZE);
			
			communication_module_process_handle = communication_module.sendd(mqtt_buffer, pingreq_message_size);
			add_mqtt_communication_protocol_event_type(EVENT_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t send_result = communication_module.get_communication_result();
			append_communication_module_type_data(&send_result, &communication_protocol_type_data.communication_module_type_data);
			
			if(is_communication_module_success(&send_result))
			{
				LOG(1, "Mqtt pingreq message sent");
				
				transition(STATE_MQTT_RECEIVE_PINGRESP);
			}
			else
			{
				LOG(1, "Unable to send mqtt pingreq message");
				
				set_mqtt_communication_protocol_error(ERROR_SENDING_MQTT_MESSAGE, state->id);
				
				transition(STATE_MQTT_DISCONNECTED);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving mqtt send ping request state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_mqtt_receive_pingresp(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering mqtt receive pingresp state");
			
			add_mqtt_communication_protocol_event_type(EVENT_MQTT_RECEIVE_PINGRESP);
			
			return true;
		}
		case EVENT_MQTT_RECEIVE_PINGRESP:
		{
			clear_mqtt_buffer();
			
			communication_module_process_handle = communication_module.receive(mqtt_buffer, MQTT_BUFFER_SIZE, &mqtt_buffer_position);
			add_mqtt_communication_protocol_event_type(EVENT_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t receive_result = communication_module.get_communication_result();
			append_communication_module_type_data(&receive_result, &communication_protocol_type_data.communication_module_type_data);
			
			if(is_communication_module_success(&receive_result))
			{				
				if(mqtt_parse_message() && mqtt_message.type == MQTT_MSG_PINGRESP)
				{
					if(mqtt_message.type == MQTT_MSG_PINGRESP)
					{
						LOG(1, "Mqtt pingresp message received");
						
						transition(STATE_MQTT_CONNECTED);
					}
					else if(mqtt_message.type == MQTT_MSG_PUBLISH)
					{
						LOG(1, "Mqtt message received not pingresp, publish message received, retrying");
						
						//LOG_PRINT(1, PSTR("Received data from mqtt server %s\r\n"), mqtt_message.data);
						
						//memcpy(buffer, mqtt_message.data, mqtt_message.data_size);
						//*received_data_size = mqtt_message.data_size;
						
						add_mqtt_communication_protocol_event_type(EVENT_MQTT_RECEIVE_PINGRESP);
					}
					else
					{
						LOG(1, "Mqtt message received not pingresp, retrying");
						// just try again to receive pingresp
						add_mqtt_communication_protocol_event_type(EVENT_MQTT_RECEIVE_PINGRESP);
					}
				}
				else
				{
					LOG(1, "Mqtt pingresp message not received");
					
					set_mqtt_communication_protocol_error(ERROR_RECEIVING_MQTT_MESSAGE, state->id);
					
					transition(STATE_MQTT_DISCONNECTED);
				}
			}
			else
			{
				LOG(1, "Communication module error while waiting mqtt connack message");
				
				set_mqtt_communication_protocol_error(ERROR_RECEIVING_MQTT_MESSAGE, state->id);
				
				transition(STATE_MQTT_DISCONNECTED);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving mqtt receive ping response state");
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

static bool state_mqtt_disconnecting(state_machine_state_t* state, event_t* event)
{
	switch (event->type)
	{
		case EVENT_ENTERING_STATE:
		{
			LOG(1, "Entering mqtt disconnecting state");
			
			clear_mqtt_buffer();
			
			uint16_t disconnect_message_size = mqtt_disconnect(&broker, mqtt_buffer, MQTT_BUFFER_SIZE);
			
			communication_module_process_handle = communication_module.sendd(mqtt_buffer, disconnect_message_size);
			add_mqtt_communication_protocol_event_type(EVENT_COMMUNICATION_MODULE_PROCESS);
			
			return true;
		}
		case EVENT_COMMUNICATION_MODULE_DONE:
		{
			communication_module_type_data_t send_result = communication_module.get_communication_result();
			append_communication_module_type_data(&send_result, &communication_protocol_type_data.communication_module_type_data);
			
			if(is_communication_module_success(&send_result))
			{
				LOG(1, "Mqtt disconnect message sent");
				
				transition(STATE_MQTT_DISCONNECTED);
			}
			else
			{
				LOG(1, "Unable to send mqtt disconnect message");
				
				set_mqtt_communication_protocol_error(ERROR_SENDING_MQTT_MESSAGE, state->id);
				
				transition(STATE_MQTT_DISCONNECTED);
			}
			
			return true;
		}
		case EVENT_LEAVING_STATE:
		{
			LOG(1, "Leaving mqtt disconnecting state");
			
			circular_buffer_clear(&mqtt_communication_protocol_event_buffer);
			
			return false;
		}
		default:
		{
			return false;
		}
	}
}

communication_protocol_type_data_t get_mqtt_communication_result(void)
{
	return communication_protocol_type_data;
}
