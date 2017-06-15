#ifndef SENSOR_H_
#define SENSOR_H_

#include <stdbool.h>
#include <avr/io.h>

#include "clock.h"
#include "twi.h"

bool get_sensors_states(char* sensors_ids, uint8_t sensors_count);

extern TWI_Master_t sensor_twi;
	
#endif /* SENSOR_H_ */
