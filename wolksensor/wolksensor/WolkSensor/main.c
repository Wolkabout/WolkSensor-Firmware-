/*
 * This file is a part of WolkSensor firmware.
 * Copyright (C) 2016 WolkAbout Technology s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

// OS
#include "Sensors/sensor.h"
#include "Sensors/batteryADC.h"
#include "Sensors/LSM303.h"
#include "RTC.h"
#include "event_buffer.h"
#include "clock.h"
#include "brd.h"
#include "logger.h"
#include "config.h"
#include "sensors.h"
#include "UART.h"
#include "platform_specific.h"
#include "commands.h"
#include "clock.h"
#include "wolksensor.h"
#include "wolksensor_dependencies.h"
#include "wifi_communication_module.h"
#include "wifi_communication_module_dependencies.h"
#include "mqtt_communication_protocol.h"
#include "mqtt_communication_protocol_dependencies.h"
#include "wifi_cc3100.h"
#include "encryption.h"
#include "global_dependencies.h"
#include "nonvolatile_memory.h"
#include "simplelink.h"
#include "flc_api.h"

/* flash code length -- this is loaded in flash by the linker scripts */ 
const uint32_t ProgramLength __attribute__ ((section (".length"))) = 0x12345678;

#ifdef DEBUG
unsigned int opcodeTestMem1 = 0;
unsigned int opcodeTestMem2 = 0;
unsigned int opcodePreviousTestMem = 0;
unsigned int callNmbr = 0;
unsigned int callNmbr1 = 0;
unsigned int unsolEventNmbr = 0;
unsigned int event_handlerStateTestMem = 0;
unsigned char receivedSPITest[32];
unsigned char sentSPITest[32];
unsigned int numberOFSPIBytesReceivedTest = 0;
unsigned int numberOFSPIBytesSentTest = 0;
unsigned int patchState = 0;
unsigned long int nmbrOfPasses = 0;
unsigned char timeON = 0;
#endif

// functions from sensors
void init_sensors(void);

void resetCheck(void)
{
	if (RST.STATUS & RST_PORF_bm)
	{
		reset_reason = RST_POWER_ON;
	}
	else if(RST.STATUS & RST_PDIRF_bm)
	{
		reset_reason = RST_PROGRAMMING_DEBUG;
	}
	else if(RST.STATUS & RST_EXTRF_bm)
	{
		reset_reason = RST_EXTERNAL;
	}
	else if (RST.STATUS & RST_WDRF_bm)
	{
		reset_reason = RST_WATCHDOG;
	}
	else if (RST.STATUS & RST_BORF_bm)
	{
		reset_reason = RST_BROWN_OUT;
	}
	else if (RST.STATUS & RST_SRF_bm)
	{
		;// reset_reason = reset_reason;
	}
	RST.STATUS = 0xff;
	
}

static void init_global_dependencies(void)
{
	global_dependencies.rtc_get = rtc_get;
	global_dependencies.log = send_command_response;
	global_dependencies.send_response = send_command_response;
	global_dependencies.config_read = config_read;
	global_dependencies.config_write = config_write;
}
 
static void init_wolksensor_dependencies(void)
{
	wolksensor_dependencies.get_usb_state = get_usb_state;
	wolksensor_dependencies.get_sensors_states = get_sensors_states;
	wolksensor_dependencies.enable_battery_voltage_monitor = enable_voltage_monitor;
	wolksensor_dependencies.disable_battery_voltage_monitor = disable_voltage_monitor;
	wolksensor_dependencies.add_minute_expired_listener = add_minute_expired_listener;
	wolksensor_dependencies.add_usb_state_change_listener = add_usb_state_change_listener;
	wolksensor_dependencies.add_command_data_received_listener = add_command_data_received_listener;
	wolksensor_dependencies.add_battery_voltage_listener = add_battery_voltage_listener;
	wolksensor_dependencies.add_sensors_states_listener = add_sensors_states_listener;
	wolksensor_dependencies.system_reset = system_reset;
	wolksensor_dependencies.enable_movement = enable_movement;
	wolksensor_dependencies.disable_movement = disable_movement;
}

static void init_wifi_communication_module_dependencies(void)
{		
	wifi_communication_module_dependencies.wifi_start = wifi_start;
	wifi_communication_module_dependencies.wifi_connect = wifi_connect;
	wifi_communication_module_dependencies.wifi_disconnect = wifi_disconnect;
	wifi_communication_module_dependencies.wifi_stop = wifi_stop;
	wifi_communication_module_dependencies.wifi_reset = wifi_reset;

	wifi_communication_module_dependencies.wifi_open_socket = wifi_open_socket;
	wifi_communication_module_dependencies.wifi_close_socket = wifi_close_socket;
	wifi_communication_module_dependencies.wifi_receive = wifi_receive;
	wifi_communication_module_dependencies.wifi_send = wifi_send;
	
	wifi_communication_module_dependencies.get_ip_address = wifi_get_current_ip;
	
	wifi_communication_module_dependencies.serialize_wifi_platform_specific_error_code = serialize_wifi_platform_specific_error_code;
	
	wifi_communication_module_dependencies.add_milisecond_expired_listener = add_milisecond_expired_listener;
	wifi_communication_module_dependencies.add_second_expired_listener = add_second_expired_listener;
	
	wifi_communication_module_dependencies.add_wifi_connected_listener = add_wifi_connected_listener;
	wifi_communication_module_dependencies.add_wifi_ip_address_acquired_listener = add_wifi_ip_address_acquired_listener;
	wifi_communication_module_dependencies.add_wifi_disconnected_listener = add_wifi_disconnected_listener;
	wifi_communication_module_dependencies.add_wifi_error_listener = add_wifi_error_listener;
	wifi_communication_module_dependencies.add_wifi_socket_closed_listener = add_wifi_socket_closed_listener;
	
	wifi_communication_module_dependencies.add_wifi_platform_specific_error_code_listener = add_wifi_platform_specific_error_code_listener;
}

static void init_mqtt_communication_protocol_dependencies(void)
{
	mqtt_communication_protocol_dependencies.encrypt = encrypt;
	mqtt_communication_protocol_dependencies.decrypt = decrypt;
}

static void wire_wifi_communication_module(void)
{
	communication_module.sendd = wifi_communication_module_send;
	communication_module.receive = wifi_communication_module_receive;
	communication_module.stop = wifi_communication_module_stop;
	communication_module.get_communication_result = get_wifi_communication_result;
	communication_module.get_status = get_wifi_communication_module_status;
}

static void wire_mqtt_communication_protocol(void)
{
	communication_protocol.send_sensor_readings_and_system_data = mqtt_protocol_send_sensor_readings_and_system_data;
	communication_protocol.receive_commands = mqtt_protocol_receive_commands;
	communication_protocol.disconnect = mqtt_protocol_disconnect;
	communication_protocol.get_communication_result = get_mqtt_communication_result;
}

static bool process(void)
{
	bool poll = poll_usb_state() || waiting_movement_sensor_enable_timeout();
	bool application_processing = wolksensor_process();
	
	CC3100_process();
	
	return poll || application_processing;
}

static void set_sensors_types(void)
{
	sensors[0].id = 'P';
	sensors[0].type = VALUE_ON_DEMAND;
	sensors[0].alarm_type = ALARM_TYPE_NOTIFY_ONCE;
	
	sensors[1].id = 'T';
	sensors[1].alarm_type = VALUE_ON_DEMAND;
	sensors[1].alarm_type = ALARM_TYPE_NOTIFY_ONCE;
	
	sensors[2].id = 'H';
	sensors[2].type = VALUE_ON_DEMAND;
	sensors[2].alarm_type = ALARM_TYPE_NOTIFY_ONCE;
	
	sensors[3].id = 'M';
	sensors[3].type = NOTIFIES_VALUE;
	sensors[3].alarm_type = ALARM_TYPE_NOTIFY_ALWAYS;
}

int main(void)
{
	resetCheck();
	
	start_type_t start_type;
	switch(reset_reason)
	{
		case RST_WATCHDOG:
		{
			start_type = WATCHDOG_RESET;
			break;
		}
		case RST_BROWN_OUT:
		{
			start_type = BROWNOUT_RESET;
			break;
		}
		default:
		{
			start_type = POWER_ON;
		}
	}
	
	init_global_dependencies();
	init_wolksensor_dependencies();
	init_wifi_communication_module_dependencies();
	init_mqtt_communication_protocol_dependencies();
	
	wire_wifi_communication_module();
	wire_mqtt_communication_protocol();
	
	brd_init();
	clock_init();

	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
	sei(); // allows interrupts, cli() disables them

	RTC_init(start_type == POWER_ON);
	
	uart_init();
	
	LOG_PRINT(1, PSTR("Reset reason %d\r\n"), reset_reason);
	
	set_sensors_types();
	init_sensors();

	init_commands();

	init_wifi_communication_module();
	mqtt_protocol_init();
	
	if(start_type != BROWNOUT_RESET)
	{
		init_wifi();
	}

	LOG(1, "Watchdog enable");
	watchdog_enable(WDT_WPER_8KCLK_gc);
	
	init_wolksensor(start_type);
	
	system_error_t reset_system_error;
	reset_system_error.type = SYSTEM_RESET;
	reset_system_error.data.system_reset_reason = reset_reason;
	add_system_error(&reset_system_error);
	
	for (;;) 
	{
		bool keep_runing = process();
		while(keep_runing)
		{
			watchdog_reset();
			keep_runing = process();
		}
		
		watchdog_reset();
		set_sleep_mode(SLEEP_MODE_PWR_SAVE);
		cli();
		
		{
			watchdog_disable();
			
			sleep_enable();
			sei();
			sleep_cpu();
			sleep_disable();
			watchdog_enable(WDT_WPER_8KCLK_gc);	
		}
		
		sei();
		
		LOG(1, "Awake!");
	}
	
	return 0;
}

