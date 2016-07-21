/*
 * wiSensorsStateMachine.h
 *
 * Created: 1/22/2015 9:43:04 AM
 *  Author: btomic
 */ 

#ifndef STATEMACHINE_H_
#define STATEMACHINE_H_

#include "platform_specific.h"
#include "event_buffer.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define	EVENT_ENTERING_STATE 254
#define	EVENT_LEAVING_STATE 255

struct state_machine_state;

typedef bool (*state_machine_state_handler)(struct state_machine_state* state, event_t* event);

typedef struct state_machine_state
{
	int8_t id;
	const char* human_readable_name;
	struct state_machine_state* parent;
	int8_t current_state;
	state_machine_state_handler handler;
}
state_machine_state_t;

void state_machine_init_state(int8_t id, const char* human_readable_name, state_machine_state_t* parent, state_machine_state_t* states, int8_t initial_state, state_machine_state_handler handler);

/*
* Handles event in current state.
*/
bool state_machine_process_event(state_machine_state_t* states, state_machine_state_t* state, event_t* event);

/**
* Changes the state of state machine.
*/
bool state_machine_transition(state_machine_state_t* states, state_machine_state_t* state, uint8_t new_state_id);

uint16_t get_state_human_readable_name(state_machine_state_t* states, state_machine_state_t* state, char *buffer, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif /* WISENSORSSTATEMACHINE_H_ */
