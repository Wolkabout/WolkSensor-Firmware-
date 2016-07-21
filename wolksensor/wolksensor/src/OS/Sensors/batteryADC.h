#ifndef BATTERY_ADC_H_
#define BATTERY_ADC_H_

bool batteryADC_init(void);
void batteryADC_poll(void);
void batteryADC_sub(uint32_t capacity);
bool batteryADC_battery(uint16_t *value);
bool batteryADC_powerOK(void);
bool USB_state(void);
void batteryMinVoltage_start(void);
void batteryMinVoltage_stop(void);
void batteryMinVoltage_poll(void);
uint16_t batteryMinVoltage_read(void);

void enable_voltage_monitor(void);
void disable_voltage_monitor(void);

void add_battery_voltage_listener(void (*listener)(uint16_t voltage));
void add_usb_state_change_listener(void (*listener_t)(bool usb_state));

bool get_usb_state(void);

bool poll_usb_state(void);

typedef enum { USB_OFF, USB_ON } power_t;
typedef enum { CHARGE_OFF, CHARGE_ON } charge_t;
	
#define USB_CHANGE_STATE_TIME 1000

#define VOLTAGE_DROPOUT_SERIES_DIODE 80.6	// = 0.806 * 100; 0.806V is voltage dropout on D1
#define RESISTOR_DIVIDER			 570	// Uadc = Umeas * (1 + 470k/100k) -> Uadc = Umeas * 5.7; sending 100x higher to server, 570 = 5.7*100
#define ADC_MAX_VALUE				 2047	// = 2^11; 11-bit ADC Result register
#define VOLTAGE_MEASUREMENT_OFFSET	 5		// Tolerance of used resistors

#endif /* BATTERY_ADC_H_ */
