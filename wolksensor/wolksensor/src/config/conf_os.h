#ifndef _CONF_OS_H_
#define _CONF_OS_H_

#include "Sensors/sensor.h"
#include "Sensors/batteryADC.h"



// OS callback definition

#define  OS_TIMER_EVENT()					my_callback_os_timer()
extern void my_callback_os_timer(void);

#define  OS_USB_EVENT(state)				my_callback_USB_state(state)
extern void my_callback_USB_state(power_t state);

#define  OS_BATTERY_FULL_EVENT()			my_callback_os_battery_full()
extern void my_callback_os_battery_full(void);



#define  OS_CC3000_CONFIG_DONE()
//#define  OS_CC3000_CONFIG_DONE()			my_callback_os_CC3000_config_done()
//extern void my_callback_os_CC3000_config_done(void);

#define  OS_CC3000_CONNECT()				my_callback_os_CC3000_connect()
extern void my_callback_os_CC3000_connect(void);

#define  OS_CC3000_DISCONNECT()				my_callback_os_CC3000_disconnect()
extern void my_callback_os_CC3000_disconnect(void);

#define  OS_CC3000_KEEPALIVE()
//#define  OS_CC3000_KEEPALIVE()				my_callback_os_CC3000_keepalive()
//extern void my_callback_os_CC3000_keepalive(void);

#define  OS_CC3000_INIT()
//#define  OS_CC3000_INIT()					my_callback_os_CC3000_init()
//extern void my_callback_os_CC3000_init(void);

#define  OS_CC3000_DHCP(data)				my_callback_os_CC3000_DHCP(data)
extern void my_callback_os_CC3000_DHCP(char *data);

#define  OS_CC3000_CAN_SHUT_DOWN()			my_callback_os_CC3000_can_shut_down()
extern void my_callback_os_CC3000_can_shut_down(void);

#define  OS_CC3000_PING_REPORT(data, length)
//#define  OS_CC3000_PING_REPORT(data, length)		my_callback_os_CC3000_ping_report(data, length)
//extern void my_callback_os_CC3000_ping_report(char *data, unsigned char length);

#define  OS_CC3000_TCP_CLOSE_WAIT(data) my_callback_os_CC3000_TCP_close_wait(data)
//#define  OS_CC3000_TCP_CLOSE_WAIT(data)		my_callback_os_CC3000_TCP_close_wait(data)
extern void my_callback_os_CC3000_TCP_close_wait(unsigned char data);

#define  OS_CC3000_LOOP(commandToResend, opcode)	my_callback_os_CC3000_loop(commandToResend,opcode)
extern unsigned char my_callback_os_CC3000_loop(unsigned char commandToResend, uint16_t opcode);

#define  OS_CC3000_WIFI_OK()					wifiOK()
extern unsigned char wifiOK(void);

#define  OS_CC3000_SET_WIFI_ERROR()				setWifiError()
extern void setWifiError(void);

#endif // _CONF_OS_H_
