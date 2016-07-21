#ifndef CLOCK_H_
#define CLOCK_H_

typedef enum { CLK_UNKNOWN, CLK_32KHZ, CLK_2MHZ, CLK_24MHZ } speed_t;

void clock_init(void);
speed_t clock_get(void);

bool read_usb_change_state(void);
void start_auxTimeout(uint16_t time);
bool read_auxTimeout(void);

void schedule_wifi_timeout(uint16_t period);
bool is_wifi_timeout(void);

void add_milisecond_expired_listener(void (*listener)(void));
void add_second_expired_listener(void (*listener)(void));

#endif /* CLOCK_H_ */