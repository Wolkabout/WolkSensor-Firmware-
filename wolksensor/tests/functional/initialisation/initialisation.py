import ctypes
import constants
import logging_device

from colorama import init

def initialisation():
    ctypes.windll.kernel32.SetConsoleTitleA("Functional Testing")
    
    init()
    logging_device.set_level(logging_device._info_)
    logging_device.info("")

    print constants.LOGO + "\n\r\t\t   Functional Testing\n\r==========================================================="
