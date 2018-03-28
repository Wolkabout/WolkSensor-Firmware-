'''
Created on August 08th 2016
@author: srdjan.stankovic@wolkabout.com
Last Modified on October 31th 2016
@author: srdjan.stankovic@wolkabout.com

brief: Set of flow states tests
'''
import os
import sys
import time
import re

sys.path.append(os.path.dirname(os.path.realpath("Blesser.py")) + "/../../../WolkSensor-python-lib/core")
sys.path.append(os.path.dirname(os.path.realpath("Blesser.py")) + "/../../../WolkSensor-python-lib/API")

import logging_device
import constants

from device_test_func import protocol_parser
from device_test_func import test
from device_test_func import test_now
from device_test_func import parse_readings
from device_test_func import receive_string_serial
from device_test_func import send_string_serial_wait
from device_test_func import wait_dev_state_idle
from device_test_func import parse_system_reading
from device_test_func import test_readings_response
from device_wlan_set import set_wifi_parameters

def flow_states():
    return_value = True
    
    logging_device.info("\n\r\t\t   Flow States Testing\n\r***************************************************************\n\r")
    while not set_wifi_parameters(constants.SSID, constants.AUTH, constants.PASS): pass
    while not protocol_parser('MOVEMENT', True, 'OFF', True): pass
    while not protocol_parser('ATMO', True, 'ON', True): pass
    while not protocol_parser('STATIC_IP', True, 'OFF', True): pass

    logging_device.info("\t\t---------------- Set WiFi Flow ----------------")
    logging_device.log("\t---True flow---")
    logging_device.info("\n\r\t\t-------- set Wifi with WPA2 secure --------")
    response = test_now('', '')
    if response != True:
        return_value = False
        logging_device.error("Return value is %s. Response from command NOW; is: %s" %(return_value, response))

    '''
    logging_device.info("\n\r\t\t-------- set Wifi with WPA secure --------")
    if not set_wifi_parameters('WolkSensorWlan2', 'WPA', 'WolkSensorFabrication'):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    response = test_now('', '')
    if response != True:
        return_value = False
        logging_device.error("Return value is %s. Response from command NOW; is: %s" %(return_value, response))

    logging_device.info("\n\r\t\t-------- set Wifi with WEP secure --------")
    if not set_wifi_parameters('WolkSensorWlan', 'WEP', '5FD6D8ED2087FEF502CDFE873F'):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    response = test_now('', '')
    if response != True:
        return_value = False
        logging_device.error("Return value is %s. Response from command NOW; is: %s" %(return_value, response))

    logging_device.info("\n\r\t\t-------- set Wifi with OPEN secure --------")
    if not set_wifi_parameters('WolkSensorWlan1', 'NONE', 'NULL'):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    response = test_now('', '')
    if response != True:
        return_value = False
        logging_device.error("Return value is %s. Response from command NOW; is: %s" %(return_value, response))
    '''

    logging_device.log("\t---False flow---")
    if not set_wifi_parameters('my0penwl4n', 'NONE', 'NULL'):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    if test_now('', '') == True:
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- read Readings --------")
    logging_device.log("\t---True flow---")
    wait_dev_state_idle()
    while not set_wifi_parameters('NULL', 'NONE', 'NULL'): pass
    test('MOVEMENT', ['OFF'],[])
    test('ATMO', ['ON'],[])
    if not parse_readings('CLEAR', "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.log("set ACQUISITION state")
    if not test('ACQUISITION', [], []):
        return_value = False
        logging_device.error("Return value is %s in send ACQUISITION" %return_value)

    if not parse_readings('CLEAR', "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    logging_device.log("RELOAD WolkSensor \n\r..waiting for STATUS ACQUISITION;")
    send_string_serial_wait('RELOAD;')
    wait_dev_state_idle()
    if not parse_readings('', "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.log("CLEAR and OFF atmo measurements")
    test('ATMO', ['OFF'],[])
    if not parse_readings('CLEAR', "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    for i in reversed(range(0, 65)):
        time.sleep(1)
        print " | %s | ...wait 65sec to countdown ACQUISITION period \r" %i,
    if 'empty' != parse_readings('', "show"):
        return_value = False
        logging_device.error("READINGS message isn't empty. Expected to be. ATMO is OFF. Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- Check OFFSET settings --------")
    def set_offset(pressure_offset, temperature_offset, humidity_offset):
        logging_device.debug( "\n\rP offset: %s\n\rT offset: %s\n\rH offset: %s" %(str(pressure_offset), str(temperature_offset), str(humidity_offset)) )
        if not test('PRESSURE_OFFSET', [str(pressure_offset)],[]):
            return False
        if not test('TEMP_OFFSET', [str(temperature_offset)],[]):
            return False
        if not test('HUMIDITY_OFFSET', [str(humidity_offset)],[]):
            return False

        return True

    logging_device.log("\t---True flow---")
    if not test('ATMO', ['ON'],[]):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    if not set_offset(0, 0, 0):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    if not parse_readings('CLEAR', "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.log("set ACQUISITION state")
    if not test('ACQUISITION', [], []):
        return_value = False
        logging_device.error("Return value is %s in send ACQUISITION" %return_value)
    send_string_serial_wait("READINGS;")
    received_string = receive_string_serial()
    logging_device.debug("Received message readings: %s" %received_string)
    if len(received_string) > 9 :
        string_split = re.split(" ", received_string)
    logging_device.debug("string_split is/are %s" %string_split)
    pressure = test_readings_response(string_split[1], "show")[1]
    temp = test_readings_response(string_split[1], "show")[2]    
    humidity = test_readings_response(string_split[1], "show")[3]
    logging_device.log("%s, %s, %s" %(pressure, temp, humidity))

    logging_device.log("\tSET NEW OFFSET VALUES")
    if not set_offset(1, 2, 3):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    if not parse_readings('CLEAR', "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.log("set ACQUISITION state")
    if not test('ACQUISITION', [], []):
        return_value = False
        logging_device.error("Return value is %s in send ACQUISITION" %return_value)
    send_string_serial_wait("READINGS;")
    received_string = receive_string_serial()
    logging_device.debug("Received message readings: %s" %received_string)
    if len(received_string) > 9 :
        string_split = re.split(" ", received_string)
    logging_device.debug("string_split is/are %s" %string_split)
    pressure_new = test_readings_response(string_split[1], "show")[1]
    temp_new = test_readings_response(string_split[1], "show")[2]    
    humidity_new = test_readings_response(string_split[1], "show")[3]
    logging_device.log("Sensors values with new offsets are: %s, %s, %s" %(pressure_new, temp_new, humidity_new))

    if not ((int(float(temp)*100) in range(int(float(temp_new)*100) - 25*10, int(float(temp_new)*100) - 15*10)) or (int(float(humidity)*100) in range(int(float(humidity_new)*100) - 35*10, int(float(humidity_new)*100) - 25*10)) or (int(float(pressure)*100) in range(int(float(pressure_new)*100) - 15*10, int(float(pressure_new)*100) - 5*10))):
        logging_device.error("Return value is %s \n\rSet new offset values failed" %return_value)
        return_value = False
    else:
        logging_device.log("\tSUCCESSFUL ARE SETS NEW OFFSET VALUES")

    logging_device.info("\n\r\t\t-------- read Movement --------")
    logging_device.log("\t---True flow---")
    wait_dev_state_idle()
    test('MOVEMENT', ['ON'],[])
    test('ATMO', ['OFF'],[])
    if not set_wifi_parameters('NULL', 'NONE', 'NULL'):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    if not parse_readings('CLEAR', "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.log("\t -->MOVE WolkSensor to continue testing<--")
    send_string_serial_wait("READINGS;")
    received_string = receive_string_serial()
    while ",M:1.00;" not in received_string:
        send_string_serial_wait("READINGS;")
        received_string = receive_string_serial()
    time.sleep(0.5)
    if not parse_readings('', "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    wait_dev_state_idle()
    test('MOVEMENT', ['OFF'],[])
    if not parse_readings('CLEAR', "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    logging_device.log("\t -->MOVE WolkSensor to continue testing<--")
    for i in reversed(range(0, 5)):
        time.sleep(1)
        print " | %s |...move WolkSensor in this time period \r" %i,
    if not parse_readings('', "show"):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    test('ATMO', ['ON'],[])

    logging_device.info("\n\r\t\t-------- read System readings--------")
    logging_device.log("\t---True flow---")
    wait_dev_state_idle()
    if not set_wifi_parameters(constants.SSID, constants.AUTH, constants.PASS):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    if not parse_system_reading('CLEAR', 'show'):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    logging_device.log("---NOW---")
    wait_dev_state_idle()
    while test_now('', '') != True:
        pass
    if not parse_system_reading('', 'show'):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.log("\t---False flow---")
    wait_dev_state_idle()
    if not set_wifi_parameters('doesnotexist', 'WPA', 'doesnotexist'):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    if not parse_system_reading('CLEAR', 'show'):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    logging_device.log("---NOW---")
    wait_dev_state_idle()
    if test_now('', '') == True:
        return_value = False
        logging_device.error("Return value is %s. Received false from test_now('', '')" %return_value)

    wait_dev_state_idle()
    if not parse_system_reading('', 'show'):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    '''
    logging_device.info("\n\r\t\t-------- set Static IP--------")
    while not protocol_parser('ATMO', True, 'OFF', True): pass

    logging_device.log("\t---True flow---")
    wait_dev_state_idle()
    if not set_wifi_parameters('wolkabout', 'WPA2', 'Walkm3int0'):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    if not protocol_parser('STATIC_IP', False, '', True):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    i = 0
    list = ['STATIC_IP', '192.168.15.98', 'STATIC_GATEWAY', '192.168.15.1' , 'STATIC_MASK', '255.255.255.0', 'STATIC_DNS', '89.216.1.30']
    while( i < len(list)):
        if not protocol_parser(list[i], True, list[i+1], True):
            return_value = False
            logging_device.error("Return value is %s" %return_value)
        i += 2
    logging_device.log("---NOW---")
    wait_dev_state_idle()
    response = test_now('', '')
    if response != True:
        return_value = False
        logging_device.error("Return value is %s. Response from command NOW; is: %s" %(return_value, response))

    if not protocol_parser('STATIC_IP', True, 'OFF', True):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    i = 0
    list = ['STATIC_IP' , 'STATIC_GATEWAY', 'STATIC_MASK', 'STATIC_DNS']
    while( i < len(list)):
        if not protocol_parser(list[i], False, '', True):
            return_value = False
            logging_device.error("Return value is %s" %return_value)
        i += 1
    logging_device.log("---NOW---")
    wait_dev_state_idle()
    response = test_now('', '')
    if response != True:
        return_value = False
        logging_device.error("Return value is %s. Response from command NOW; is: %s" %(return_value, response))

    logging_device.log("\t---False flow---")
    wait_dev_state_idle()
    if not protocol_parser('STATIC_IP', False, '', True):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    i = 0
    list = ['STATIC_IP', '0.0.0.0', 'STATIC_GATEWAY', '0.0.0.0' , 'STATIC_MASK', '0.0.0.0', 'STATIC_DNS', '0.0.0.0']
    while( i < len(list)):
        if not protocol_parser(list[i], True, list[i+1], True):
            return_value = False
            logging_device.error("Return value is %s" %return_value)
        i += 2
    logging_device.log("---NOW---")
    wait_dev_state_idle()
    response = test_now('', '')
    if response == True:
        return_value = False
        logging_device.error("Return value is %s. Response from command NOW; is: %s" %(return_value, response))

    if not protocol_parser('STATIC_IP', True, 'OFF', True):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    while not protocol_parser('ATMO', True, 'ON', True): pass
    '''
    logging_device.info("\n\r\t\t-------- Factory OFFSET settings --------")
    while not set_wifi_parameters('NULL', 'NONE', 'NULL'): pass
    wait_dev_state_idle()

    if not set_offset(-10, -2, 1.5):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    send_string_serial_wait('OFFSET_FACTORY' + ';')
    received_string = receive_string_serial()
    logging_device.debug(received_string)
    string_split = re.split(" |;", received_string)
    factory = string_split[1]
    logging_device.debug("Factory: %s" %factory)
    
    if not test('OFFSET_FACTORY', ['RESET'],[]):
        return_value = False
        logging_device.error("Return value is %s in OFFSET_FACTORY reset" %return_value)

    send_string_serial_wait('TEMP_OFFSET' + ';')
    received_string = receive_string_serial()
    logging_device.debug("TEMP_OFFSET: %s" %received_string)
    string_split = re.split(" |;", received_string)
    temp = string_split[1]
    logging_device.debug("temp: %s" %temp)

    send_string_serial_wait('HUMIDITY_OFFSET' + ';')
    received_string = receive_string_serial()
    logging_device.debug("HUMIDITY_OFFSET: %s" %received_string)
    string_split = re.split(" |;", received_string)
    humidity = string_split[1]
    logging_device.debug("humidity: %s" %humidity)

    send_string_serial_wait('PRESSURE_OFFSET' + ';')
    received_string = receive_string_serial()
    logging_device.debug("PRESSURE_OFFSET: %s" %received_string)
    string_split = re.split(" |;", received_string)
    pressure = string_split[1]
    logging_device.debug("pressure: %s" %pressure)

    offset = "P:%s,T:%s,H:%s" %(pressure, temp, humidity)
    logging_device.log(factory)
    logging_device.log(offset)
    if factory != offset:
        return_value = False
        logging_device.error("Return value is %s in compare offset from factory. Factory offset are: %s, New offset are: %s" %(return_value, factory, offset))
    else:
        logging_device.log("\tSUCCESSFUL RESET TO FACTORY OFFSET")

    logging_device.log("\n\r")
    return return_value
