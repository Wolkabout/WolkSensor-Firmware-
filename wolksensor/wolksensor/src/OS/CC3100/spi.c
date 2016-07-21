#include <stdbool.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>


// OS
#include "spi.h"
#include "clock.h"
#include "rtc.h"		// !!!!!
#include "brd.h"
#include "Sensors/sensor.h"
#include "Sensors/batteryADC.h"
#include "config/conf_os.h"
#include "test.h"
#include "logger.h"
#include "chrono.h"
#include "clock.h"
#include <util/delay.h>

#define SHUT_DOWN_DELAY			10		// 10ms

#define ASSERT_CS()             PORTB.OUTCLR = 0b00100000;		// ioport_set_pin_level(M_SPI_CS, 0)
#define DEASSERT_CS()           PORTB.OUTSET = 0b00100000;		// ioport_set_pin_level(M_SPI_CS, 1)

// Static buffer for 5 bytes of SPI HEADER
unsigned char tSpiReadHeader[] = {3, 0, 0, 0, 0};
	
static void (*pIraEventHandler)(void* pValue) = 0;

inline long ReadWlanInterruptPin(void) {
	register8_t saved_sreg = SREG;
	cli();
	long tmp = ((PORTD.IN & 0b01000000)?1:0);
	SREG = saved_sreg;
	return tmp;
}


inline void WlanInterruptEnable(void) {
	register8_t saved_sreg = SREG;
	cli();
	PORTD.INTCTRL |= 0b00000010;		// interrupt 0, medium level
	SREG = saved_sreg;
}


inline void WlanInterruptDisable(void) {
	register8_t saved_sreg = SREG;
	cli();
	PORTD.INTCTRL &= ~0b00000011;		// interrupt 0 off
	SREG = saved_sreg;
}

void WriteWlanPin( unsigned char val ) {
	register8_t saved_sreg = SREG;
	cli();
	uint16_t volatile delay_t = 0;

	if (val && !(PORTB.IN & 0b00000001)) {
		DEASSERT_CS();					
		PORTB.OUTSET = 0b00000001;		// enable CC3000 power
	LOG(4,"Write Wlan Pin");

	} else if (!val && (PORTB.IN & 0b0000001)) {
		PORTB.OUTCLR = 0b00000001;		// disable CC3000 power
		delay_t = SHUT_DOWN_DELAY;
	}
	
	SREG = saved_sreg;
	if (delay_t) {
		start_auxTimeout(delay_t);
		while (!read_auxTimeout());
	}

}

void SpiPauseSpi(void) 
{
	register8_t saved_sreg = SREG;
	cli();
	PORTD.INTCTRL &= ~0b00000011;		// interrupt 0 off
	SREG = saved_sreg;
}


void SpiResumeSpi(void) 
{
	register8_t saved_sreg = SREG;
	cli();
	PORTD.INTCTRL |= 0b00000010;		// interrupt 0, medium level
	SREG = saved_sreg;
}

//////////////////////////////////////////////////////////////////////////////

ISR(PORTD_INT0_vect) 
{	
	if (pIraEventHandler)
	{
		pIraEventHandler(0);
	}
}


int SlStudio_RegisterInterruptHandler(void (*InterruptHdl)(void* pValue), void* pValue)
{
	pIraEventHandler = InterruptHdl;

	return 0;
}

Fd_t spi_Open(char *ifName, unsigned long flags)
{
	LOG(1, "SPI open");
	
	SpiPauseSpi();

	PR.PRPC &= ~PR_SPI_bm;						// enable SIP peripheral on port C
	SPIC.CTRL = 0b11010000;						// setup SPI device
	SPIC.INTCTRL=SPIC.INTCTRL&~SPI_INTLVL_gm;	//disable SPI interrupt

	// enable external falling edge interrupt on M_SPI_IRQ
	PORTD.INT0MASK = 0b01000000;		// M_SPI_IRQ is in int0 group
	PORTD.PIN6CTRL = 0b00010001;		// pull-down, detect falling edge
	PORTD.INTFLAGS = 0b00000001;		// clear pending int0 interrupts
	
	//_delay_ms(50);
	
	SpiResumeSpi();
	
	return 0;
}

int spi_Close(Fd_t fd)
{
	//register8_t saved_sreg = SREG;
	//cli();

	SpiPauseSpi();

	SPIC.CTRL = 0b10010100;				// disable SPI device
	PR.PRPC |= PR_SPI_bm;				// disable SIP peripheral on port C

	//SREG = saved_sreg;
	
	//_delay_ms(50);
	
	return 0;
}

int spi_Read(Fd_t fd, unsigned char *data, int size)
{
	ASSERT_CS();
	
	long i = 0;
	unsigned char *data_to_send = tSpiReadHeader;

	for (i = 0; i < size; i ++)
	{
		SPIC.DATA = 0xFF;
		while (!(SPIC.STATUS & SPI_IF_bm));
		data[i] = SPIC.DATA;
		while (SPIC.STATUS & SPI_IF_bm);
	}
	
	DEASSERT_CS();
	
	//_delay_ms(10);
	
	return size;
}

int spi_Write(Fd_t fd, unsigned char *data, int len)
{
	int size = len;

	ASSERT_CS();
	
	SPIC.STATUS;
	SPIC.DATA;
	while (SPIC.STATUS & SPI_IF_bm);
	while (size) {
		SPIC.DATA = *data;
		while (!(SPIC.STATUS & SPI_IF_bm));
		SPIC.DATA;
		size --;
		data++;
		while (SPIC.STATUS & SPI_IF_bm);
	}
	
	DEASSERT_CS();
	
	_delay_ms(10);
	
	return len;
}



	