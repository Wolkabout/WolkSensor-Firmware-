#include "state_machine.h"
#include "chrono.h"
#include "config.h"
#include "logger.h"
#include "sensors.h"
#include "global_dependencies.h"

static uint32_t RTC_offset NO_INIT_MEMORY;

void chrono_init(bool reset)
{
	if(reset)
	{
		RTC_offset = 0x5C890E99;//13rd March 2019.
	}
}

uint32_t rtc_get_ts(void)
{
	return (RTC_offset + global_dependencies.rtc_get());
}

void rtc_set(uint32_t time)
{
	RTC_offset = (time - global_dependencies.rtc_get());
}
