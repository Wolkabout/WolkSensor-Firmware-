'''
Created on July 29th 2016
@author: srdjan.stankovic@wolkabout.com
Last Modified on July 29th 2016
@author: srdjan.stankovic@wolkabout.com

brief: Set of functional test for WolkSensor device.
       WolkSensor device are connected to USB Serial port via COMx.
       Functional testing is implemented true three stages:
            * Data Driven tests
            * Flow test
            * Robustness test
'''
#!/Python27/python
import os
import sys
sys.path.append(os.path.dirname(os.path.realpath("functional.py")) + "/../../../WolkSensor-python-lib/core")
sys.path.append(os.path.dirname(os.path.realpath("functional.py")) + "/initialisation")
sys.path.append(os.path.dirname(os.path.realpath("functional.py")) + "/tests")

import logging_device
import constants

from serial_func import open_serial
from serial_func import close_serial
from initialisation import initialisation
from data_driven import data_driven
from flow_states import flow_states
from robustness import robustness


def main():
    initialisation()

    while 1:
        serial = open_serial()
        if serial == True:

            if data_driven():
                logging_device.info(constants.PASSED)
            else:
                logging_device.error(constants.FAIL)

            if flow_states():
                logging_device.info(constants.PASSED)
            else:
                logging_device.error(constants.FAIL)

            if robustness():
                logging_device.info(constants.PASSED)
            else:
                logging_device.error(constants.FAIL)

            close_serial()
            return True

        elif serial == constants.QUIT:
            print "...quit to main menu"
            return constants.QUIT

        else:
            logging_device.error("Selected serial port is open in another program\n\r")

    return True

if __name__ == '__main__':
    sys.exit(main())
