#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "RTC.h"
#include "event_buffer.h"
#include "logger.h"
#include "test.h"

static void (*minute_expired_listener)(void) = NULL;

bool RTC_interruptEvent=true;

static uint32_t RTC_Minutes __attribute__ ((section (".noinit1")));

void RTC_init(bool cold_boot) {
	register8_t saved_sreg = SREG;
	cli();
	if (cold_boot) {
		RTC_Minutes = 0;
	} 

	/* restore SREG */
	SREG = saved_sreg;
}

void add_minute_expired_listener(void (*listener)(void))
{
	minute_expired_listener = listener;
}

uint32_t rtc_get(void)
{
	register8_t saved_sreg = SREG;
	cli();
	uint32_t tmp = RTC_Minutes;

	tmp = tmp*60;
	uint16_t tmp1 = RTC.CNT;
	tmp = tmp + (tmp1/30720.0)*60;
	SREG = saved_sreg;

	return tmp;
}

bool RTC_interruptStatus(void)
{
	return RTC_interruptEvent;
}

void RTC_interruptReset(void)
{
	RTC_interruptEvent = false;
}

ISR(RTC_OVF_vect) 
{
	LOG(1,"RTC interrupt");
	
	if(minute_expired_listener)
	{
		minute_expired_listener();
	}
	
	RTC_interruptEvent = true;
	RTC_Minutes++;
}

