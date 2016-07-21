/*
 * circularBuffer.h
 *
 * Created: 1/22/2015 10:08:43 AM
 *  Author: btomic
 */ 

#ifndef CIRCULARBUFFER_H_
#define CIRCULARBUFFER_H_

#include "platform_specific.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct 
{
	uint16_t head; /* points to where the first element is */
	uint16_t tail; /* points to where next element will be inserted (after last element) */
	void* storage; /* pointer to array where values are actually stored */
	uint16_t storage_size; /* size of the storage array */
	uint16_t element_size; /* size of buffer element obtained with sizeof() */
	bool empty; /* set when buffer is empty. Initially should be set to true. */
	bool full; /* set when buffer is full. Initially should be set to false. */
	bool wrap; /* should buffer overwrite oldest values if there is no more free space to store new values */
} 
circular_buffer_t;

void circular_buffer_init(circular_buffer_t* circular_buffer, void* storage, uint16_t storage_size, uint16_t element_size, bool wrap, bool clear);

/**
 * Adds element to buffer. If buffer is full it will overwrite the oldest element.
*/
bool circular_buffer_add(circular_buffer_t* buffer, void* element);

/**
 * Adds array of elements to buffer. If buffer is full it will overwrite the oldest element.
*/
bool circular_buffer_add_array(circular_buffer_t* buffer, const void* elements_array, uint16_t length);

/**
 * Adds array of elements to buffer. If buffer is full it will overwrite the oldest element.
*/
uint16_t circular_buffer_add_as_many_as_possible(circular_buffer_t* buffer, const void* elements_array, uint16_t length);

/**
 * Reads element from head position in buffer and removes it. Returns false if buffer is empty, true otherwise.
*/
bool circular_buffer_pop(circular_buffer_t* buffer, void* element);

/**
 * Reads length elements starting at element_position into elements_array and removes them. 
 * Returns number of read elements.
*/
uint16_t circular_buffer_pop_array(circular_buffer_t* buffer, uint16_t length, void* elements_array);

/**
 * Drops first number_of_elements.
*/
uint16_t circular_buffer_drop_from_beggining(circular_buffer_t* buffer, uint16_t number_of_elements);

/**
 * Drops first number_of_elements.
*/
uint16_t circular_buffer_drop_from_end(circular_buffer_t* buffer, uint16_t number_of_elements);

/**
 * Reads element at given position without removing it. 
 * Returns false if position is larger than number of elements in buffer or buffer is empty, true otherwise.
*/
bool circular_buffer_peek(circular_buffer_t* buffer, uint16_t element_position, void* element);

/**
 * Reads length elements starting at element_position into elements_array without removing them. 
 * Returns number of read elements.
*/
uint16_t circular_buffer_peek_array(circular_buffer_t* buffer, uint16_t element_position, uint16_t length, void* elements_array);

bool circular_buffer_empty(circular_buffer_t* buffer);

bool circular_buffer_full(circular_buffer_t* buffer);

uint16_t circular_buffer_size(circular_buffer_t* buffer);

uint16_t circular_buffer_free_space(circular_buffer_t* buffer);

/**
 * Clears the entire buffer.
*/
void circular_buffer_clear(circular_buffer_t* buffer);

#ifdef __cplusplus
}
#endif

#endif /* CIRCULARBUFFER_H_ */
