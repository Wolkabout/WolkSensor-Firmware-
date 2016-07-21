#ifndef ACTUATORS_H_
#define ACTUATORS_H_

#include "platform_specific.h"
#include "sensor_readings_buffer.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define ACTUATOR_ID_MAX_LENGTH 10

typedef enum
{
	ACTUATOR_TYPE_SWITCH = 0,
	ACTUATOR_TYPE_DC_MOTOR,
	ACTUATOR_TYPE_SERVO
}
actuator_type_t;

typedef enum
{
	ACTUATOR_NOT_REPORTED,
	ACTUATOR_REPORTING_IN_PROGRESS,
	ACTUATOR_REPORTED,
	ACTUATOR_REPORTING_FAILED
}
actuator_reporting_state_t;

typedef enum
{
	ACTUATOR_STATUS_OK = 0,
	ACTUATOR_STATUS_IN_PROGRESS,
	ACTUATOR_STATUS_FAILED
}
actuator_status_t;

typedef union
{
	bool switch_value;
	int32_t dc_motor_value;
	int16_t servo_value;
}
actuator_value_t;

typedef struct  
{
	char id[ACTUATOR_ID_MAX_LENGTH];
	actuator_type_t type;
	actuator_value_t target_value;
	actuator_reporting_state_t reporting_state;
}
actuator_t;

typedef struct
{
	char id[ACTUATOR_ID_MAX_LENGTH];
	actuator_value_t value;
	actuator_status_t status;
}
actuator_state_t;

extern actuator_t actuators[NUMBER_OF_ACTUATORS];

int8_t get_index_of_actuator(char* id);

#ifdef __cplusplus
}
#endif

#endif /* ACTUATORS_H_ */
