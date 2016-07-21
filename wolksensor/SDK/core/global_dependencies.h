#ifndef GLOBAL_DEPENDENCIES_H_
#define GLOBAL_DEPENDENCIES_H_

#include "platform_specific.h"

typedef struct
{
	uint32_t (*rtc_get)(void);
	// This function sends response over eg. UART.
	void (*send_response) (const char* response, uint16_t length);
	void (*log)(const char* message, uint16_t length);
	bool (*config_read)(void *data, uint8_t type, uint8_t version, uint8_t length);
	bool (*config_write)(void *data, uint8_t type, uint8_t version, uint8_t length);
}
global_dependencies_t;

extern global_dependencies_t global_dependencies;

#endif /* GLOBAL_DEPENDENCIES_H_ */
