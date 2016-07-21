/*
 * eventBuffer.h
 *
 * Created: 1/22/2015 10:57:51 AM
 *  Author: btomic
 */ 

#ifndef EVENTBUFFER_H_
#define EVENTBUFFER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "circular_buffer.h"
#include "platform_specific.h"
#include "event.h"

void add_event_type(circular_buffer_t* event_buffer, uint8_t event_type);
void add_event(circular_buffer_t* event_buffer, event_t* event);
bool pop_event(circular_buffer_t* event_buffer, event_t* event);
void clear_event_buffer(circular_buffer_t* event_buffer);
uint16_t events_count(circular_buffer_t* event_buffer);

#ifdef __cplusplus
}
#endif

#endif /* EVENTBUFFER_H_ */
