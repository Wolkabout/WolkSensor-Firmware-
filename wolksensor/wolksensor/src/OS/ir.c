#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "clock.h"

static uint16_t *code;
static uint16_t count;
static bool state;

uint16_t ir_code[128];

/*
uint16_t ir_code[128] = {
	342, 171,

	21, 22,
	21, 22,
	21, 22,
	21, 64,
	21, 22,
	21, 22,
	21, 22,
	21, 22,

	21, 64,
	21, 64,
	21, 64,
	21, 22,
	21, 64,
	21, 64,
	21, 64,
	21, 64,

	21, 22,
	21, 64,
	21, 22,
	21, 22,
	21, 64,
	21, 22,
	21, 22,
	21, 22,
	
	21, 64,
	21, 22,
	21, 64,
	21, 64,
	21, 22,
	21, 64,
	21, 64,
	21, 64,

	21, 21,
	0
};
*/



void ir_start(uint16_t freq, uint16_t *in_code) {
	clock_set(CLK_24MHZ);

	register8_t saved_sreg = SREG;
	cli();

	PR.PRPE &= ~PR_TC0_bm;
	TCE0.CTRLA = 0;									// stop timer
	TCE0.CTRLB = 0b10000011;						// single slope PWM
	TCE0.CNT = 0;
	TCE0.PER = 24000000L / (unsigned long)freq;		// 666 for 36 khz, 632 for 38KHz
	TCE0.CCD = 8000000L / (unsigned long)freq;		// on third period

	code = in_code;
	count = *code;
	state = true;

	TCE0.INTCTRLA = 0x3;							// enable interrupts
	TCE0.CTRLA = 0x01;								// run timer

	SREG = saved_sreg;
}



ISR(TCE0_OVF_vect) {
	if (--count == 0) {
		code ++;
		count = *code;
		if (count != 0) {
			state ^= true;
			if (state)
				TCE0.CCD = 222;
			else
				TCE0.CCD = 0;
		} else {
			TCE0.CTRLA = 0;			// stop
			TCE0.CTRLB = 0;			// reset output
//			clock_set(CLK_32KHZ);
		}
	}
}
