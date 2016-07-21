#include "state_machine.h"
#include "logger.h"

void state_machine_init_state(int8_t id, const char* human_readable_name, state_machine_state_t* parent, state_machine_state_t* states, int8_t initial_state, state_machine_state_handler handler)
{
	states[id].id = id;
	states[id].human_readable_name = human_readable_name;
	states[id].parent = parent;
	states[id].current_state = initial_state;
	states[id].handler = handler;
}

bool state_machine_process_event(state_machine_state_t* states, state_machine_state_t* state, event_t* event)
{
	bool event_processed  = false;

	if(state->current_state != -1)
	{
		event_processed = state_machine_process_event(states, &states[state->current_state], event);
	}

	/* child state does not exist or did not process the event */
	if(!event_processed && state->handler)
	{
		event_processed = state->handler(state, event);
	}

	return event_processed;
}

bool state_machine_transition(state_machine_state_t* states, state_machine_state_t* state, uint8_t new_state_id)
{
	state_machine_state_t* new_state = &states[new_state_id];

	event_t event;
	event.type = EVENT_LEAVING_STATE;

	if(new_state->parent->current_state != -1)
	{
		state_machine_state_t* leaving_state = &states[new_state->parent->current_state];
		state_machine_process_event(states, leaving_state, &event);
	}

	new_state->parent->current_state = new_state_id;

	event.type = EVENT_ENTERING_STATE;

	if(new_state->handler)
	{
		new_state->handler(new_state, &event);
	}
	while(new_state->current_state != -1)
	{
		new_state = &states[new_state->current_state];
		if(new_state->handler)
		{
			new_state->handler(new_state, &event);
		}
	}

	return true;
}

uint16_t get_state_human_readable_name(state_machine_state_t* states, state_machine_state_t* state, char *buffer, uint16_t size)
{
	uint16_t parent_name_size = 0;
	if(state->parent)
	{
		parent_name_size = get_state_human_readable_name(states, state->parent, buffer, size);
		if(parent_name_size)
		{
			buffer[parent_name_size++] = '-';
		}
	}
		
	int my_name_size = state->human_readable_name ? sprintf_P(buffer + parent_name_size, state->human_readable_name) : 0;
	
	return parent_name_size + my_name_size;
}
