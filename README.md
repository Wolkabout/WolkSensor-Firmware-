**WolkSensor Firmware Specification**
=====================================

**Introduction**
------------

WolkSensor is a device with built in temperature, pressure, humidity and movement sensor.
Purpose of this device is to:

 - measure temperature, pressure and humidity every minute
 - monitor movement
 - deliver recorded data to specified destination. Currently, WolkSensor can deliver data to WolkSense sensor data cloud.

Device can be powered through micro USB or three AA size batteries.
Device has two hardware communication interfaces onboard:
 - WiFi interface dedicated to delivering data to WolkSense sensor data cloud and getting settings from WolkSense sensor data cloud.
 - Micro USB port interface dedicated for configuring device and device maintenance (firmware update, retrieving event logs etc.).
Complete human machine interaction is done by use of web, mobile or desktop GUI.
WiFi configuration is done by desktop GUI (WolkSensor Assistant). WolkSensor Assistant application establishes serial over USB communication with device. WolkSensor Assistant application and device are exchanging data and actions through custom communication protocol.

Detailed information about WolkSensor can be found [here](https://wolksense.com/wolksensor/).

**WolkSensor Operation Data Cloud mode**
--------------------


All atmo sensors (temperature, humidity and pressure) data acquisition is done every minute, movement is constantly monitored if it is enabled.
Data delivery is done when any of the following cases:

 - on user defined interval (heartbeat)
 - when movement occurs if that is enabled.
 - if alarm conditions are met for any of the sensors (value read from sensor is less that minimum specified value, or higher than maximum specified value)

To keep the energy consumption to minimum WolkSensor is not constantly online. Those conditions are part of WolkSensor power management.

WolkSensor acquisition sequence:

 - Sensor data is collected (temperature, humidity, air pressure) every minute
 - Movement monitoring - constantly when enabled, else not monitored

WolkSensor connection sequence:

 - Reset heartbeat timer.
 - Turn on WiFi module.
 - Connect to WiFi access point and establish internet connection.
 - Connect to WolkSense sensor data cloud using MQTT communication protocol and SSL secure.
 - Deliver Current RTC (Real time clock), readings and system data.
 - Receive RTC, heartbeat and alarm data.
 - Disconnect from WolkSense sensor data cloud.
 - Disconnect from WiFi access point.
 - Turn off WiFi module.


**Connection Error Handling**
-------

During connection sequence various connection errors can occur. In case connection step lasts longer than specified timeout time, TIMEOUT event is triggered and connection sequence is aborted. When already connected, various communication errors or unexpected disconnects can happen. In case of any error, connection sequence will be aborted and started again when it is next scheduled. Collected sensor data will be saved and delivered to during next connection sequence.

In case connection fails for some reason two times in the row and device is working on battery power (USB is disconnected), device heartbeat interval value is doubled until it reaches maximum of 60 minutes. This is done to keep the power consumption low is cases of connectivity problems.

**Movement sensor operation**
-------

Whenever movement occurs and movement is enabled WolkSensor reports this event to the WolkSense sensor data cloud. Next movement can be reported after 2 seconds period.

**Readings**
-------

Data acquired from sensors is logged on device. Device should be able to store at least 3 hours of reading (up to 180 readings).

**System data**
-------

Data acquired while device is operational (battery voltage, connection duration and error) is logged on device. Device should be able to store at least 3 hours of this data (up to 60 readings).

**Error codes**
-------

Errors that occur during device operation should be logged on device. Errors are delivered to the WolkSense sensor data cloud with SYSTEM message in case of communication with WolkSense sensor data cloud or can be read in WolkSensor Assistant application.

**WolkSensor functional tests**
-------

WolkSensor functional tests help to ensure that the firmware is working correctly by checking whether data format is correct and satisfying in the matter of specific constraints, by covering different data flows and situations through which both true and false cases are brought to the device. What is also being tested: connection to the cloud, data publishing, device specific commands, sensor readings, device parameters, etc.

Functional tests consist of three sections:
1. Data driven tests
2. Flow tests
3. Robustness tests

At the end of each test section, it is desired to obtain an output in form of a *Test passed* message. If the output should be *Test failed*, please refer to the test log for details.

In order to run the functional tests, *WolkSensor-python-lib* library is required. This library is imported to the *WolkSensor-Firmware-* project as a  GitHub submodule.
