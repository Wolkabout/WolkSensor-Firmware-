#ifndef UART_H_
#define UART_H_

void uart_init(void);
void add_command_data_received_listener(void (*listener)(char *data, uint16_t length));
void send_command_response(const char* response, uint16_t size);
void transmit_log(char* message, int size);

#ifdef LOG_ENABLED

bool transmitting_log_in_progress(void);

#endif

#endif /* UART_H_ */
