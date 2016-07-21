#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/power.h>

#include "brd.h"

reset_reason_t reset_reason __attribute__ ((section (".noinit1")));


void brd_init(void) {
	PORTA.OUT = 0b00000001;
	PORTB.OUT = 0b00100000;
	PORTC.OUT = 0b10100000;
	PORTD.OUT = 0b00000000;
	PORTE.OUT = 0b00000000;
	PORTF.OUT = 0b00000000;
	
	PORTA.DIR = 0b00000001;
	PORTB.DIR = 0b00100001;
	PORTC.DIR = 0b10111000;
	PORTD.DIR = 0b00000000;
	PORTE.DIR = 0b00000000;
	PORTF.DIR = 0b00000000;
	PORTR.DIR = 0b00000000;

	PORTA.PIN0CTRL = 0b00010111;		// pull-down, empty pin
	PORTA.PIN1CTRL = 0b00010111;		// pull-down, empty pin
	PORTA.PIN2CTRL = 0b00010111;		// pull-down, empty pin
	PORTA.PIN3CTRL = 0b00000111;		// VBAT_AD
	PORTA.PIN4CTRL = 0b00010111;		// pull-down, empty pin
	PORTA.PIN5CTRL = 0b00010111;		// pull-down, empty pin
	PORTA.PIN6CTRL = 0b00010111;		// pull-down, empty pin
	PORTA.PIN7CTRL = 0b00011111;		// pull-up, empty pin

	PORTB.PIN0CTRL = 0b00000000;		// output, CC3000_ON
	PORTB.PIN1CTRL = 0b00010111;		// pull-down, empty pin
	PORTB.PIN2CTRL = 0b00010111;		// pull-down, empty pin
	PORTB.PIN3CTRL = 0b00010111;		// pull-down, empty pin
	PORTB.PIN4CTRL = 0b00010111;		// pull-down, empty pin
	PORTB.PIN5CTRL = 0b00000000;		// output, CC3000_CS
	PORTB.PIN6CTRL = 0b00010111;		// pull-down, empty pin
	PORTB.PIN7CTRL = 0b00010111;		// pull-down, empty pin

	PORTC.PIN0CTRL = 0b00011000;		// pull-up, S_SDA
	PORTC.PIN1CTRL = 0b00011000;		// pull-up, S_SCL
	PORTC.PIN2CTRL = 0b00000000;		//
	PORTC.PIN3CTRL = 0b00000000;		//	
	PORTC.PIN4CTRL = 0b00010111;		// pull-down, empty pin	
	PORTC.PIN5CTRL = 0b00010000;		// pull-down (SPI output when anabled), MOSI, M_SPI_DIN
	PORTC.PIN6CTRL = 0b00010000;		// pull-down, MISO, M_SPI_DOUT
	PORTC.PIN7CTRL = 0b00010000;		// pull-down (SPI output when anabled), SCK, M_SPI_CLK

	PORTD.PIN0CTRL = 0b00010111;		// pull-down, empty pin
	PORTD.PIN1CTRL = 0b00000000;		// UART_CTS
	PORTD.PIN2CTRL = 0b00000000;		// UART_RXD
	PORTD.PIN3CTRL = 0b00000000;		// UART_TXT
	PORTD.PIN4CTRL = 0b00000000;		// UART_RTS
	PORTD.PIN5CTRL = 0b00010111;		// pull-down, empty pin 
	PORTD.PIN6CTRL = 0b00010010;		// pull-down, falling edge interrupt, CC3000_IRQ
	PORTD.PIN7CTRL = 0b00010111;		// pull-down, empty pin

	PORTE.PIN0CTRL = 0b00000111;		// disabled pin
	PORTE.PIN1CTRL = 0b00000111;		// disabled pin
	PORTE.PIN2CTRL = 0b00000000;		// USB_DETECT
	PORTE.PIN3CTRL = 0b00000111;		// disabled pin
	PORTE.PIN4CTRL = 0b00000111;		// disabled pin
	PORTE.PIN5CTRL = 0b00000111;		// disabled pin
	PORTE.PIN6CTRL = 0b00000000;		// open, XTAL
	PORTE.PIN7CTRL = 0b00000000;		// open, XTAL

	PORTF.PIN0CTRL = 0b00000111;		// disabled pin
	PORTF.PIN1CTRL = 0b00000111;		// disabled pin
	PORTF.PIN2CTRL = 0b00000111;		// disabled pin
	PORTF.PIN3CTRL = 0b00000111;		// disabled pin
	PORTF.PIN4CTRL = 0b00000111;		// disabled pin
	PORTF.PIN5CTRL = 0b00000111;		// disabled pin
	PORTF.PIN6CTRL = 0b00000111;		// disabled pin
	PORTF.PIN7CTRL = 0b00000111;		// disabled pin
		
	PR.PRGEN = 0xff;		/* General Power Reduction */
	PR.PRPA = 0xff;			/* Power Reduction Port A */
	PR.PRPC = 0xff;			/* Power Reduction Port C */
	PR.PRPD = 0xff;			/* Power Reduction Port D */
	PR.PRPE = 0xff;			/* Power Reduction Port E */
	PR.PRPF = 0xff;			/* Power Reduction Port F */

//	PR.PRGEN |= 0x41;		/* General Power Reduction */
//	PR.PRPA |= 0x03;			/* Power Reduction Port A */
//	PR.PRPC |= 0x14;			/* Power Reduction Port C */
//	PR.PRPD |= 0x08;			/* Power Reduction Port D */
//	PR.PRPE |= 0x51;			/* Power Reduction Port E */
//	PR.PRPF |= 0x10;			/* Power Reduction Port F */
}

void system_reset(void)
{
	reset(RST_RELOAD);
}

void reset(reset_reason_t reason) {
	reset_reason = reason;
	cli();
	ccp_write_IO((uint8_t *)&RST.CTRL, RST_SWRST_bm);
	for(;;);
}


void ccp_write_IO( volatile uint8_t * address, uint8_t value ) {
	register8_t saved_sreg = SREG;
	cli();
	volatile uint8_t * tmpAddr = address;
#ifdef RAMPZ
	RAMPZ = 0;
#endif
	asm volatile(
	"movw r30,  %0"       "\n\t"
	"ldi  r16,  %2"       "\n\t"
	"out   %3, r16"       "\n\t"
	"st     Z,  %1"       "\n\t"
	:
	: "r" (tmpAddr), "r" (value), "M" (CCP_IOREG_gc), "i" (&CCP)
	: "r16", "r30", "r31"
	);
	SREG = saved_sreg;
}


static inline void nvm_wait_until_ready( void ) {
	do {} while ((NVM.STATUS & NVM_NVMBUSY_bm) == NVM_NVMBUSY_bm);
}


uint8_t ReadUserSignatureByte(uint16_t Address) {
	nvm_wait_until_ready();
	NVM_CMD = NVM_CMD_READ_USER_SIG_ROW_gc;
	uint8_t Result;
	__asm__ ("lpm %0, Z\n" : "=r" (Result) : "z" (Address));
	NVM_CMD = NVM_CMD_NO_OPERATION_gc;
	return Result;
}


void ReadUserSignature(void *dst, void *src, size_t size) {
	uint8_t *dest = (uint8_t*)dst;
	uint8_t *source = (uint8_t*)src;
	while (size) {
		*(dest++) = ReadUserSignatureByte((uint16_t)source++);
		size--;
	}
}


uint8_t ReadSignatureByte(uint16_t Address) {
	nvm_wait_until_ready();
	NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
	uint8_t Result;
	__asm__ ("lpm %0, Z\n" : "=r" (Result) : "z" (Address));
	NVM_CMD = NVM_CMD_NO_OPERATION_gc;
	return Result;
}


static inline void wdt_wait_while_busy(void) {
	while ((WDT.STATUS & WDT_SYNCBUSY_bm) == WDT_SYNCBUSY_bm) {		// Wait until synchronization
	}
}


void watchdog_enable(uint8_t period) {
	uint8_t temp = (WDT_PER_gm & period) | (1 << WDT_ENABLE_bp) | (1 << WDT_CEN_bp);
	ccp_write_IO((void *)&WDT.CTRL, temp);
	wdt_wait_while_busy();
}


void watchdog_disable(void) {
	uint8_t temp = (WDT.CTRL & ~WDT_ENABLE_bm) | (1 << WDT_CEN_bp);
	ccp_write_IO((void *)&WDT.CTRL, temp);
	wdt_wait_while_busy();
}

