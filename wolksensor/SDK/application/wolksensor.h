#ifndef WOLKSENSOR_H_
#define WOLKSENSOR_H_

#include "platform_specific.h"
#include "state_machine.h"

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