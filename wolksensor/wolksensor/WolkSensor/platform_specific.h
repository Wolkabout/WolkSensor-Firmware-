#ifndef PLATFORM_SPECIFIC_H_
#define PLATFORM_SPECIFIC_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <compiler.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <math.h>

#define FW_VERSION_MAJOR 4 // number 0 -99
#define FW_VERSION_MINOR 3 // number 0 -99
#define FW_VERSION_PATCH 6 // number 0 -99

#define SYNCHRONIZED_BLOCK_START register8_t saved_sreg = SREG; cli();
#define SYNCHRONIZED_BLOCK_END SREG = saved_sreg;

#define WIFI_SECURITY_UNSECURED		0
#define WIFI_SECURITY_WEP			1
#define WIFI_SECURITY_WPA2			2
#define WIFI_SECURITY_WPA			3

#define NO_INIT_MEMORY __attribute__ ((section (".noinit2")))

#define MAX_BUFFER_SIZE 768

#define NUMBER_OF_ACTUATORS 0

#define NUMBER_OF_SENSORS 4

#define LOG_FORMAT "%S\r\n"

#endif /* PLATFORM_SPECIFIC_H_ */
