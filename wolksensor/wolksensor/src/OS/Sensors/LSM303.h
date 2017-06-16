#ifndef LSM303_H_
#define LSM303_H_

#include "sensors.h"

extern void (*sensors_states_listener)(sensor_state_t* sensors_states, uint8_t sensors_count);

void movement_sensor_disable(void);
void movement_sensor_enable(void);

void movement_sensor_milisecond_listener(void);

void add_sensors_states_listener(void (*listener)(sensor_state_t* sensors_states, uint8_t sensors_count));

bool LSM303_init(void);
void LSM303_poll(void);

bool waiting_movement_sensor_enable_timeout(void);

bool disable_movement(void);
bool enable_movement(void);

#endif /* LSM303_H_ */
