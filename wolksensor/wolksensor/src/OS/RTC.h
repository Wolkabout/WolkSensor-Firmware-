#ifndef RTC_H_
#define RTC_H_

#include "chrono.h"
#include "clock.h"

void RTC_init(bool cold_boot);
void add_minute_expired_listener(void (*)(void));
uint32_t rtc_get(void);

#endif /* RTC_H_ */