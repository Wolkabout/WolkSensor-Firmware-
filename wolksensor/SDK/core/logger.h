/*
 * logger.h
 *
 * Created: 1/22/2015 11:02:26 AM
 *  Author: btomic
 */ 

#ifndef LOGGER_H_
#define LOGGER_H_

#include "chrono.h"
#include "circular_buffer.h"
#include "platform_specific.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef LOG_ENABLED

	extern uint8_t log_level;

	void log_print_P(const char *format, ...);

	#define LOG(LVL, X) { \
		if (LVL <= log_level) { \
			log_print_P(PSTR(LOG_FORMAT), PSTR(X)); \
		} \
	}

	#define LOG_PRINT(LVL, ...) { \
		if (LVL <= log_level) { \
			log_print_P(__VA_ARGS__); \
		} \
	}

	void transmit_log(char *buffer, int length);

#else

	#define LOG(LVL, X)
	#define LOG_PRINT(LVL, ...)

#endif

#ifdef __cplusplus
}
#endif

#endif /* LOGGER_H_ */
