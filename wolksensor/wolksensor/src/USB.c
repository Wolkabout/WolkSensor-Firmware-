#include <asf.h>

#include <string.h>
#include <stdio.h>

// OS
#include "clock.h"

#include "parser.h"
#include "system.h"
#include "USB.h"

#include "config/conf_os.h"


static volatile bool	my_flag_autorize_cdc_transfert = false; // indicates that the driver has been initialized (usb up and running)
static volatile bool	exec_mode = false; // characters are stored under interrupt to buffer, when we reach ';' this flag is set to true and buffer is processed
static char	usb_buffin[256]; // input buffer
static char	usb_buffout[256]; // output buffer
static volatile int put_ptr = 0; // pointer for input buffer

// starts usb driver if it is not running already
void usb_start(void) {
	if (!my_flag_autorize_cdc_transfert) {
		clock_set(CLK_24MHZ); // usb requires this speed
		udc_start();
	}
}


void usb_stop(void) {
	udc_stop();
	clock_set(CLK_32KHZ);
}


bool uart_poll(void) {
	if (my_flag_autorize_cdc_transfert) {
		if (exec_mode) {
			exec_mode = false;
			// process content of the buffer
			parse_line(usb_buffin, usb_buffout, sizeof(usb_buffout) - 1, true);
				
			char *ptr = usb_buffout;
			// print out the command response
			while (*ptr) {
				udi_cdc_putc(*ptr++);
			}
			// reset pointer and exec mode
			put_ptr = 0;
			return true;
		}
	}
	return false;
}


// sends a single character if usb driver is running
void usb_putc(char in) {
	if (my_flag_autorize_cdc_transfert)
		udi_cdc_putc(in);
}


bool my_callback_cdc_enable(void) {
	my_flag_autorize_cdc_transfert = true;
	// usb is up and running, we can raise the charging current
	PORTD.OUTSET = 0b00000001;			// 500 mA charge current
	OS_USB_START();
	return true;
}

// usb is turned off
void my_callback_cdc_disable(void) {
	my_flag_autorize_cdc_transfert = false;
}

// receive characters from usb
void my_callback_rx_notify(uint8_t port) {
	uint16_t size = udi_cdc_get_nb_received_data();
	while (size--) {
		char in = udi_cdc_getc();
		// always parse all received characters, discard them if device is in exec mode (';' received)
		if (!exec_mode) {
			if (in ==';') {
				usb_buffin[put_ptr++] = in;
				usb_buffin[put_ptr] = '\0';
				exec_mode = true;
			} else if (put_ptr < (sizeof(usb_buffin) - 2)) {
				usb_buffin[put_ptr++] = in;
				usb_buffin[put_ptr] = '\0';
			}
		}
	}
}


