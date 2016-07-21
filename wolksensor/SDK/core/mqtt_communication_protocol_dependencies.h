#ifndef MQTT_COMMUNICATION_PROTOCOL_DEPENDENCIES_H_
#define MQTT_COMMUNICATION_PROTOCOL_DEPENDENCIES_H_

typedef struct  
{
	uint16_t (*encrypt)(uint8_t* buff, uint16_t size, uint8_t* key);
	void (*decrypt)(uint8_t* message, uint16_t message_len, uint8_t* key);
}
mqtt_communication_protocol_dependencies_t;

extern mqtt_communication_protocol_dependencies_t mqtt_communication_protocol_dependencies;

#endif /* MQTT_COMMUNICATION_PROTOCOL_DEPENDENCIES_H_ */
