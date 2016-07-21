/*
 * logger.c
 *
 * Created: 2/5/2015 4:31:26 PM
 *  Author: btomic
 */ 
#include "logger.h"
#include "commands.h"
#include "global_dependencies.h"
#include <string.h>

#ifdef LOG_ENABLED

uint8_t log_level = 2;
char buffer[MAX_BUFFER_SIZE];

void log_print_P(const char *format, ...)
{
	va_list args;
	va_start (args, format);
	
	uint16_t length = vsnprintf_P(buffer, MAX_BUFFER_SIZE, format, args);

	global_dependencies.log(buffer, length);
	
	va_end (args);
}

#endif
