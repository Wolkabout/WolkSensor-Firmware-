/*****************************************************************************/
/* Battery management module                                                 */
/*                                                                           */
/*****************************************************************************/


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "clock.h"
#include "RTC.h"
#include "brd.h"
#include "test.h"
#include "logger.h"
#include "event_buffer.h"

#include "Sensors/batteryADC.h"

#include <util/delay.h>
#include "config/conf_os.h"

static volatile bool power_state=false;
static bool batteryMinVoltageMonitor=false;
static uint16_t batteryMinVoltage=0;

void usb_change_state(uint16_t time);
bool read_usb_change_state(void);

static void (*usb_state_change_listener)(bool usb_state) = NULL;
static void (*battery_voltage_listener)(uint16_t voltage) = NULL;

static uint16_t battery_voltage(void) {
	ADCA.REFCTRL = 0b00000010;			// enable voltage reference
	ADCA.CTRLA = 0b00000001;			// enable ADC

	ADCA.CH0.CTRL = 0b10000001;			//start AD conversion, single ended, gain=1
	while (!(ADCA.CH0.INTFLAGS & 0x01));
	ADCA.CH0.INTFLAGS = 0x01;

	ADCA.CH0.CTRL = 0b10000001;
	while (!(ADCA.CH0.INTFLAGS & 0x01));
	ADCA.CH0.INTFLAGS = 0x01;

	uint16_t battVoltage = ADCA.CH0.RES;

	ADCA.REFCTRL = 0b00000000;			// disable voltage reference
	ADCA.CTRLA = 0b00000000;			// disable ADC

	return battVoltage;
}

bool batteryADC_init(void) {
	
	PR.PRPA &= ~PR_ADC_bm;				// enable ADC

	uint16_t cal;
	MSB(cal) = ReadSignatureByte(offsetof(NVM_PROD_SIGNATURES_t, ADCACAL1));
	LSB(cal) = ReadSignatureByte(offsetof(NVM_PROD_SIGNATURES_t, ADCACAL0));
	ADCA.CALL = cal;					//read and write calibration

	ADCA.CTRLB = 0b01110000;			// current limit, signed, 12-bit right adjusted
	
	ADCA.PRESCALER = 0b00000101;		// prescaler 101 = 128 -> 187.5kHz
	
	ADCA.CH0.MUXCTRL = 0b00011000;

	/* USB_SELECT interupt*/
	PORTE.INT0MASK = 0b00000100;		// USB_SELECT is in int0 group
	PORTE.PIN2CTRL = 0b00010001;		// pull-down, detect rising edge
	PORTE.INTFLAGS = 0b00000001;		// clear pending int0 interrupts
	PORTE.INTCTRL |= 0b00000011;		// interrupt 0 enabled

	batteryADC_battery(&batteryMinVoltage);
	
	power_state = USB_ON_STATE ? true : false;
	
	LOG_PRINT(1, PSTR("Initial USB state is %u\r\n"), power_state);
	
	return true;
}

void enable_voltage_monitor(void)
{
	batteryMinVoltage_start();
}

void disable_voltage_monitor(void)
{
	batteryMinVoltage_stop();
}

void batteryMinVoltage_start(void)
{
	batteryMinVoltage=0;
	batteryMinVoltageMonitor=true;
	ADCA.CTRLA = 0b00000001;			// enable ADC
	ADCA.REFCTRL = 0b00000010;			// enable voltage reference
	
	_delay_ms(3);						//Propagation Delay
	
	ADCA.CH0.INTCTRL = 0b00000011;		// interrupt enabled - high priority

	ADCA.CH0.CTRL = 0b10000001;			//start AD conversion, single ended, gain=1
}

void batteryMinVoltage_stop(void)
{
	batteryMinVoltageMonitor=false;
	ADCA.CH0.INTCTRL = 0b00000000;		// interrupt disabled

	ADCA.REFCTRL = 0b00000000;			// disable voltage reference
	ADCA.CTRLA = 0b00000000;			// disable ADC
}

uint16_t batteryMinVoltage_read(void)
{
	//LOG_PRINT(2, PSTR("battery Min voltagex100 %d\r\n"), batteryMinVoltage);
	//LOG_PRINT(2, PSTR("number of min voltage polls %d\r\n"), numberOFMinVoltagePolls);
	return batteryMinVoltage;
}

void batteryADC_poll(void)
{
	//LOG_PRINT(2, PSTR("battery Min voltagex100 %d\r\n"), batteryMinVoltage);
	//return batteryMinVoltage;
}

bool batteryADC_powerOK(void) {
	return true;
}

bool batteryADC_battery(uint16_t *value) {
	if(batteryMinVoltageMonitor){
		*value=batteryMinVoltage;
		return false;
	}
	else {
		uint32_t batt_Voltagex100=battery_voltage();
		batt_Voltagex100 = (batt_Voltagex100*RESISTOR_DIVIDER ) / ADC_MAX_VALUE;
		*value = batt_Voltagex100;
		return true;
	}
}

void add_usb_state_change_listener(void (*listener)(bool usb_state))
{
	usb_state_change_listener = listener;
}

bool poll_usb_state(void)
{
	if (USB_ON_STATE && (power_state == false) && !read_usb_change_state())
	{
		LOG(1, "Poll USB ON");
		usb_change_state(USB_CHANGE_STATE_TIME);
		power_state = true;
		if(usb_state_change_listener) usb_state_change_listener(true);
	}
	else if(!USB_ON_STATE && (power_state == true) && !read_usb_change_state())
	{
		LOG(1, "Poll USB OFF");
		usb_change_state(USB_CHANGE_STATE_TIME);
		power_state = false;
		if(usb_state_change_listener) usb_state_change_listener(false);
		
		return true;
	}
	
	return power_state || (read_usb_change_state() != 0);
}

void my_callback_USB_state(power_t state)
{
}

bool power_usb_state(void)
{
	return USB_ON_STATE ? true : false;
}

bool get_usb_state(void)
{
	return power_usb_state();
}

//USB interrupt
ISR(PORTE_INT0_vect)
{
	LOG(1, "USB interrupt");
	if(!power_state && !read_usb_change_state())
	{
		LOG(1, "Interrupt USB ON");
		power_state = true;
		usb_change_state(USB_CHANGE_STATE_TIME);
		if(usb_state_change_listener) usb_state_change_listener(true);
	}
}

//ADC interrupt
ISR(ADCA_CH0_vect){
	if(batteryMinVoltageMonitor){
		uint32_t batt_Voltagex100 = ADCA.CH0.RES;
		if(batt_Voltagex100 > ADC_MAX_VALUE) batt_Voltagex100 = ADC_MAX_VALUE;
		
		batt_Voltagex100 = (batt_Voltagex100*RESISTOR_DIVIDER ) / ADC_MAX_VALUE;
		batt_Voltagex100 += VOLTAGE_MEASUREMENT_OFFSET;
		
		if(power_state) batt_Voltagex100 += VOLTAGE_DROPOUT_SERIES_DIODE;
		
		if(battery_voltage_listener) battery_voltage_listener(batt_Voltagex100);
		
		ADCA.CH0.CTRL = 0b10000001;			//start AD conversion, single ended, gain=1
	}
}

void add_battery_voltage_listener(void (*listener)(uint16_t voltage))
{
	battery_voltage_listener = listener;
}