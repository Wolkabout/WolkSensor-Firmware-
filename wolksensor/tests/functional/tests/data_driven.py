'''
Created on August 1st 2016
@author: srdjan.stankovic@wolkabout.com
Last Modified on August 08st 2016
@author: srdjan.stankovic@wolkabout.com

brief: Set of Data Driven tests
'''
import os
import sys

sys.path.append(os.path.dirname(os.path.realpath("Blesser.py")) + "/../../../WolkSensor-python-lib/core")
sys.path.append(os.path.dirname(os.path.realpath("Blesser.py")) + "/../../../WolkSensor-python-lib/API")

import logging_device
import constants

from device_test_func import check_argument
from device_test_func import parse_readings
from device_test_func import parse_system_reading
from device_test_func import protocol_parser
from device_test_func import test
from device_test_func import wait_dev_state_idle
from device_wlan_set import set_wifi_parameters


def data_driven():
    return_value = True
    logging_device.info("\n\r\t\t   Data Driven Tests\n\r***************************************************************\n\r")

    logging_device.info("------------ turn OFF all functionalities -----------")

    while not test('MOVEMENT', ['OFF'],[]): pass
    while not test('ATMO', ['OFF'],[]): pass
    while not test('SSID', ['NULL'],[]): pass
    while not test('PASS', ['NULL'],[]): pass
    while not test('AUTH', ['NONE'],[]): pass

    logging_device.info("---------------- Read Only parameters ----------------")

    logging_device.info("\n\r\t\t-------- STATUS --------")
    if not protocol_parser('STATUS', False,'', True) or not protocol_parser('STATUS', True, 'change status', False):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- VERSION --------")
    if not protocol_parser('VERSION', False,'', True) or not protocol_parser('VERSION', True, 'which version', False):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- ID --------")
    if not protocol_parser('ID', False,'', True) or not protocol_parser('ID', True, 'identifier', False):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- SIGNATURE --------")
    if not protocol_parser('SIGNATURE', False,'', True) or not protocol_parser('SIGNATURE', True, 'some signature', False):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- MAC --------")
    if not protocol_parser('MAC', False,'', True) or not protocol_parser('SIGNATURE', True, '11223344aaBBccDD', False):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("------------------- R\W parameters -------------------")

    logging_device.info("\n\r\t\t-------- URL --------")
    if not test('URL', ['8.8.8.8', '0.0.0.0', '255.255.255.255', 'appwolksense.com', 'app-wolksense.com', '9gag.com', 'automatika.ftn.uns.ac.rs', 'ftn.uns.ac.rs', constants.URL, constants.HOSTNAME], ['', '-1.-1.-1.-1', '256.256.256.256',\
    'appwolksense', '-app.wolksense', 'app.wolksense-', 'app.wolksense*com', '9821.com.123', '9gag.123.com', 'abc.123.abc.abc', 'app.wolksense.comapp.wolksense.comapp.wolksense.comapp.wolksense.comapp.wolksense.com' , '\Test1234', '!")(*&^%;']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- SSID --------")
    if not test('SSID', ['mywirelessnetwork', '1234567890', 'm4w1r3l3ssn3tw0rk', 'qwertyuiopqwertyuiopqwertyuiopqw', '' , '!")(*&^%', '\n\r', 'WA_1'], ['qwertyuiopqwertyuiopqwertyuiopqwe']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- AUTH --------")
    if not test('AUTH', ['NONE', 'WEP', 'WPA', 'WPA2'], ['', 'WPA Ent', '0123456789']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- PASS --------")
    if not test('PASS', ['mywirelessnetworkpassword', '1234567890', 'm4w1r3l3ssn3tw0rkpassw0rd', 'qwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwe', '' , '!")(*&^%', '\n\r', \
    'wolksensorsystem'], ['qwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwer', '0123456789012345678901234567890123456789012345678901234567890123456789']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    if not test('AUTH', ['WEP'], []):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    if not test('PASS', ['008C7073C348F91054E1FB8729', '00000000000000000000000000', 'FFFFFFFFFFFFFFFFFFFFFFFFFF', '3164E175384E17240A78CE63D3' ], \
    ['qwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwer', '0123456789012345678901234567890123456789012345678901234567890123456789', '!")(*&^%', '\n\r', '']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)
    if not test('AUTH', ['WPA2'], []):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- RTC --------")
    if not test('RTC', ['0', '1470236756', '4294967295', '1388588755'], ['', '-1', '12345678901234567890123456789', 'timecountdown', 't1m3c0u5td0wn', '!")(*&^%']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- PORT --------")
    if not test('PORT', ['1883', '0', '65535', '8883'], ['', '-5', '65536', 'mynetworkport', 'm4n3tw0rkp0rt', '!")(*&^%', '0123456789012345678901234567890123456789012345678901234567890123456789', \
    'qwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwer']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- HEARTBEAT --------")
    if not test('HEARTBEAT', ['5', '10', '30', '0', '60'], ['', '-1', '61', '65535', '1234567890', 'heartbeatcount', '!")(*&^%', '0123456789012345678901234567890123456789012345678901234567890123456789', \
    'qwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwer']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- MOVEMENT --------")
    if not test('MOVEMENT', ['ON', 'OFF'], ['', 'yes', 'NO', '0123', 'movement', 'm0v3m3nt', '0123456789012345678901234567890123456789012345678901234567890123456789', \
    'qwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwer']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- ATMO --------")
    if not test('ATMO', ['ON', 'OFF'], ['', 'yes', 'NO', '0123', 'atmo', '4tm0', '0123456789012345678901234567890123456789012345678901234567890123456789', \
    'qwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwertyuiopqwer']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- STATIC MASK --------")
    if not test('STATIC_MASK', ['192.168.15.98', '0.0.0.0', '255.255.255.255', '255.255.255.0'], ['ON', '', '-1.-1.-1.-1', '256.256.256.256', 'app.wolkabout.com', '\Test1234', '!")(*&^%;']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- STATIC GATEWAY --------")
    if not test('STATIC_GATEWAY', ['192.168.15.98', '0.0.0.0', '255.255.255.255', '192.168.15.1'], ['ON', '', '-1.-1.-1.-1', '256.256.256.256', 'app.wolkabout.com', '\Test1234', '!")(*&^%;']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- STATIC DNS --------")
    if not test('STATIC_DNS', ['192.168.15.98', '0.0.0.0', '255.255.255.255', '8.8.4.4'], ['ON', '', '-1.-1.-1.-1', '256.256.256.256', 'app.wolkabout.com', '\Test1234', '!")(*&^%;']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- STATIC IP --------")
    if not test('STATIC_IP', ['192.168.15.98', '0.0.0.0', '255.255.255.255', 'OFF'], ['ON', '', '-1.-1.-1.-1', '256.256.256.256', 'app.wolkabout.com', '\Test1234', '!")(*&^%;']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- READINGS --------")
    if not test_readings_response():
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- SYSTEM --------")
    if not test_system_response():
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- TEMP_OFFSET --------")
    if not test('TEMP_OFFSET', ['0', '1', '-1', '37', '-20'], ['38', '-21', '', '-1.temperature.+1.QWE', 'app.wolkabout.com', '\Test1234', '!")(*&^%;']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- HUMIDITY_OFFSET --------")
    if not test('HUMIDITY_OFFSET', ['0', '1', '-1', '30', '-30'], ['31', '-31', '', '-1.humidity.+1.QWE', 'app.wolkabout.com', '\Test1234', '!")(*&^%;']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- PRESSURE_OFFSET --------")
    if not test('PRESSURE_OFFSET', ['0', '1', '-1', '100', '-100'], ['101', '-101', '', '-1.pressure.+1.QWE', 'app.wolkabout.com', '\Test1234', '!")(*&^%;']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- OFFSET_FACTORY --------")
    if not test('OFFSET_FACTORY', ['RESET'], ['RESETTOFACTORY', 'RESE', '', '-1.pressure.+1.QWE', 'app.wolkabout.com', '\Test1234', '!")(*&^%;']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    logging_device.info("\n\r\t\t-------- ACQUISITION --------")
    if not test('ACQUISITION', [], ['empty', 'CLEAR', '', '-1.pressure.+1.QWE', '!")(*&^%;']):
        return_value = False
        logging_device.error("Return value is %s" %return_value)

    if not set_wifi_parameters('WA_1', 'WPA2', 'wolksensorsystem'):
        return_value = False
        logging_device.error("Return value is %s" %return_value)


    logging_device.log("\n\r")
    return return_value


def test_readings_response():
    response = True

    i = 0
    list = ['', 'CLEAR']
    while i < len(list):
        if not parse_readings(list[i], "show"):
            response = False
        i += 1

    i = 0
    list = ['-1.-1.-1.-1', '256.256.256.256', 'app.wolkabout.com', '\Test1234', '!")(*&^%;']
    while i < len(list):
        if parse_readings(list[i], "show"):
            response = False
        logging_device.log("False conditions: " + list[i])
        i += 1

    return response

def test_system_response():
    response = True

    i = 0
    list = ['', 'CLEAR']
    while i < len(list):
        if not parse_system_reading(list[i], 'show'):
            response = False
        i += 1

    i = 0
    list = ['-1.-1.-1.-1', '256.256.256.256', 'app.wolkabout.com', '\Test1234', '!")(*&^%;']
    while i < len(list):
        if parse_system_reading(list[i], 'show'):
            response = False
        logging_device.log("False conditions: " + list[i])
        i += 1

    return response
