/*
 * eventBuffer.c
 *
 * Created: 1/22/2015 10:58:08 AM
 *  Author: btomic
 */ 

#include "event_buffer.h"
#include "event.h"
#include "logger.h"

void add_event_type(circular_buffer_t* event_buffer, uint8_t event_type)
{
	SYNCHRONIZED_BLOCK_START
	event_t event;
	event.type = event_type;
	
	event.timestamp = rtc_get_ts();
	circular_buffer_add(event_buffer, &event);

	SYNCHRONIZED_BLOCK_END
}

void add_event(circular_buffer_t* event_buffer, event_t* event)
{
	SYNCHRONIZED_BLOCK_START
	
	event->timestamp = rtc_get_ts();
	circular_buffer_add(event_buffer, event);
	
	SYNCHRONIZED_BLOCK_END
}

bool pop_event(circular_buffer_t* event_buffer, event_t* event)
{
	SYNCHRONIZED_BLOCK_START

	bool event_read = circular_buffer_pop(event_buffer, event);

	SYNCHRONIZED_BLOCK_END

	return event_read;
}

void clear_event_buffer(circular_buffer_t* event_buffer)
{
	SYNCHRONIZED_BLOCK_START

	circular_buffer_clear(event_buffer);

	SYNCHRONIZED_BLOCK_END
}

uint16_t events_count(circular_buffer_t* event_buffer)
{
	return circular_buffer_size(event_buffer);
}
