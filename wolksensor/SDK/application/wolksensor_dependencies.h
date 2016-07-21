#ifndef WOLKSENSOR_DEPENDENCIES_H_
#define WOLKSENSOR_DEPENDENCIES_H_

#include "platform_specific.h"
#include "sensors.h"
#include "actuators.h"

typedef struct
{
	bool (*get_sensors_states)(char* sensors_ids, uint8_t sensors_count);
	void (*enable_battery_voltage_monitor)(void);
	void (*disable_battery_voltage_monitor)(void);
	void (*system_reset)(void);
	bool (*get_usb_state)(void);
	void (*enable_movement)(void);
	void (*disable_movement)(void);
	
	void (*add_minute_expired_listener)(void (*listener)(void));
	void (*add_second_expired_listener)(void (*listener)(void));
	void (*add_usb_state_change_listener)(void (*listener_t)(bool usb_state));
	void (*add_command_data_received_listener)(void (*listener)(char *data, uint16_t length));
	void (*add_battery_voltage_listener)(void (*listener)(uint16_t voltage));
	void (*add_sensors_states_listener)(void (*listener)(sensor_state_t* sensors_states, uint8_t sensors_count));
}
wolksensor_dependencies_t;

extern wolksensor_dependencies_t wolksensor_dependencies; 

#endif /* WOLKSENSOR_DEPENDENCIES_H_ */