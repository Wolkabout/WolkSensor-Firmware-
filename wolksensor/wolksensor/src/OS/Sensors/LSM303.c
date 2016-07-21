#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "clock.h"
#include "RTC.h"
#include "twi.h"
#include "chrono.h"
#include "clock.h"
#include "test.h"
#include "logger.h"
#include "sensors.h"
#include "config.h"
#include "Sensors/LSM303.h"

#include "config/conf_os.h"

#include "event_buffer.h"

#define MOVEMENT_SENSOR_DISABLED_TIME 2000 // ms

static volatile uint16_t movement_sensor_disabled_timeout = 0;

static bool sensor_detected = false;
static bool sensor_ready = false;

static uint8_t twi_buff[8];

static volatile int	compass_X;
static volatile int	compass_Y;
static volatile int	compass_Z;

void (*sensors_states_listener)(sensor_state_t* sensors_states, uint8_t sensors_count) = NULL;

bool LSM303_init(void) {
	bool  timeout=false;
	start_auxTimeout(10);
	while ((sensor_twi.status != TWIM_STATUS_READY) && !timeout) {timeout=read_auxTimeout();}
	if(timeout) return false;
	LOG(3,"LSM303 - 1");
	twi_buff[0] = 0x80 + 0x20;
	twi_buff[1] = 0b01010111;		// REG1, 100Hz, enable XYZ
	twi_buff[2] = 0b00110000;		// REG2, High pass filter, cut-off 0.125Hz, don't use filtered data
	twi_buff[3] = 0b01000000;		// REG3, AOI1 signal on INT1
	twi_buff[4] = 0b00000000;		// REG4, full scale 2G
	twi_buff[5] = 0b00001000;		// REG5, latched interrupt on INT1
	twi_buff[6] = 0b00000000;		// REG6. interrupt active high
	twi_buff[7] = 0b00000001;		// REFERENCE_A
	TWI_MasterWriteRead(&sensor_twi, 0x19, twi_buff, 8, 0);
	start_auxTimeout(10);
	while ((sensor_twi.status != TWIM_STATUS_READY) && !timeout) {timeout=read_auxTimeout();}
	if(timeout || (sensor_twi.result != TWIM_RESULT_OK)) return false;
	LOG(3,"LSM303 - 2");
	twi_buff[0] = 0x80 + 0x30;
	twi_buff[1] = 0b01111111;		// INT1_CFG_A, XYZ high interrupt
	TWI_MasterWriteRead(&sensor_twi, 0x19, twi_buff, 2, 0);
	start_auxTimeout(10);
	while ((sensor_twi.status != TWIM_STATUS_READY) && !timeout) {timeout=read_auxTimeout();}
	if(timeout || (sensor_twi.result != TWIM_RESULT_OK)) return false;
	LOG(3,"LSM303 - 3");
	twi_buff[0] = 0x80 + 0x32;
	twi_buff[1] = 0b01001101;		// INT1_THS, threshold
	twi_buff[2] = 0b00000001;		// INT1_DURATION, duration
	TWI_MasterWriteRead(&sensor_twi, 0x19, twi_buff, 3, 0);
	start_auxTimeout(10);
	while ((sensor_twi.status != TWIM_STATUS_READY) && !timeout) {timeout=read_auxTimeout();}
	if(timeout || (sensor_twi.result != TWIM_RESULT_OK)) return false;

	sensor_detected = true;

	// enable external rising edge interrupt for LSM303
	PORTB.DIR &= 0b11111011;
	PORTB.INT0MASK = 0b00000100;		// LSM303 interrupt is in int0 group
	PORTB.PIN2CTRL = 0b00010001;		// pull-down, detect rising edge
	PORTB.INTFLAGS = 0b00000001;		// clear pending int0 interrupts
	PORTB.INTCTRL |= 0b00000011;		// interrupt 0, high level
	LOG(1,"LSM303 detected");
	
	movement_sensor_enable();
	
	return true;
}


void LSM303_poll(void) 
{
	LOG(2,"LSM poll start");
	
	if (!sensor_detected){
		LOG(1,"LSM not processed");
		return;
	}
	
	bool  timeout=false;
	start_auxTimeout(10);
	while ((sensor_twi.status != TWIM_STATUS_READY) && !timeout) {timeout=read_auxTimeout();}
	if(timeout) return;
	
	LOG(2,"LSM poll 1");

	twi_buff[0] = 0x80 + 0x31;
	TWI_MasterWriteRead(&sensor_twi, 0x19, twi_buff, 1, 1);
	start_auxTimeout(10);
	while ((sensor_twi.status != TWIM_STATUS_READY) && !timeout) {timeout=read_auxTimeout();}
	if(timeout || (sensor_twi.result != TWIM_RESULT_OK)) return;
	LOG(1,"LSM303 ready");
	
	sensor_ready = true;
}

ISR(PORTB_INT0_vect) 
{
	LOG(2,"LSM303 interrupt !!!!");
	if (sensor_detected && sensor_ready)
	{
		LOG(1,"LSM303 moved !!!!");
		
		movement_sensor_disable();
		
		movement_sensor_disabled_timeout = MOVEMENT_SENSOR_DISABLED_TIME;
		
		if(movement_status)
		{
			sensor_state_t movement_sensor_state;
			movement_sensor_state.id = 'M';
			movement_sensor_state.value = 1;
			
			if(sensors_states_listener) sensors_states_listener(&movement_sensor_state, 1);
		}
	}
}

void movement_sensor_disable(void)
{
	sensor_ready = false;
}

void movement_sensor_enable(void)
{
	LSM303_poll();
}

void movement_sensor_milisecond_listener(void)
{
	if(movement_sensor_disabled_timeout)
	{
		if((--movement_sensor_disabled_timeout == 0))
		{
			LOG(1, "Movement sensor enable");
			
			movement_sensor_enable();
		}
	}
}

void add_sensors_states_listener(void (*listener)(sensor_state_t* sensors_states, uint8_t sensors_count))
{
	sensors_states_listener = listener;
}

bool waiting_movement_sensor_enable_timeout(void)
{
	return movement_status && (movement_sensor_disabled_timeout > 0);
}


void disable_movement(void)
{
	bool  timeout=false;
	
	start_auxTimeout(10);
	while ((sensor_twi.status != TWIM_STATUS_READY) && !timeout) {timeout=read_auxTimeout();}
	if(timeout) return false;
	
	LOG(1,"Disable movement sensor");
	twi_buff[0] = 0x80 + 0x20;
	twi_buff[1] = 0b00000000;	
	TWI_MasterWriteRead(&sensor_twi, 0x19, twi_buff, 2, 0);
}

void enable_movement(void)
{
	bool  timeout=false;
	
	start_auxTimeout(10);
	while ((sensor_twi.status != TWIM_STATUS_READY) && !timeout) {timeout=read_auxTimeout();}
	if(timeout) return false;
	
	LOG(1,"Enable movement sensor");
	twi_buff[0] = 0x80 + 0x20;
	twi_buff[1] = 0b01010111;	
	TWI_MasterWriteRead(&sensor_twi, 0x19, twi_buff, 2, 0);
}