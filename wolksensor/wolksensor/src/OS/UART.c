#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <util/delay.h>
#include "UART.h"
#include "test.h"
#include "circular_buffer.h"
#include "commands.h"
#include "logger.h"
#include "clock.h"

#define UART_COMMAND_RESPONSE_BUFFER_STORAGE_SIZE 256

static circular_buffer_t uart_command_response_buffer;
static uint8_t uart_command_response_buffer_storage[UART_COMMAND_RESPONSE_BUFFER_STORAGE_SIZE];
volatile static bool transmitting_command_response = false;

static void (*command_data_received_listener)(char *data, uint16_t length) = NULL;

#ifdef LOG_ENABLED

	#define LOG_BUFFER_STORAGE_SIZE 1024

	static circular_buffer_t log_buffer;
	static char log_buffer_storage[LOG_BUFFER_STORAGE_SIZE];
	volatile static bool transmitting_log = false;

#endif

void uart_init(void) {
	register8_t saved_sreg = SREG;
	cli();

	PORTD.OUTSET = PIN3_bm;		// set TXD high
	PORTD.DIRSET = PIN3_bm;		// PC3 (TXD0) as output.
	PORTD.DIRCLR = PIN2_bm;		// PC2 (RXD0) as input.
	PORTD.DIRSET = PIN1_bm;		// PC1 (TXD0) as output.
	PORTD.OUTCLR = PIN1_bm;		// set PC1 to low.
	
	/* enable power for USART0 on port D */
	PR.PRPD &= ~PR_USART0_bm;
	
	//Baud rate 115200
	USARTD0.BAUDCTRLA = 0x19;
	USARTD0.BAUDCTRLB = 0x00;
	
	USARTD0.CTRLA = USART_TXCINTLVL_MED_gc|USART_RXCINTLVL_MED_gc;
	USARTD0.CTRLC = (USARTD0.CTRLC & ~USART_CHSIZE_gm) |  USART_CHSIZE_8BIT_gc;
	USARTD0.CTRLB = (USART_TXEN_bm | USART_CLK2X_bm | USART_RXEN_bm);
	
	circular_buffer_init(&uart_command_response_buffer, uart_command_response_buffer_storage, UART_COMMAND_RESPONSE_BUFFER_STORAGE_SIZE, sizeof(char), false, true);
	
#ifdef LOG_ENABLED
	/*settings for DEBUG UART*/
	PORTC.OUTSET = PIN3_bm; // set TXD high
	PORTC.DIRSET = PIN3_bm; // PC3 (TXC0) as output.
	PORTC.DIRCLR = PIN2_bm; // PC2 (RXC0) as input.
	/* enable power for USART0 on port C */
	PR.PRPC &= ~PR_USART0_bm;
	//Baud rate 115200
	USARTC0.BAUDCTRLA = 0x19;
	USARTC0.BAUDCTRLB = 0x00;
	USARTC0.CTRLA = USART_TXCINTLVL_LO_gc;
	USARTC0.CTRLC = (USARTD0.CTRLC & ~USART_CHSIZE_gm) | USART_CHSIZE_8BIT_gc;
	USARTC0.CTRLB = (USART_TXEN_bm | USART_CLK2X_bm);
	
	circular_buffer_init(&log_buffer, log_buffer_storage, LOG_BUFFER_STORAGE_SIZE, sizeof(char), true, true);
#endif

	SREG = saved_sreg;
}

void add_command_data_received_listener(void (*listener)(char *data, uint16_t length))
{
	command_data_received_listener = listener;
}

void transmit_command_response_character(void)
{
	register8_t saved_sreg = SREG;
	cli();
	char character;
	circular_buffer_pop(&uart_command_response_buffer, &character);
	USARTD0.DATA = character;
	SREG = saved_sreg;
}

void send_command_response(const char* response, uint16_t length)
{
	uint16_t added = circular_buffer_add_as_many_as_possible(&uart_command_response_buffer, response, length);
	
	if(!transmitting_command_response)
	{
		transmitting_command_response = true;
		transmit_command_response_character();
	}
	
	uint8_t retries = 100;
	while((added < length) && (--retries > 0))
	{
		_delay_ms(10);
		uint16_t new_bytes = circular_buffer_add_as_many_as_possible(&uart_command_response_buffer, response + added, length - added);
		added += new_bytes;
		if(new_bytes)
		{
			retries = 100;
		}
	}

	retries = 100;
	while(!circular_buffer_empty(&uart_command_response_buffer) && (--retries > 0))
	{
		_delay_ms(10);
	}
}

ISR(USARTD0_TXC_vect) {
	if(circular_buffer_empty(&uart_command_response_buffer))
	{
		transmitting_command_response = false;
	}
	else
	{
		transmit_command_response_character();
	}
}

ISR(USARTD0_RXC_vect) {
	register8_t saved_sreg = SREG;
	cli();
	char in = USARTD0.DATA;
	if(command_data_received_listener) command_data_received_listener(&in, 1);
	SREG = saved_sreg;
}

#ifdef LOG_ENABLED

void transmit_log_character(void)
{
	register8_t saved_sreg = SREG;
	cli();
	char character;
	circular_buffer_pop(&log_buffer, &character);
	USARTC0.DATA = character;
	SREG = saved_sreg;
}

void transmit_log(char* message, int size)
{
	uint16_t added = circular_buffer_add_as_many_as_possible(&log_buffer, message, size);
	
	if(!transmitting_log)
	{
		transmitting_log = true;
		transmit_log_character();
	}
	
	uint8_t retries = 10;
	while((added < size) && (--retries > 0))
	{
		_delay_ms(10);
		added += circular_buffer_add_as_many_as_possible(&log_buffer, message + added, size - added);
	}
}

ISR(USARTC0_TXC_vect) {
	if(circular_buffer_empty(&log_buffer))
	{
		transmitting_log = false;
	}
	else
	{
		transmit_log_character();
	}
}

bool transmitting_log_in_progress(void)
{
	return transmitting_log;
}

#endif