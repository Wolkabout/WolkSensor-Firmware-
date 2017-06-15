#ifndef WOLKSENSOR_H_
#define WOLKSENSOR_H_

#include "platform_specific.h"
#include "state_machine.h"


#define EVENTS_BUFFER_SIZE 10
#define COMMANDS_BUFFER_SIZE 10
#define COMMAND_STRING_BUFFER_SIZE MAX_BUFFER_SIZE
#define COMMAND_RESPONSE_BUFFER_SIZE MAX_BUFFER_SIZE
#define MAX_ALARM_RETRIES 2
#define MAX_NO_CONNECTION_HEARTBEAT	60
#define COMMUNICATION_MODULE_MINIMUM_REQUIRED_VOLTAGE 280

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum 
{
	POWER_ON = 0,
	BROWNOUT_RESET,
	WATCHDOG_RESET
}
start_type_t;

void init_wolksensor(start_type_t start_type);
bool wolksensor_process(void);

#ifdef __cplusplus
}
#endif

#endif /* WOLKSENSOR_H_ */