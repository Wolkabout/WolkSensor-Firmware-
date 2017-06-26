'''
Created on August 10th 2016
@author: srdjan.stankovic@wolkabout.com
Last Modified on August 11th 2016
@author: srdjan.stankovic@wolkabout.com

brief: Set of robustness tests
'''
import os
import sys
import time

sys.path.append(os.path.dirname(os.path.realpath("Blesser.py")) + "/../../../WolkSensor-python-lib/core")
sys.path.append(os.path.dirname(os.path.realpath("Blesser.py")) + "/../../../WolkSensor-python-lib/API")

import logging_device

from serial_func import send_string_serial_wait
from serial_func import receive_string_serial
from device_test_func import wait_dev_state_idle
from device_test_func import unwanted_response
from device_test_func import protocol_parser
from device_test_func import parse_readings
from device_test_func import test
from device_wlan_set import set_wifi_parameters


def robustness():
    return_value = True
    
    def check_more_commands(command_argument_list, check_condition, visibility):
        response = True
        i = 0

        while i < len(command_argument_list):
            if "show" in visibility: logging_device.log("---> Send commands:" + command_argument_list[i])
            send_string_serial_wait(command_argument_list[i])
            if check_condition:
                received = receive_string_serial()
                if unwanted_response(received):
                    logging_device.debug("Unwanted response occurred. Received: %s" %received)
                    response = False
                if "show" in visibility: logging_device.log(" <-- Received response: %s" %received)
            elif not check_condition:
                received = receive_string_serial()
                if not unwanted_response(received):
                    logging_device.debug("Strange response occurred. Received: %s" %received)
                    response = False
                if "show" in visibility: logging_device.log(" <-- Received response: %s" %received)
            else:
                logging_device.error("Wrong input for check condition. Received: %s" %check_condition)
            i += 1

        return response


    logging_device.info("\n\r\t\t   Robustness Testing\n\r***************************************************************\n\r")
    wait_dev_state_idle()
    protocol_parser('MOVEMENT', True, 'OFF', True)

    logging_device.info("---------------- Send set of commands ----------------")
    logging_device.log("\t---True---")
    wait_dev_state_idle()
    if not check_more_commands(['SIGNATURE;STATIC_DNS;VERSION;PASS;MOVEMENT;SSID;STATIC_IP;READINGS;SYSTEM;URL;PORT;STATUS;'], True, "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    logging_device.log("\n\r\t---False---")
    wait_dev_state_idle()
    if not check_more_commands(['PORT;;', 'PORT1;', 'qPORT;', 'PORT;ST4TIC_DNS;V3RS1ON;ATMO;HEARTBEATMAC;7422;MOVEMENT;SSID123SSID;STATIC_IP;STATUS;', \
    'PORT;ST4TIC_DNS;V3RS1ON;ATMO;HEARTBEATMAC;7422;MOVEMENT;SSID123SSID;STATIC_IP;STATUS;', 'everything;is;WRONG;here.;THIS;;TEST;will;pass;STATUS;'], False, "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("---------------- Read/Write values during connection ----------------")
    logging_device.log("\t---True---")
    if not set_wifi_parameters('fakeargumentssid', 'NONE', 'fakeargumentpass'):
        return_value = False
    if not check_more_commands(['NOW;', 'SIGNATURE;STATIC_DNS;VERSION;PASS;MOVEMENT;SSID;STATIC_IP;MAC;HEARTBEAT;RTC;AUTH;PASS;TEMP_OFFSET;ID;STATUS;OFFSET_FACTORY'], True, "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.log("\n\r\t---False---")
    wait_dev_state_idle()
    if not check_more_commands(['NOW;', 'STATIC_DNS 89.89.89.90;PASS mypassword;OFFSET_FACTORY RESET;SSID myssid;STATIC_IP 192.168.24.23;HEARTBEAT 35;RTC 12356789;AUTH NONE;'], True, "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.log("\t---True---")
    set_wifi_parameters('wolkabout', 'WPA2', 'Walkm3int0')
    wait_dev_state_idle()
    if not check_more_commands(['NOW;'], True, "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    if not check_more_commands(['SIGNATURE;STATIC_DNS;VERSION;PASS;MOVEMENT;SSID;STATIC_IP;MAC;HEARTBEAT;RTC;AUTH;PASS;ID;STATUS;'], True, "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    '''
    logging_device.info("---------------- Read/Write during reload ----------------")
    logging_device.log("\n\r\t---False---")
    if not set_wifi_parameters('NULL', 'NONE', 'NULL'):
        return_value = False
    if not check_more_commands(['RELOAD;', 'SIGNATURE;STATIC_DNS;VERSION;PASS;OFFSET_FACTORY;MOVEMENT;SSID;STATIC_IP;MAC;HEARTBEAT;RTC;AUTH;PASS;ID;STATUS;'], False, "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    wait_dev_state_idle()
    if not check_more_commands(['RELOAD;','PORT;ST4TIC_DNS;V3RS1ON;ATMO;HEARTBEATMAC;7422;MOVEMENT;SSID123SSID;STATIC_IP;STATUS;'], False, "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    wait_dev_state_idle()
    if not check_more_commands(['RELOAD;', 'STATIC_DNS 89.89.89.90;PASS mypassword;PRESSURE_OFFSET 0;SSID myssid;STATIC_IP 192.168.24.23;HUMIDITY_OFFSET 0;HEARTBEAT 35;RTC 12356789;AUTH NONE;'], False, "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    '''
    logging_device.info("---------------- Read during movement ----------------")
    logging_device.log("\t---True---")
    wait_dev_state_idle()
    if not set_wifi_parameters('wolkabout', 'WPA2', 'Walkm3int0'):
        return_value = False
    protocol_parser('MOVEMENT', True, 'ON', True)

    logging_device.log("\t -->MOVE WolkSensor to continue testing<--")
    while "STATUS CONNECTING_TO_AP;" not in receive_string_serial(): pass
    if not check_more_commands(['SIGNATURE;STATIC_DNS;VERSION;PRESSURE_OFFSET;PASS;MOVEMENT;SSID;STATIC_IP;MAC;HEARTBEAT;RTC;AUTH;OFFSET_FACTORY;PASS;ID;STATUS;'], True, "show"):
        return_value = False
    if not check_more_commands(['P0RT;ST4TIC_DNS;V3RS1ON;ATM0;HEARTBEATMAC;7422;MOVEMENTBRE;SSID123SSID;ST4TUS;'], False, "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.log("\t---False---")
    wait_dev_state_idle()
    if not set_wifi_parameters('fakeargumentssid', 'WEP', 'FA7EA4603E27'):
        return_value = False
    protocol_parser('MOVEMENT', True, 'ON', True)

    logging_device.log("\t -->MOVE WolkSensor to continue testing<--")
    while "STATUS CONNECTING_TO_AP;" not in receive_string_serial(): pass
    if not check_more_commands(['SIGNATURE;STATIC_DNS;VERSION;PRESSURE_OFFSET;PASS;MOVEMENT;SSID;STATIC_IP;MAC;HEARTBEAT;RTC;AUTH;OFFSET_FACTORY;PASS;ID;STATUS;'], True, "show"):
        return_value = False
    if check_more_commands(['P0RT;ST4TIC_DNS;V3RS1ON;ATM0;HEARTBEATMAC;7422;MOVEMENTBRE;SSID123SSID;ST4TUS;'], False, "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    wait_dev_state_idle()
    protocol_parser('MOVEMENT', True, 'OFF', True)
    while not set_wifi_parameters('wolkabout', 'WPA2', 'Walkm3int0'): pass
    if not check_more_commands(['NOW;'], True, "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t---------------- Buffer size test with ACQUISITION; ----------------")
    logging_device.log("\t---True---")
    BUFFER_SIZE = 180
    wait_dev_state_idle()
    while not set_wifi_parameters('wolkabout', 'WPA2', 'Walkm3int0'): pass
    check_more_commands(['ATMO ON;'],True, "show")
    if not parse_readings('CLEAR', "show"):
        return_value = False
        logging_device.error("Return value is %s on READINGS CLEAR; command" %return_value)

    def check_buff(result):
        if int(result) == BUFFER_SIZE:
            logging_device.log("The Buffer is full. Number of readings is/are %s which is equal to buffer size.\n\r\tSUCCESSFULLY FILLED BUFFER" % result)
        elif int(result) < BUFFER_SIZE:
            logging_device.error("Number of readings is smaller than full MAX buffer size.")
        else:
            logging_device.error("Number of readings is bigger than full MAX buffer size.")

        if not parse_readings('CLEAR', "hide"):
            return_value = False
            logging_device.error("Return value is %s on READINGS CLEAR; command" %return_value)

    for counter in range(0, BUFFER_SIZE):
        if not check_more_commands(['ACQUISITION;'], True, "hide"):
            return_value = False
            logging_device.error("Return value is %s on ACQUISITION; action" %return_value)
            return
        sys.stdout.write("...sending ACQUISITION;\r")
        logging_device.debug("number of iteration is %d on ACQUISITION; action" %counter)

    wait_dev_state_idle()
    result = parse_readings('', "hide")
    if not result:
        return_value = False
        logging_device.error("Return value is %s on READINGS; command" %return_value)
    check_buff(result)

    logging_device.log("Made more[190] ACQUISITIONs than buffer can store[180]")
    for counter in range(0, BUFFER_SIZE+10):
        if not check_more_commands(['ACQUISITION;'], True, "hide"):
            return_value = False
            logging_device.error("Return value is %s on ACQUISITION; action" %return_value)
            return
        sys.stdout.write("...sending ACQUISITION;\r")
        logging_device.debug("number of iteration is %d on ACQUISITION; action" %counter)

    wait_dev_state_idle()
    result = parse_readings('', "hide")
    if not result:
        return_value = False
        logging_device.error("Return value is %s on READINGS; command" %return_value)
    check_buff(result)


    logging_device.info("\n\r\t\t---------------- Return WolkSensor to settings before the test was started ----------------")
    wait_dev_state_idle()
    while not set_wifi_parameters('wolkabout', 'WPA2', 'Walkm3int0'): pass
    while not protocol_parser('MOVEMENT', True, 'OFF', True): pass
    while not protocol_parser('ATMO', True, 'ON', True): pass

    logging_device.log("\n\r")
    return return_value
