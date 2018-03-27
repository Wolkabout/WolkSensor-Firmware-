'''
Created on Dec 22th 2015
@author: sstankovic@execom.eu

Last Modified on July 29th 2016
@author: sstankovic@execom.eu

brief:  list of constant values
'''

BLESSERVERSIONMAJOR = 2
BLESSERVERSIONMINOR = 0
BLESSERVERSIONPATCH = 0

MANUFACTURER = 'FTDIBUS'
COMPORTNAME  = 'USB Serial Port'
VID          = 'VID_0403'
PID          = 'PID_6015'

SSID         = "wolkabout"
AUTH         = "WPA2"
PASS         = "Walkm3int0"

URL          = '52.213.16.227'
HOSTNAME     = 'api-demo.wolkabout.com'
PORT         = '8883'
RTC          = '1514824768' #1st January 2018.

TIMESTAMPSTRING     = 'R:'
PRESSURESTRING      = 'P:'
TEMPERATURESTRING   = 'T:'
HUMIDITYSTRING      = 'H:'
MOVEMENT            = 'M:'
	
BATTERY             = 'B:'
ACCESSPOINTTIME     = 'A:'
DHCPTIME            = 'D:'
SERVER              = 'S:'    
DATAEXCHANGETIME    = 'Q:'
DISCONNECTTIME      = 'L:'
CONNECTIONTIME      = 'C:'
	
ERROR = 'E:'

HB5   = '5'
HB10  = '10'
HB30  = '30'
HB60  = '60'

FILEBIN           = 'bin\wolksensor_cc3100_sensors.bin'
FILEBLESSED       = 'BlessedDevices.csv'
FILESERIALNUMBERS = '0116WS10Series.csv'

NAME = 'Functional Tests'
LOGO = '    _    _       _ _    _____                           \
   \n\r   | |  | |     | | |  /  ___|                          \
   \n\r   | |  | | ___ | | | _\ `--.  ___ _ __  ___  ___  _ __ \
   \n\r   | |/\| |/ _ \| | |/ /`--. \/ _ \  _ \/ __|/ _ \|  __|\
   \n\r   \  /\  / (_) | |   </\__/ /  __/ | | \__ \ (_) | |   \
   \n\r    \/  \/ \___/|_|_|\_\____/ \___|_| |_|___/\___/|_|   '

FAIL =  '\
		 \n\r\t _____ _____ ____ _____   _____ _    ___ _     _____ ____ \
		 \n\r\t|_   _| ____/ ___|_   _| |  ___/ \  |_ _| |   | ____|  _ \ \
		 \n\r\t  | | |  _| \___ \ | |   | |_ / _ \  | || |   |  _| | | | |\
		 \n\r\t  | | | |___ ___) || |   |  _/ ___ \ | || |___| |___| |_| |\
		 \n\r\t  |_| |_____|____/ |_|   |_|/_/   \_\___|_____|_____|____/ '
		 

PASSED ='\
		\n\r\t _____ _____ ____ _____   ____   _    ____ ____  _____ ____  \
		\n\r\t|_   _| ____/ ___|_   _| |  _ \ / \  / ___/ ___|| ____|  _ \ \
		\n\r\t  | | |  _| \___ \ | |   | |_) / _ \ \___ \___ \|  _| | | | |\
		\n\r\t  | | | |___ ___) || |   |  __/ ___ \ ___) |__) | |___| |_| |\
		\n\r\t  |_| |_____|____/ |_|   |_| /_/   \_\____/____/|_____|____/ '

QUIT      = -1
IDLENGTH  = 16
SIGNATURE = '****'
MACLENGTH = 12
