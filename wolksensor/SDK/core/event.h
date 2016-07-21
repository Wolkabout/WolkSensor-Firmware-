/*
 * event.h
 *
 * Created: 1/22/2015 11:02:26 AM
 *  Author: btomic
 */ 

#ifndef EVENT_H_
#define EVENT_H_

#include "circular_buffer.h"
#include "platform_specific.h"
#include "chrono.h"
#include "actuators.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef union
{
	actuator_state_t actuator_state;
}
event_argument_t;

typedef struct 
{
	uint8_t type;
	uint32_t timestamp;
	event_argument_t argument;
} 
event_t;

#ifdef __cplusplus
}
#endif

#endif /* EVENT_H_ */
