#ifndef WIFI_CC3100_H_
#define WIFI_CC3100_H_

void CC3100_enable(void);
void CC3100_disable(void);
bool CC3100_process(void);

bool init_wifi(void);

bool wifi_start(void);
bool wifi_connect(char* ssid, char* password, uint8_t auth_type);
bool wifi_disconnect(void);
bool wifi_stop(void);
bool wifi_reset(void);

int wifi_open_socket(char* address, uint16_t port, bool secure);
int wifi_open_udp_socket(char* address, uint16_t port);
bool wifi_close_socket(int socket_id);
int wifi_send(int socket, uint8_t* buffer, uint16_t length);
int wifi_send_to(int socket, uint8_t* buffer, uint16_t count, char* address, uint16_t port);
int wifi_receive(int socket, uint8_t* buffer, uint16_t length);
int wifi_receive_from(int socket, uint8_t* buffer, uint16_t size, char* address, uint16_t* port);

uint16_t serialize_wifi_platform_specific_error_code(uint32_t error_code, char* buffer);

uint32_t wifi_get_current_ip(char* ip_address);

void add_wifi_connected_listener(void (*listener)(void));
void add_wifi_ip_address_acquired_listener(void (*listener)(void));
void add_wifi_disconnected_listener(void (*listener)(void));
void add_wifi_error_listener(void (*listener)(void));
void add_wifi_platform_specific_error_code_listener(void (*listener)(uint32_t error_code));
void add_wifi_socket_closed_listener(void (*listener)(void));

#endif /* WIFI_CC3100_H_ */