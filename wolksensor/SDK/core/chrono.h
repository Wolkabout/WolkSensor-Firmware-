#ifndef CHRONO_H_
#define CHRONO_H_

#include "platform_specific.h"

#ifdef __cplusplus
extern "C"
{
#endif

void chrono_init(bool reset);
uint32_t rtc_get_ts(void);
void rtc_set(uint32_t time);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_H_ */
