/*
 * protocol.h
 *
 * Created: 6/5/2015 3:42:38 PM
 *  Author: btomic
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include "sensors.h"
#include "actuators.h"
#include "commands_dependencies.h"

uint16_t append_sensor_readings(circular_buffer_t* sensor_readings_buffer, uint16_t start_position, circular_buffer_t* message_buffer, bool split);
uint16_t append_system_info(circular_buffer_t* system_info_buffer, uint16_t start, circular_buffer_t* message_buffer, bool split);

uint16_t serialize_communication_protocol_error(communication_protocol_type_data_t* communication_protocol_type_data, char* buffer);

bool append_done(circular_buffer_t* message_buffer);
bool append_bad_request(circular_buffer_t* message_buffer);
bool append_busy(circular_buffer_t* message_buffer);
bool append_mac_address(unsigned char* mac, circular_buffer_t* message_buffer);
bool append_heartbeat(uint16_t heartbeat, circular_buffer_t* response_buffer);
bool append_rtc(uint32_t rtc, circular_buffer_t* response_buffer);
bool append_version(const char* version, circular_buffer_t* response_buffer);
bool append_status(char* status, circular_buffer_t* response_buffer);
bool append_id(char* id, circular_buffer_t* response_buffer);
bool append_signature(char* signature, circular_buffer_t* response_buffer);
bool append_url(char* url, circular_buffer_t* response_buffer);
bool append_port(uint16_t port, circular_buffer_t* response_buffer);
bool append_ssid(char* ssid, circular_buffer_t* response_buffer);
bool append_pass(char* pass, circular_buffer_t* response_buffer);
bool append_auth(uint8_t auth, circular_buffer_t* response_buffer);
bool append_movement_enabled(bool enabled, circular_buffer_t* response_buffer);
bool append_atmo_enabled(bool enabled, circular_buffer_t* response_buffer);
bool append_static_ip(char* ip, circular_buffer_t* response_buffer);
bool append_static_mask(char* mask, circular_buffer_t* response_buffer);
bool append_static_gateway(char* gateway, circular_buffer_t* response_buffer);
bool append_static_dns(char* dns, circular_buffer_t* response_buffer);
bool append_alarms(sensor_alarms_t* alarms, uint16_t length, circular_buffer_t* response_buffer);
bool append_actuator_state(actuator_t* actuator, actuator_state_t* actuator_state, circular_buffer_t* buffer);
bool append_knx_physical_address(uint8_t knx_physical_address[2], circular_buffer_t* buffer);
bool append_knx_group_address(uint8_t knx_group_address[2], circular_buffer_t* buffer);
bool append_multicast_address(char* url, circular_buffer_t* response_buffer);
bool append_multicast_port(uint16_t port, circular_buffer_t* response_buffer);
bool append_knx_nat_status(bool knx_nat_status, circular_buffer_t* response_buffer);
bool append_detected_wifi_networks(wifi_network_t* networks, uint8_t networks_size, circular_buffer_t* response_buffer);
bool append_location_status(bool location_status, circular_buffer_t* response_buffer);
bool append_ssl_status(bool ssl_status, circular_buffer_t* response_buffer);
bool append_mqtt_username(char* id, circular_buffer_t* response_buffer);
bool append_mqtt_password(char* password, circular_buffer_t* response_buffer);
bool append_temp_offset(uint16_t offset, circular_buffer_t* message_buffer);
bool append_humidity_offset(uint16_t offset, circular_buffer_t* message_buffer);
bool append_pressure_offset(uint16_t offset, circular_buffer_t* message_buffer);
bool append_offset_factory(char* offset_factory, circular_buffer_t* message_buffer);

#endif /* PROTOCOL_H_ */
