#ifndef USB_H_
#define USB_H_

enum USBSTATE { USB_OFF, USB_ON };
	
void usb_start(void);
void usb_stop(void);
bool uart_poll(void);

void usb_putc(char in);

#endif /* USB_H_ */