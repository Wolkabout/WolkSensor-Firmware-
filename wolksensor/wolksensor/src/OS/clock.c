#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "brd.h"
#include "clock.h"
#include "RTC.h"
#include "UART.h"
#include "test.h"
#include "chrono.h"
#include "clock.h"
#include "config/conf_os.h"
#include "event_buffer.h"
#include "Sensors/LSM303.h"

#define TIMER_FREQ	375		// timer overflows on 1ms

static speed_t sysSpeed = CLK_UNKNOWN;

static void clock_speed(void);

#define ONE_SECOND_PERIOD 1000

static volatile uint16_t wifi_timeout = 0;
static volatile uint16_t wifi_timeout_timer = 0;
static volatile bool wifi_timeout_flag = false;

static volatile uint16_t timer_auxTimeout;
static volatile bool auxTimeout_flag=false;
static volatile uint16_t timer_usbChangeStateTime=0;

static volatile uint16_t oneSecondCount=0;

typedef void (*second_expired_listener_t)(void);
typedef void (*milisecond_expired_listener_t)(void);

static second_expired_listener_t second_expired_listeners[3] = {NULL, NULL, NULL};
static milisecond_expired_listener_t milisecond_expired_listeners[2] = {NULL, NULL};

void clock_init(void) {

	sysSpeed = CLK_24MHZ;

	/* Disable interrupts */
	register8_t saved_sreg = SREG;
	cli();

	/* Calibrate 48MHz internal clock */
	uint16_t cal;
	MSB(cal) = ReadSignatureByte(offsetof(NVM_PROD_SIGNATURES_t, USBRCOSC));
	LSB(cal) = ReadSignatureByte(offsetof(NVM_PROD_SIGNATURES_t, USBRCOSCA));
	if (cal == 0xFFFF) {
		cal = 0x2340;
	}
	DFLLRC32M.CALA=LSB(cal);
	DFLLRC32M.CALB=MSB(cal);

	/* Enable external 32KHz low power clock, wait until ready */
	OSC.XOSCCTRL = OSC_X32KLPM_bm | OSC_XOSCSEL_32KHz_gc;
	OSC.CTRL |= OSC_XOSCEN_bm;
	while (!(OSC.STATUS & OSC_XOSCRDY_bm)) {};


	clock_speed();


	/* Enable RTC clock source 32KHz clock */
	CLK.RTCCTRL = CLK_RTCSRC_TOSC32_gc | CLK_RTCEN_bm;
	
	/* enable power for RTC */
	PR.PRGEN &= ~PR_RTC_bm;
	
	/* stop RTC counter */
	while (RTC.STATUS & RTC_SYNCBUSY_bm) {}
	RTC.CTRL = 0b00000000;

	/* reset counter */
	while (RTC.STATUS & RTC_SYNCBUSY_bm) {}
	RTC.CNT = 0;

	/* set counter period, 1 min */
	while (RTC.STATUS & RTC_SYNCBUSY_bm) {}
	RTC.PER = 30720;

	/* set RTC prescaler, RTC clock / 64 */
	while (RTC.STATUS & RTC_SYNCBUSY_bm) {}
	RTC.CTRL = 0b00000101;

	/* enable RTC overflow interrupt */
	RTC.INTCTRL=0b00000011;
	
	/* enable power for timer D0*/
	PR.PRPD &= ~PR_TC0_bm;
	
	/* set timer D0 to count periph clk */
	TCD0.CTRLA = 0b00000000;
	TCD0.CTRLB = 0b00000000;
	TCD0.CTRLD = 0b00000000;
	TCD0.CTRLE = 0b00000000;
	TCD0.INTCTRLA = 0b00000000;
	TCD0.INTCTRLB = 0b00000000;
	TCD0.CNT = 0;
	TCD0.PER = 0xffff;
	
	/* Prescaler: Clk/64 */
	TCD0.CTRLA = 0b00000101;
	TCD0.INTCTRLB &= ~0b00110000;		// disable CCC interrupt
	TCD0.CCC = TCD0.CNT + TIMER_FREQ;
	TCD0.INTCTRLB |= 0b00010000;		// enable CCC interrupt


	
	/* restore interrupt state */
	SREG = saved_sreg;
}


static void clock_speed(void) {
		/* Enable internal 2MHz clock, wait until ready */
		OSC.CTRL |= OSC_RC2MEN_bm;
		while (!(OSC.STATUS & OSC_RC2MRDY_bm)) {};

		/* Calibrate internal 2MHz clock on 32KHz external clock */
		OSC.DFLLCTRL |= OSC_RC2MCREF_bm;
		DFLLRC2M.CTRL |= DFLL_ENABLE_bm;

		/* Enable PLL0 (driven by 2MHz internal calibrated clock), wait until ready */
		if (!(OSC.STATUS & OSC_PLLRDY_bm)) {
			OSC.PLLCTRL = OSC_PLLSRC_RC2M_gc | (12 << OSC_PLLFAC_gp);
			OSC.CTRL |= OSC_PLLEN_bm;
			while (!(OSC.STATUS & OSC_PLLRDY_bm));
		}

		/* System clock is using PLL0, driven by 2MHz internal calibrated clock */
		ccp_write_IO((uint8_t *)&CLK.CTRL, CLK_SCLKSEL_PLL_gc);
}

speed_t clock_get(void) {
	return sysSpeed;
}

void add_second_expired_listener(void (*listener)(void))
{
	if(second_expired_listeners[0] == NULL)
	{
		second_expired_listeners[0] = listener;
	}
	else if(second_expired_listeners[1] == NULL)
	{
		second_expired_listeners[1] = listener;
	}
	else if(second_expired_listeners[2] == NULL)
	{
		second_expired_listeners[2] = listener;
	}
}

void add_milisecond_expired_listener(void (*listener)(void))
{
	if(milisecond_expired_listeners[0] == NULL)
	{
		milisecond_expired_listeners[0] = listener;
	}
	else
	{
		milisecond_expired_listeners[1] = listener;
	}
}

void my_callback_os_timer(void)
{
	if(milisecond_expired_listeners[0]) milisecond_expired_listeners[0]();
	if(milisecond_expired_listeners[1]) milisecond_expired_listeners[1]();
	
	movement_sensor_milisecond_listener();
			
	if (timer_usbChangeStateTime)
	timer_usbChangeStateTime--;
	
	if(wifi_timeout)
	{
		if (wifi_timeout_timer >= wifi_timeout)
		{
			wifi_timeout_flag = true;
		}
		else
		{
			wifi_timeout_timer++;
		}
	}
	else
	{
		wifi_timeout_flag = false;
	}
	
	if (++oneSecondCount >= ONE_SECOND_PERIOD)
	{		
		if(second_expired_listeners[0]) second_expired_listeners[0]();
		if(second_expired_listeners[1]) second_expired_listeners[1]();
		oneSecondCount = 0;	
	}
	if (timer_auxTimeout)
	{
		if (--timer_auxTimeout == 0)
		auxTimeout_flag = true;
	}
}

ISR(TCD0_CCC_vect) {
	TCD0.CCC = TCD0.CCC + TIMER_FREQ;		// 1 msec. interval
	my_callback_os_timer();
}

void start_auxTimeout(uint16_t time)
{
	register8_t saved_sreg = SREG;
	cli();
	timer_auxTimeout = time;
	auxTimeout_flag = false;
	SREG = saved_sreg;
}

bool read_auxTimeout(void)
{
	return auxTimeout_flag;
}

void usb_change_state(uint16_t time)
{
	register8_t saved_sreg = SREG;
	cli();
	timer_usbChangeStateTime = time;
	SREG = saved_sreg;
}


bool read_usb_change_state(void)
{
	return (timer_usbChangeStateTime != 0);
}

void schedule_wifi_timeout(uint16_t period)
{
	wifi_timeout = period;
	wifi_timeout_timer = 0;
	wifi_timeout_flag = false;
}

bool is_wifi_timeout(void)
{
	return wifi_timeout_flag;
}