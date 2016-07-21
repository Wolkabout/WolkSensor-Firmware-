#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "sensor.h"
#include "sensors.h"
#include "Sensors/batteryADC.h"
#include "Sensors/BME280.h"
#include "Sensors/LSM303.h"
#include "config.h"
#include "event_buffer.h"
#include "sensor_readings_buffer.h"
#include "logger.h"
#include "platform_specific.h"


TWI_Master_t sensor_twi;    /*!< TWI master module. */

ISR(TWIC_TWIM_vect) {
	TWI_MasterInterruptHandler(&sensor_twi);
}

void init_sensors(void)
{
	LOG(1, "Sensors init");
		
	register8_t saved_sreg = SREG;
	cli();

	/* enable power for TWIC */
	PR.PRPC &= ~PR_TWI_bm;
	TWI_MasterInit(&sensor_twi, &TWIC, TWI_MASTER_INTLVL_MED_gc, TWI_MasterBaud(CLK_24MHZ));

	SREG = saved_sreg;
	
	LSM303_init();
	BME280_init();
	batteryADC_init();
}

static bool get_pressure(int16_t *value)
{	
	*value = BME280_GetPressure()*10;
		
	return true;
}

static bool get_temperature(int16_t *value)
{	
	*value = BME280_GetTemperature()*10;
	
	return true;
}

static bool get_humidity(int16_t *value)
{
	*value = BME280_GetHumidity()*10;
	
	return true;
}

bool get_sensors_states(char* sensors_ids, uint8_t sensors_count)
{
	LOG(1, "Getting sensor values");
	
	sensor_twi.interface->MASTER.BAUD = TWI_MasterBaud(CLK_24MHZ);

	BME280_poll();
	
	while (sensor_twi.status != TWIM_STATUS_READY) {}
		
	sensor_state_t atmo_sensors_states[NUMBER_OF_SENSORS];
	
	for(uint8_t i = 0; i < sensors_count; i++)
	{
		switch(sensors_ids[i])
		{
			case 'P':
			{
				LOG(1, "Getting pressure");
				
				atmo_sensors_states[i].id = 'P';
				if(!get_pressure(&atmo_sensors_states[i].value))
				{
					atmo_sensors_states[i].value = 0; 
				}
				break;
			}
			case 'T':
			{
				LOG(1, "Getting temperature");
				
				atmo_sensors_states[i].id = 'T';
				if(!get_temperature(&atmo_sensors_states[i].value))
				{
					atmo_sensors_states[i].value = 2000;
				}
				
				break;
			}
			case 'H':
			{
				LOG(1, "Getting humidity");
				
				atmo_sensors_states[i].id = 'H';
				if(!get_humidity(&atmo_sensors_states[i].value))
				{
					atmo_sensors_states[i].value = 0;
				}
				
				break;
			}
		}
	}
	
	if(sensors_states_listener) sensors_states_listener(atmo_sensors_states, sensors_count);
	
	return true;
}
