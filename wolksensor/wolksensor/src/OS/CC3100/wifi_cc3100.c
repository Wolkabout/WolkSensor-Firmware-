#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// OS
#include "logger.h"
#include "config.h"
#include "brd.h"
#include "sensors/sensor.h"
#include "sensors/batteryADC.h"
#include "event_buffer.h"
#include "chrono.h"
#include "clock.h"
#include "inet.h"
#include "commands.h"
#include "wifi_communication_module.h"
#include "wifi_communication_module_dependencies.h"
#include "wifi_cc3100.h"
#include "global_dependencies.h"
#include "simplelink.h"
#include "device.h"
#include "spi.h"
#include "sl_common.h"
#include "source/protocol.h"
#include "commands_dependencies.h"
#include <util/delay.h>
#include "flc_api.h"
#include "util_conversions.h"
#include "time.h"

#define CA_CERT "ca.der"
//#define CA_CERT "ca_cert.der"
#define CA_CERT_VERSION "ca_version.txt"

static void (*wifi_connected_listener)(void) = NULL;
static void (*wifi_ip_address_acquired_listener)(void) = NULL;
static void (*wifi_disconnected_listener)(void) = NULL;
static void (*wifi_error_listener)(void) = NULL;
static void (*wifi_socket_closed_listener)(void) = NULL;
static void (*wifi_platform_specific_error_code_listener)(uint32_t operation_code) = NULL;

static char cc3100_ssid[MAX_WIFI_SSID_SIZE + 1];
static char cc3100_password[MAX_WIFI_PASSWORD_SIZE + 1];
static uint8_t cc3100_auth_type;

static char cc3100_static_ip[MAX_WIFI_STATIC_IP_SIZE];
static char cc3100_static_mask[MAX_WIFI_STATIC_MASK_SIZE];
static char cc3100_static_gateway[MAX_WIFI_STATIC_GATEWAY_SIZE];
static char cc3100_static_dns[MAX_WIFI_STATIC_DNS_SIZE];

char server_ip[MAX_SERVER_IP_SIZE + 1];
tcp_communication_module_data_t tcp_communication_module_data;

static volatile bool fast_connect = false;

 static _u32 cipher = SL_SEC_MASK_TLS_RSA_WITH_AES_256_CBC_SHA;
 static _u8 method = SL_SO_SEC_METHOD_SSLv3_TLSV1_2;
 
 #define CA_CERTIFICATE_VERSION 1

static uint32_t create_CC3100_error_code(uint16_t command, int16_t error)
{
	uint32_t error_code = command;
	error_code <<= 16;
	error_code |= (0x0000FFFF & (uint32_t)error);
	return error_code;
}

void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
	LOG(1, "General error event");
	
	if(pDevEvent == NULL)
	{
		LOG(1, "General event NULL error");
		
		wifi_platform_specific_error_code_listener(create_CC3100_error_code(0, 0));
		wifi_error_listener();
		
		return;
	}
	
	switch(pDevEvent->Event)
	{
		case SL_DEVICE_FATAL_ERROR_EVENT:
		{
			LOG(1, "General event fatal error");
			
			break;
		}
		case SL_DEVICE_ABORT_ERROR_EVENT:
		{
			LOG(1, "General event abort error");
			
			break;
		}
		default:
		{
			LOG(1, "General event unknown error");
			
			break;
		}
	}
	
	wifi_platform_specific_error_code_listener(create_CC3100_error_code(0, pDevEvent->Event));
	wifi_error_listener();
}

void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
	if(pWlanEvent == NULL)
    {
        LOG(1, "Wlan event NULL pointer Error");
		
		wifi_platform_specific_error_code_listener(create_CC3100_error_code(1, 0));
		wifi_error_listener();
		
        return;
    }
    
    switch(pWlanEvent->Event)
    {
        case SL_WLAN_CONNECT_EVENT:
        {	
			LOG(1, "CC3100 connected to AP");
			
			wifi_connected_listener();
			
            break;
        }
        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            if(SL_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
                LOG(1, "CC3100 disconnected from the AP on application's request");
				
				wifi_disconnected_listener();
            }
            else
            {
                LOG(1, "CC3100 disconnected from the AP on an error");
				
				wifi_platform_specific_error_code_listener(create_CC3100_error_code(1, pEventData->reason_code));
				wifi_disconnected_listener();
            }
			break;
        }
        default:
        {
            LOG(1, "Wlan unexpected event");
			
			wifi_platform_specific_error_code_listener(create_CC3100_error_code(1, pWlanEvent->Event));
			wifi_error_listener();
			
			break;
        }
    }
}

void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
	if(pNetAppEvent == NULL)
    {
        LOG(1, "Netapp event NULL pointer error");
		
		wifi_platform_specific_error_code_listener(create_CC3100_error_code(2, 0));
		wifi_error_listener();
		
        return;
    }
 
    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
			LOG_PRINT(1, PSTR("\n\rIP add: %d.%d.%d.%d \n\rGateway add: %d.%d.%d.%d \n\rDNS add: %d.%d.%d.%d \n\r"), \
			(int)SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,3), (int)SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,2), (int)SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,1), (int)SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,0), \
			(int)SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,3), (int)SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,2), (int)SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,1), (int)SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,0), \
			(int)SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.dns,3), (int)SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.dns,2), (int)SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.dns,1), (int)SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.dns,0));
			
			fast_connect = true;
			
			wifi_ip_address_acquired_listener();

			break;
        }
        default:
        {
            LOG(1, "Netapp unexpected event");
			
			wifi_platform_specific_error_code_listener(create_CC3100_error_code(2, pNetAppEvent->Event));
			wifi_error_listener();
        }
    }
}

void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
	if(pSock == NULL)
    {
        LOG(1, "Socket event NULL pointer Error");
		
		wifi_platform_specific_error_code_listener(create_CC3100_error_code(3, 0));
		wifi_error_listener();
		
        return;
    }
	
    switch( pSock->Event )
    {
        case SL_SOCKET_TX_FAILED_EVENT:
		{
			LOG(1, "Socket failed event");
      
            switch( pSock->socketAsyncEvent.SockTxFailData.status )
            {
                case SL_ECLOSE:
				{
                    LOG(1, "Socket closed error");
				}
                default:
				{
					wifi_platform_specific_error_code_listener(create_CC3100_error_code(3, pSock->Event << 8 | (uint32_t)pSock->socketAsyncEvent.SockTxFailData.status));
                    wifi_socket_closed_listener();
					
                    break;
				}
            }
			
            break;
		}
		case SL_SOCKET_ASYNC_EVENT:
		{
			switch( pSock->socketAsyncEvent.SockAsyncData.type )
			{
				case SSL_ACCEPT:
				{
					LOG(1, "SSL accept event");
					
					break;
				}
				case RX_FRAGMENTATION_TOO_BIG:
				{
					LOG(1, "RX fragmentation too big event");
					
					wifi_platform_specific_error_code_listener(create_CC3100_error_code(3, (pSock->Event << 8) | pSock->socketAsyncEvent.SockAsyncData.type));
					 wifi_socket_closed_listener();
					
					break;
				}
				case OTHER_SIDE_CLOSE_SSL_DATA_NOT_ENCRYPTED:
				{
					LOG(1, "Other side colse ssl not encrypted event");
					
					wifi_platform_specific_error_code_listener(create_CC3100_error_code(3, (pSock->Event << 8) | (uint32_t)pSock->socketAsyncEvent.SockAsyncData.type));
					wifi_error_listener();
					
					break;
				}
			}
			
			break;
		}
        default:
		{
            LOG(1, "Socket unexpected event");
			
			wifi_platform_specific_error_code_listener(create_CC3100_error_code(3, pSock->Event));
			wifi_error_listener();
			
            break;
		}
    }
}

void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent, SlHttpServerResponse_t *pHttpResponse)
{
	// not used
}

void CC3100_enable(void)
{
	LOG(1, "Enabling cc3100");
	
	WriteWlanPin(1);
}

void CC3100_disable(void)
{
	LOG(1, "Disabling cc3100");
	
	WriteWlanPin(0);
}

bool CC3100_process(void)
{
	_SlNonOsMainLoopTask();
	
	return true;
}

static uint8_t get_surroundig_networks(wifi_network_t* networks, uint8_t networks_size)
{
	/* set scan policy - this starts the scan */
	uint8_t policy_option = SL_SCAN_POLICY(1);
	uint32_t policy_value = 10; 
	
	if(sl_WlanPolicySet(SL_POLICY_SCAN , policy_option, (_u8 *)&policy_value, sizeof(policy_value)) < 0)
	{
		return 0;
	}
	
	watchdog_reset();
	
	_delay_ms(5000);
	 
	Sl_WlanNetworkEntry_t net_entry = {0}; 
	uint16_t idx = 0;
	uint16_t running_idx = 10;
	
	do
	{
		running_idx--;
		
		if(sl_WlanGetNetworkList(running_idx, 1, &net_entry) < 1)
		{
			break;
		}
		     
		bool duplicate = false;	 
		uint8_t i;
		for(i = 0; i < idx; i++)
		{
			if(memcmp(net_entry.bssid, networks[i].bssid, SL_BSSID_LENGTH) == 0)
			{
				/* Duplicate entry */
				duplicate = true;
				break;
			}
		}	     
		
		if(!duplicate)
		{
			pal_Memcpy(&networks[idx].bssid, &net_entry.bssid, 6);
			strcpy(networks[idx].ssid, net_entry.ssid);
			networks[idx].rssi = net_entry.rssi;
			
			LOG_PRINT(1, PSTR("Network %s - %02X:%02X:%02X:%02X:%02X:%02X  %d\r\n"), net_entry.ssid, net_entry.bssid[0], net_entry.bssid[1], net_entry.bssid[2], net_entry.bssid[3], net_entry.bssid[4], net_entry.bssid[5], net_entry.rssi);
			
			idx++;
		}
	}
	while ((running_idx > 0) && (idx < networks_size));
	
	policy_option = SL_SCAN_POLICY(0);
	sl_WlanPolicySet(SL_POLICY_SCAN, policy_option, NULL, 0);
	
	return idx;
}

static _i32 SetTime(void)
{
	_i32 retVal = -1;
	SlDateTime_t dateTime= {0};

	uint32_t rtc = rtc_get_ts();
	struct tm *times = gmtime(&rtc);

	dateTime.sl_tm_day = 1+times->tm_mday;
	dateTime.sl_tm_mon = 1+times->tm_mon;
	dateTime.sl_tm_year = 1870+times->tm_year;
	dateTime.sl_tm_hour = times->tm_hour;
	dateTime.sl_tm_min = times->tm_min;
	dateTime.sl_tm_sec = times->tm_sec;

	retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
	sizeof(SlDateTime_t),(_u8 *)(&dateTime));
	ASSERT_ON_ERROR(retVal);

	return SUCCESS;
}

static bool update_ca_cert(void)
{
	LOG(1, "Updating ca certificate");

	uint32_t token = 0;	
	uint8_t ca_version = 0;
	
	//sl_FsDel(CA_CERT, token);
	
	int32_t ca_version_file = 0;
	int32_t file_opened = sl_FsOpen(CA_CERT_VERSION, FS_MODE_OPEN_READ, &token, &ca_version_file);
	if(file_opened != 0)
	{
		LOG(1, "Ca version file does not exist, creating");
		file_opened = sl_FsOpen(CA_CERT_VERSION, FS_MODE_OPEN_CREATE(1, _FS_FILE_OPEN_FLAG_COMMIT|_FS_FILE_PUBLIC_WRITE), &token, &ca_version_file);
		if(file_opened < 0)
		{
			LOG_PRINT(1, PSTR("Error creating ca version file %d\r\n"), (int16_t)file_opened);
			return false;
		}
	}
	else
	{
		int32_t bytes_read = sl_FsRead(ca_version_file, 0, &ca_version, 1);
		if (bytes_read != 1)
		{
			LOG(1, "Error reading ca version from file");
			//return false;
		}
	}
	
	LOG_PRINT(1, PSTR("Ca version is %d\r\n"), ca_version);
	
	int16_t ca_version_file_closed = sl_FsClose(ca_version_file, 0, 0, 0);
	if(ca_version_file_closed < 0)
	{
		LOG(1, "Error closing ca version file");
		return false;
	}
	
	// version check
	if(ca_version == CA_CERTIFICATE_VERSION)
	{
		return true;
	}

	LOG(1, "Deleting old cert file");
	sl_FsDel(CA_CERT, token);

	//ca.crt file
	const uint8_t cert[] = {0x30, 0x82, 0x03, 0xF9, 0x30, 0x82, 0x02, 0xE1, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x09, 0x00, 0xBC, 0xE2, 0x55, 0xD4, 0x2D, 0x7F, 0xF7, 0x1D, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B, 0x05, 0x00, 0x30, 0x81, 0x91, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x52, 0x53, 0x31, 0x0F, 0x30, 0x0D, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0C, 0x06, 0x53, 0x65, 0x72, 0x62, 0x69, 0x61, 0x31, 0x11, 0x30, 0x0F, 0x06, 0x03, 0x55, 0x04, 0x07, 0x0C, 0x08, 0x4E, 0x6F, 0x76, 0x69, 0x20, 0x53, 0x61, 0x64, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x09, 0x77, 0x6F, 0x6C, 0x6B, 0x61, 0x62, 0x6F, 0x75, 0x74, 0x31, 0x0C, 0x30, 0x0A, 0x06, 0x03, 0x55, 0x04, 0x0B, 0x0C, 0x03, 0x52, 0x26, 0x44, 0x31, 0x16, 0x30, 0x14, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x0D, 0x77, 0x6F, 0x6C, 0x6B, 0x61, 0x62, 0x6F, 0x75, 0x74, 0x2E, 0x63, 0x6F, 0x6D, 0x31, 0x24, 0x30, 0x22, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x01, 0x16, 0x15, 0x73, 0x75, 0x70, 0x70, 0x6F, 0x72, 0x74, 0x40, 0x77, 0x6F, 0x6C, 0x6B, 0x61, 0x62, 0x6F, 0x75, 0x74, 0x2E, 0x63, 0x6F, 0x6D, 0x30, 0x20, 0x17, 0x0D, 0x31, 0x35, 0x31, 0x32, 0x31, 0x30, 0x31, 0x32, 0x33, 0x39, 0x31, 0x30, 0x5A, 0x18, 0x0F, 0x32, 0x30, 0x36, 0x35, 0x31, 0x31, 0x32, 0x37, 0x31, 0x32, 0x33, 0x39, 0x31, 0x30, 0x5A, 0x30, 0x81, 0x91, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x52, 0x53, 0x31, 0x0F, 0x30, 0x0D, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0C, 0x06, 0x53, 0x65, 0x72, 0x62, 0x69, 0x61, 0x31, 0x11, 0x30, 0x0F, 0x06, 0x03, 0x55, 0x04, 0x07, 0x0C, 0x08, 0x4E, 0x6F, 0x76, 0x69, 0x20, 0x53, 0x61, 0x64, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x09, 0x77, 0x6F, 0x6C, 0x6B, 0x61, 0x62, 0x6F, 0x75, 0x74, 0x31, 0x0C, 0x30, 0x0A, 0x06, 0x03, 0x55, 0x04, 0x0B, 0x0C, 0x03, 0x52, 0x26, 0x44, 0x31, 0x16, 0x30, 0x14, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x0D, 0x77, 0x6F, 0x6C, 0x6B, 0x61, 0x62, 0x6F, 0x75, 0x74, 0x2E, 0x63, 0x6F, 0x6D, 0x31, 0x24, 0x30, 0x22, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x01, 0x16, 0x15, 0x73, 0x75, 0x70, 0x70, 0x6F, 0x72, 0x74, 0x40, 0x77, 0x6F, 0x6C, 0x6B, 0x61, 0x62, 0x6F, 0x75, 0x74, 0x2E, 0x63, 0x6F, 0x6D, 0x30, 0x82, 0x01, 0x22, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0F, 0x00, 0x30, 0x82, 0x01, 0x0A, 0x02, 0x82, 0x01, 0x01, 0x00, 0xAE, 0xD0, 0xB6, 0x27, 0xF2, 0x7C, 0x54, 0xA8, 0x57, 0x32, 0x4B, 0x5A, 0xE8, 0xEA, 0x30, 0xA5, 0x62, 0x80, 0x17, 0x48, 0x77, 0x09, 0xAC, 0x03, 0x52, 0x54, 0xD8, 0x7B, 0x0F, 0x27, 0x9E, 0x57, 0xA0, 0xA7, 0xE7, 0x0C, 0xD6, 0xD6, 0x8A, 0xC8, 0xDB, 0xAF, 0x51, 0xDE, 0x41, 0x66, 0x7B, 0xA2, 0x8F, 0xB8, 0x2F, 0x1D, 0x71, 0xA4, 0xE5, 0x00, 0x27, 0x75, 0x16, 0x7D, 0xFD, 0xF2, 0x59, 0xC4, 0x0C, 0x7D, 0xC6, 0x68, 0xBA, 0xD9, 0x9E, 0x37, 0xEF, 0x6D, 0x65, 0x9F, 0x1F, 0xF7, 0x61, 0x18, 0x1F, 0x6C, 0x4D, 0x75, 0x64, 0x5C, 0x2D, 0x3F, 0x1F, 0x32, 0x36, 0x0D, 0xBA, 0x2E, 0x3C, 0x6C, 0xC1, 0x71, 0x38, 0x7E, 0x25, 0x12, 0x20, 0x59, 0xF2, 0x78, 0xF7, 0x07, 0xBC, 0x47, 0xE7, 0x39, 0x83, 0xEC, 0x4C, 0xF9, 0x7F, 0xB8, 0xAB, 0x43, 0xE8, 0x3B, 0x09, 0x28, 0x9D, 0x63, 0xB9, 0x66, 0x47, 0xF5, 0x46, 0x49, 0xE7, 0x16, 0xDE, 0x6B, 0x31, 0xCA, 0x49, 0x5D, 0xD4, 0x5D, 0xD9, 0xF3, 0xF2, 0x70, 0x8C, 0x9B, 0xFD, 0x42, 0x91, 0xAE, 0xA9, 0xF8, 0xF6, 0xBC, 0xA2, 0x9F, 0x78, 0xFD, 0xF7, 0xD6, 0x7B, 0xAE, 0xB7, 0x36, 0x25, 0x25, 0xEF, 0x59, 0x11, 0x1F, 0x73, 0x70, 0x8F, 0xC1, 0x26, 0xDC, 0x50, 0x1B, 0xCA, 0x06, 0xC3, 0x4F, 0xDD, 0x43, 0x8C, 0xB7, 0xF0, 0xA3, 0x0D, 0x6E, 0x39, 0x4A, 0x59, 0x99, 0xE8, 0x01, 0x97, 0x36, 0x21, 0x8E, 0x0A, 0xAE, 0x21, 0x93, 0x50, 0x29, 0x54, 0x97, 0x7F, 0xA4, 0x73, 0x1D, 0xA0, 0x31, 0xD7, 0x55, 0x8C, 0xAF, 0x96, 0xF9, 0xBC, 0x65, 0x05, 0xD1, 0x81, 0x5C, 0xC7, 0xA0, 0x2E, 0xA7, 0x0D, 0x29, 0x95, 0x0F, 0x10, 0xBB, 0x69, 0xEA, 0x71, 0x48, 0xDA, 0x56, 0x62, 0x6C, 0xF1, 0x26, 0x8C, 0x9E, 0xC4, 0xEB, 0x85, 0xF5, 0x49, 0x83, 0x02, 0x03, 0x01, 0x00, 0x01, 0xA3, 0x50, 0x30, 0x4E, 0x30, 0x1D, 0x06, 0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16, 0x04, 0x14, 0x0B, 0x93, 0x9C, 0x4B, 0x1B, 0xB1, 0xE4, 0xCF, 0xB7, 0x0C, 0xFB, 0x1A, 0x37, 0x54, 0x4A, 0x28, 0xCC, 0x7D, 0x31, 0x9F, 0x30, 0x1F, 0x06, 0x03, 0x55, 0x1D, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0x0B, 0x93, 0x9C, 0x4B, 0x1B, 0xB1, 0xE4, 0xCF, 0xB7, 0x0C, 0xFB, 0x1A, 0x37, 0x54, 0x4A, 0x28, 0xCC, 0x7D, 0x31, 0x9F, 0x30, 0x0C, 0x06, 0x03, 0x55, 0x1D, 0x13, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01, 0xFF, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B, 0x05, 0x00, 0x03, 0x82, 0x01, 0x01, 0x00, 0x9C, 0x47, 0xE8, 0x72, 0xB8, 0xEC, 0x2D, 0xC7, 0xF6, 0x74, 0x27, 0x80, 0xE4, 0x6E, 0xEC, 0x9E, 0xA4, 0x11, 0xFD, 0x5D, 0x8A, 0x32, 0x6E, 0x95, 0x51, 0x1D, 0x28, 0x19, 0xA3, 0x98, 0x2B, 0xA0, 0x03, 0x2C, 0x3A, 0x07, 0x9B, 0xE1, 0xDB, 0x09, 0x89, 0x4C, 0x3F, 0x93, 0xFA, 0x34, 0x62, 0x62, 0xC4, 0x10, 0xA7, 0xBE, 0xF6, 0xD3, 0xFD, 0xC2, 0x22, 0xCA, 0x29, 0xDE, 0x19, 0x03, 0x53, 0x01, 0xFF, 0x19, 0xF9, 0x73, 0x5C, 0x37, 0xA2, 0x0E, 0xCB, 0x20, 0x7A, 0xA7, 0xC3, 0x6A, 0x01, 0x3C, 0xE7, 0x7E, 0xBF, 0x88, 0x95, 0x64, 0xC0, 0x6B, 0xFA, 0xA6, 0x2D, 0xFA, 0x71, 0xF1, 0x42, 0x18, 0x4B, 0x93, 0x4F, 0xA4, 0x6D, 0x28, 0x52, 0x94, 0x0F, 0xB2, 0xE5, 0x0B, 0x4C, 0x1E, 0x07, 0xE2, 0xF3, 0xAC, 0xA3, 0x05, 0xF1, 0x6A, 0xB9, 0x14, 0x82, 0xF9, 0x4E, 0xA1, 0x7C, 0x37, 0xA0, 0xBE, 0x63, 0x56, 0xF5, 0x99, 0xE9, 0xC8, 0x6F, 0x14, 0xBE, 0x96, 0x92, 0x40, 0x5A, 0xA7, 0xCE, 0x08, 0x2B, 0xBF, 0x23, 0xE2, 0xBA, 0x9A, 0x6D, 0x35, 0x95, 0x44, 0x0F, 0x97, 0xE7, 0x92, 0x8D, 0xE6, 0x79, 0x5A, 0x5E, 0xDA, 0x7E, 0xEA, 0x95, 0xB8, 0x5D, 0x84, 0x37, 0x30, 0x20, 0x5F, 0x58, 0x57, 0x6C, 0xB5, 0x83, 0xAF, 0xCF, 0xDA, 0x4D, 0xF3, 0x94, 0xAF, 0xE4, 0x68, 0x09, 0x0E, 0xEC, 0xA5, 0x83, 0xF1, 0x87, 0x19, 0x78, 0x3C, 0x88, 0xB1, 0x3B, 0x63, 0xF0, 0xA4, 0xB7, 0xFE, 0xEB, 0x8D, 0x47, 0xED, 0x35, 0x5F, 0x01, 0x1F, 0xDE, 0x66, 0x90, 0x0F, 0x5F, 0xEE, 0xF6, 0xBF, 0x54, 0x08, 0x17, 0x0A, 0xDF, 0x58, 0xD6, 0x2F, 0xA9, 0x00, 0x49, 0xBD, 0x46, 0x6E, 0xA9, 0xD2, 0x18, 0xBE, 0x85, 0xAD, 0x9F, 0x38, 0x8F, 0x28, 0x6B, 0xB6, 0xB2, 0x65, 0xD1, 0xE8, 0xFE, 0x28, 0x6B, 0xA5};
	uint32_t cert_size = sizeof(cert);

	int32_t ca_file = 0;
	file_opened = sl_FsOpen(CA_CERT, FS_MODE_OPEN_CREATE(cert_size, _FS_FILE_OPEN_FLAG_COMMIT|_FS_FILE_PUBLIC_WRITE), &token, &ca_file);
	if(file_opened < 0)
	{
		LOG_PRINT(1, PSTR("Error creating ca file %d\r\n"), (int16_t)file_opened);
		return false;
	}
	
	if(sl_FsWrite(ca_file, 0, cert, cert_size) != cert_size)
	{
		LOG(1, "Cert not written succefully");
		return false;
	}
	
	LOG(1, "Cert written succefully");
	
	int16_t ca_file_closed = sl_FsClose(ca_file, 0, 0, 0);
	if(ca_file_closed < 0)
	{
		LOG(1, "Error closing ca file");
		return false;
	}
	
	
	file_opened = sl_FsOpen("ca_version.txt", FS_MODE_OPEN_WRITE, &token, &ca_version_file);
	if(file_opened != 0)
	{
		LOG(1, "Error opeening ca version file");
	}
	
	uint8_t version = CA_CERTIFICATE_VERSION;
	if (sl_FsWrite(ca_version_file, 0, &version, 1) != 1)
	{
		LOG(1, "Error writing ca version to file");
		return false;
	}
	
	ca_version_file_closed = sl_FsClose(ca_version_file, 0, 0, 0);
	if(ca_version_file_closed < 0)
	{
		LOG(1, "Error closing ca version file");
		return false;
	}
		
	return true;
}

bool init_wifi(void)
{
	LOG(1, "CC3100 init");
	
	int16_t mode = sl_Start(0, 0, 0);
	if(mode >= 0)
	{
		LOG_PRINT(1, PSTR("Mode is %d\r\n"), mode);
	}
	else
	{
		LOG(1, "Unable to start CC3100");
		
		return false;
	}

	SlVersionFull ver;
	uint8_t configOpt = SL_DEVICE_GENERAL_VERSION;
	uint8_t configLen = sizeof(ver);
	if(sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &configOpt, &configLen, (uint8_t *)(&ver)) < 0)
	{
		LOG(1, "Unable to get CC3100 firmware version");
		
		return false;
	}
	
	unsigned char mac_address_length = SL_MAC_ADDR_LEN;
	if(sl_NetCfgGet(SL_MAC_ADDRESS_GET,NULL, &mac_address_length, (unsigned char *)mac_address_nwmem) < 0)
	{
		LOG(1, "Unable to get CC3100 mac address");
		
		return false;
	}
	
	if(sl_WlanPolicySet(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(0, 0, 0, 0, 0), NULL, 0) < 0)
	{
		LOG(1, "Unable to set connection policy");	
		
		return false;
	}
	
	configOpt = SL_SCAN_POLICY(0);
	if(sl_WlanPolicySet(SL_POLICY_SCAN , configOpt, NULL, 0))
	{
		LOG(1, "Unable to disable scan");
		
		return false;
	}

	_u8 power = 0;
	if(sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, WLAN_GENERAL_PARAM_OPT_STA_TX_POWER, 1, (_u8 *)&power) < 0)
	{
		LOG(1, "Unable to set TX power");
		
		return false;
	}
	
	LOG(1, "Unregister mDNS service");
	if(sl_NetAppMDNSUnRegisterService(0, 0) < 0)
	{
		LOG(1, "Unable to unregister mDNS service");
		
		return false;
	}

	_WlanRxFilterOperationCommandBuff_t  RxFilterIdMask = {0};
	pal_Memset(RxFilterIdMask.FilterIdMask, 0xFF, 8);
	if(sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (_u8 *)&RxFilterIdMask, sizeof(_WlanRxFilterOperationCommandBuff_t)) < 0)
	{
		LOG(1, "Unable to remove all 64 filters (8*8)");
		
		return false;
	}
	
	/* Remove all profiles */
	if(sl_WlanProfileDel(0xFF) < 0)
	{
		LOG(1, "Unable to delete profiles");

		return false;
	}
	
	SetTime();
	
	update_ca_cert();
	
	if(sl_Stop(1000) < 0)
	{
		LOG(1, "Unable to stop CC3100");

		return false;
	}
	
	commands_dependencies.get_surroundig_wifi_networks = get_surroundig_networks;
	 
	return true;
}

bool wifi_start(void)
{
	LOG(1, "CC3100 start");
	
	int16_t start_result = sl_Start(0, 0, 0);
	if(start_result < 0)
	{
		wifi_platform_specific_error_code_listener(create_CC3100_error_code(0, start_result));
		
		return false;
	}
	
	return true;
}

static bool static_ip_parameters_changed(void)
{
	return (strcmp(cc3100_static_ip, wifi_static_ip) != 0) || (strcmp(cc3100_static_mask, wifi_static_mask) != 0) || (strcmp(cc3100_static_gateway, wifi_static_gateway) != 0) || (strcmp(cc3100_static_dns, wifi_static_dns) != 0);
}

static bool static_ip_set(void)
{
	return strcmp(cc3100_static_ip, "") != 0 && strcmp(cc3100_static_mask, "") != 0 && strcmp(cc3100_static_gateway, "") != 0 && strcmp(cc3100_static_dns, "") != 0;
}

static bool wifi_parameters_changed(void)
{
	return (strcmp(cc3100_ssid, wifi_ssid) != 0) || (strcmp(cc3100_password, wifi_password) != 0) || (cc3100_auth_type != wifi_auth_type);
}

bool wifi_connect(char* ssid, char* password, uint8_t auth_type)
{
	LOG(1, "CC3100 connect");
	
	if(wifi_parameters_changed())
	{
		LOG(1,"Wifi parameters changed");
		
		strcpy(cc3100_ssid, wifi_ssid);
		strcpy(cc3100_password, wifi_password);
		cc3100_auth_type = wifi_auth_type;

		fast_connect = false;
	}

	if(static_ip_parameters_changed())
	{
		LOG(1,"Static IP parameters changed");
		
		strcpy(cc3100_static_ip, wifi_static_ip);
		strcpy(cc3100_static_mask, wifi_static_mask);
		strcpy(cc3100_static_gateway, wifi_static_gateway);
		strcpy(cc3100_static_dns, wifi_static_dns);
		
		if(static_ip_set())
		{
			LOG(1, "Setting static ip");
			
			SlNetCfgIpV4Args_t ipV4;
			ipV4.ipV4 = inet_aton(cc3100_static_ip);
			ipV4.ipV4Mask = inet_aton(cc3100_static_mask);
			ipV4.ipV4Gateway = inet_aton(cc3100_static_gateway);
			ipV4.ipV4DnsServer = inet_aton(cc3100_static_dns);
			
			int16_t net_cfg_result = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_STATIC_ENABLE, 1, sizeof(SlNetCfgIpV4Args_t), (_u8 *)&ipV4);
			if(net_cfg_result < 0)
			{
				LOG(1, "Unable to set static IP address");
				
				wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_DEVICE_NETCFG_SET_COMMAND, net_cfg_result));
				
				return false;
			}
			
			if(!wifi_stop())
			{
				LOG(1, "Unable to stop wifi");
				return false;
			}
			
			if(!wifi_start())
			{
				LOG(1, "Unable to start wifi");
				return false;
			}
		}
		else
		{
			LOG(1, "Setting dynamic ip");
			
			uint8_t val = 1;
			int16_t net_cfg_result = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE, 1, 1, &val);
			if(net_cfg_result < 0)
			{
				LOG(1, "Unable to enable dhcp");
				
				wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_DEVICE_NETCFG_SET_COMMAND, net_cfg_result));
				
				return false;
			}
			
			if(!wifi_stop())
			{
				LOG(1, "Unable to stop wifi");
				return false;
			}
			
			if(!wifi_start())
			{
				LOG(1, "Unable to start wifi");
				return false;
			}
		}
		
		fast_connect = false;
	}
	
	//if(fast_connect)
	//{		
	//	LOG(1, "CC3100 fast connect");
	//}
	//else
	{
		LOG(1, "CC3100 regular connect");
		
		SlSecParams_t security_parameters = {0};

		char password[MAX_WIFI_PASSWORD_SIZE + 1];

		if(cc3100_auth_type == WIFI_SECURITY_WEP)
			hexstring_to_asciistring(password, cc3100_password);
		else
			strcpy(password, cc3100_password);

		if(cc3100_auth_type == WIFI_SECURITY_WPA)
			cc3100_auth_type = WIFI_SECURITY_WPA2;

		security_parameters.Key = password;
		security_parameters.KeyLen = pal_Strlen(password);
		security_parameters.Type = cc3100_auth_type;

		int16_t connect_result = sl_WlanConnect(cc3100_ssid, strlen(cc3100_ssid), 0, &security_parameters, 0);
		if(connect_result < 0)
		{
			LOG(1, "Unable to connect");
			
			wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_WLAN_WLANCONNECTCOMMAND, connect_result));
			
			return false;
		}
		
		/*
		int16_t set_policy_result = sl_WlanPolicySet(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(0, 1, 0, 0, 0), NULL, 0);
		if(set_policy_result < 0)
		{
			LOG(1, "Connection policy could not be set");
			
			wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_WLAN_POLICYSETCOMMAND, set_policy_result));
			
			return false;
		}
		
		int16_t add_profile_result = sl_WlanProfileAdd((_i8 *)wifi_ssid, strlen(wifi_ssid), NULL, &security_parameters, 0, 7, 0);
		if(add_profile_result < 0)
		{
			LOG(1, "Unable to add profile");
			
			wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_WLAN_PROFILEADDCOMMAND, add_profile_result));
			
			return false;
		}
		*/
	}
	
	return true;
}

bool wifi_disconnect(void)
{
	LOG(1, "CC3100 disconnect");
	
	sl_WlanDisconnect();
	
	return true;
}

bool wifi_stop(void)
{
	LOG(1, "CC3100 stop");
	
	int16_t stop_result = sl_Stop(0);
	if(stop_result < 0)
	{
		wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_DEVICE_STOP_COMMAND, stop_result));
		
		return false;
	}
	
	return true;
}

bool wifi_reset(void)
{
	LOG(1, "CC3100 reset");
	
	fast_connect = false; 
	
	if(!wifi_stop())
	{
		return false;
	}
	
	if(!wifi_start())
	{
		return false;
	}
	
	sl_WlanDisconnect();
	
	int16_t set_policy_result = sl_WlanPolicySet(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(0, 0, 0, 0, 0), NULL, 0);
	if(set_policy_result < 0)
	{
		LOG(1, "Unable to clear connection policy CC3100");
		
		wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_WLAN_POLICYSETCOMMAND, set_policy_result));
		
		return false;
	}

	int16_t delete_profiles_result = sl_WlanProfileDel(0xFF); 
	if(delete_profiles_result < 0)
	{
		LOG(1, "Unable to delete connection profiles CC3100");
		
		wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_WLAN_PROFILEDELCOMMAND, delete_profiles_result));
		
		return false;
	}
	 
	return true;
}

int wifi_open_socket(char* address, uint16_t port, bool secure)
{
	uint32_t ip_address;
	unsigned long ulIpAddr = 0;

	LOG_PRINT(1, PSTR("Server IP/URL is: %s\n"), server_ip);

	if(inet_pton_ipv4(server_ip, NULL))
	{
		LOG(1, "Connect to the server with IP address\n");

		ip_address = inet_aton(server_ip);
		strcpy(server_ip,inet_ntoa(ip_address));
		LOG_PRINT(1,PSTR("Server IP is : %s\n"),server_ip);
	}

	if(((int)tcp_communication_module_data.platform_specific_error_code < 0) || (!inet_pton_ipv4(server_ip,NULL)))
	{
		LOG(1, "DNS Get Host IP address By Name.\n");

		long lRetVal = sl_NetAppDnsGetHostByName((signed char*)hostname, sizeof(hostname), &ulIpAddr, SL_AF_INET);
		if(lRetVal < 0 || ulIpAddr == 0)
		{
			LOG(1, "Error to get IP address from DNS");
			return -1;
		}

		tcp_communication_module_data.error = 0;

		ip_address = ulIpAddr;

		strcpy(server_ip,inet_ntoa(ip_address));

		LOG_PRINT(1,PSTR("NEW SERVER IP : %s\n"),server_ip);
	}

	LOG_PRINT(1, PSTR("CC3100 open socket %s\r\n"), secure ? "secure" : "unsecure");
	int16_t socket_id = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, (secure ? SL_SEC_SOCKET : 0));
	if(socket_id < 0)
	{
		LOG(1, "CC3100 unable to create TCP socket");
		
		wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_SOCKET_SOCKET, socket_id));
		
		return socket_id;
	}
	
	SlSockNonblocking_t socket_non_blocking;
	socket_non_blocking.NonblockingEnabled = 1;
	int16_t set_socket_option_result = sl_SetSockOpt(socket_id, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &socket_non_blocking, sizeof(SlSockNonblocking_t));
	if(set_socket_option_result < 0)	{		LOG(1, "CC3100 unable to set socket non-blocking mode");				wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_SOCKET_SETSOCKOPT, set_socket_option_result));				return -1;	}		if(secure)
	{
		/* configure the socket as SSLV3.0 */
		set_socket_option_result = sl_SetSockOpt(socket_id, SL_SOL_SOCKET, SL_SO_SECMETHOD, &method, sizeof(method));
		if(set_socket_option_result < 0)
		{
			LOG(1, "CC3100 unable to set socket security methods");
			
			wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_SOCKET_SETSOCKOPT, set_socket_option_result));
			
			return -1;
		}

		set_socket_option_result = sl_SetSockOpt(socket_id, SL_SOL_SOCKET, SL_SO_SECURE_MASK, &cipher, sizeof(cipher));
		if(set_socket_option_result < 0)
		{
			LOG(1, "CC3100 unable to set socket security mask");
			
			wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_SOCKET_SETSOCKOPT, set_socket_option_result));
			
			return -1;
		}

		set_socket_option_result = sl_SetSockOpt(socket_id, SL_SOL_SOCKET, SL_SO_SECURE_FILES_CA_FILE_NAME, CA_CERT, pal_Strlen(CA_CERT));
		if( set_socket_option_result < 0 )
		{
			LOG(1, "CC3100 unable to set socket certificate");
			
			wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_SOCKET_SETSOCKOPT, set_socket_option_result));
			
			return -1;
		}
	}
	SlSockAddrIn_t socket_address_in;
	socket_address_in.sin_family = SL_AF_INET;
	socket_address_in.sin_port = sl_Htons((uint16_t)server_port);

	socket_address_in.sin_addr.s_addr = sl_Htonl((uint32_t)ip_address);
	
	uint8_t retries = 0;
	int16_t connect_result = -1; 
	while((retries++ < 200) && ((connect_result = sl_Connect(socket_id, (SlSockAddr_t*)&socket_address_in, sizeof(SlSockAddrIn_t))) != 0))
	{
		if(connect_result == SL_EALREADY) // wait to be opened
		{
			//_delay_ms(10);
		}
		else if(connect_result < 0) // error
		{
			break;
		}
	}
	
	if(connect_result < 0)
	{
		LOG(1, "CC3100 unable to open TCP socket");
		
		wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_SOCKET_CONNECT, connect_result));
		
		sl_Close(socket_id);

		return -1;
	}
	
	return socket_id;
}

int wifi_open_udp_socket(char* address, uint16_t port)
{
	LOG(1, "CC3100 open UDP socket");
	
	SlSockAddrIn_t  Addr;
	SlSockAddrIn_t  LocalAddr;
	
	int16_t socket_id = sl_Socket(SL_AF_INET, SL_SOCK_DGRAM, 0);
	if(socket_id < 0)
	{
		LOG(1, "CC3100 unable to create UDP socket");
		
		wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_SOCKET_SOCKET, socket_id));
		
		return socket_id;
	}
	
	SlSockNonblocking_t socket_non_blocking;
	socket_non_blocking.NonblockingEnabled = 1;
	int16_t set_socket_option_result = sl_SetSockOpt(socket_id, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &socket_non_blocking, sizeof(SlSockNonblocking_t));
	if(set_socket_option_result < 0)	{		LOG(1, "CC3100 unable to set socket non-blocking mode");				wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_SOCKET_SETSOCKOPT, set_socket_option_result));				return -1;	}
	
	uint32_t address_hex = inet_aton(address);
	
	LocalAddr.sin_family = SL_AF_INET;
	LocalAddr.sin_port = sl_Htons((_u16)port);
	LocalAddr.sin_addr.s_addr = SL_INADDR_ANY;
	
	_u16 AddrSize = sizeof(SlSockAddrIn_t);
	
	if(sl_Bind(socket_id, (SlSockAddr_t *)&LocalAddr, AddrSize) < 0)
	{
		LOG(1, "Could not bind UDP socket");
		
		return -1;
	}
	
	return socket_id;
}

bool wifi_close_socket(int socket_id)
{
	LOG(1, "CC3100 close socket");	
	
	int16_t close_socket_result = sl_Close(socket_id);
	if(close_socket_result < 0)
	{
		wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_SOCKET_CLOSE, close_socket_result));
		
		return false;
	}
	
	return true;
}

int wifi_send(int socket, uint8_t* buffer, uint16_t length)
{
	LOG(1, "CC3100 send");	
	
	int16_t send_result = sl_Send(socket, buffer, length, 0);
	if(send_result < 0)
	{
		wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_SOCKET_SEND, send_result));
	}
	
	return send_result;
}

int wifi_send_to(int socket, uint8_t* buffer, uint16_t count, char* address, uint16_t port)
{
	LOG(1, "CC3100 send to");	
	
	SlSockAddrIn_t  Addr;
	_u16            AddrSize = 0;
	 
	uint32_t address_hex = inet_aton(address);
	
	LOG_PRINT(1, PSTR("ADDRESS HEX %lu\r\n"), address_hex);
	 
	Addr.sin_family = SL_AF_INET;
	Addr.sin_port = sl_Htons((_u16)port);
	Addr.sin_addr.s_addr = sl_Htonl((_u32)address_hex);
	
	AddrSize = sizeof(SlSockAddrIn_t);
	
	_i16 Status = sl_SendTo(socket, buffer, count, 0, (SlSockAddr_t *)&Addr, AddrSize);
	if( Status <= 0 )
	{
		LOG(1, "Unable to send data");
	}
	   
	return Status;
}

int wifi_receive(int socket, uint8_t* buffer, uint16_t length)
{	
	int16_t received = sl_Recv(socket, buffer, length, 0);
	
	if(received == SL_EAGAIN)
	{
		return 0;
	}
	
	if(received < 0)
	{
		wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_SOCKET_RECV, received));
	}

	return received;
}

int wifi_receive_from(int socket, uint8_t* buffer, uint16_t size, char* address, uint16_t* port)
{ 
	SlSockAddrIn_t  Addr;
	_u16 AddrSize = 0;
	
	AddrSize = sizeof(SlSockAddrIn_t);

	int16_t received = sl_RecvFrom(socket, buffer, size, 0, (SlSockAddr_t *)&Addr, (SlSocklen_t*)&AddrSize );
	
	if(received == SL_EAGAIN)
	{
		return 0;
	}
	
	if(received < 0)
	{
		wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_SOCKET_RECV, received));
	}
	
	LOG_PRINT(1, PSTR("RECEIVED RESULT %i\r\n"), received);
	 
	return received;
}

uint32_t wifi_get_current_ip(char* ip_address)
{
	uint8_t len = sizeof(SlNetCfgIpV4Args_t);
	uint8_t dhcpIsOn = 0;
	SlNetCfgIpV4Args_t ipV4 = {0};
		
	int16_t cfg_get_result = sl_NetCfgGet(SL_IPV4_STA_P2P_CL_GET_INFO, &dhcpIsOn, &len, (uint8_t *)&ipV4);
	if(cfg_get_result < 0)
	{
		wifi_platform_specific_error_code_listener(create_CC3100_error_code(SL_OPCODE_DEVICE_NETCFG_GET_COMMAND, cfg_get_result));
		
		return 0;
	}

	sprintf_P(ip_address, PSTR("%lu.%lu.%lu.%lu"), SL_IPV4_BYTE(ipV4.ipV4, 3), SL_IPV4_BYTE(ipV4.ipV4, 2), SL_IPV4_BYTE(ipV4.ipV4, 1), SL_IPV4_BYTE(ipV4.ipV4, 0));
	
	return ipV4.ipV4;
}

uint16_t serialize_wifi_platform_specific_error_code(uint32_t error_code, char* buffer)
{
	return sprintf_P(buffer, PSTR("%04X%04X"), MSW(error_code), LSW(error_code));
}

void add_wifi_connected_listener(void (*listener)(void))
{
	wifi_connected_listener = listener; 
}

void add_wifi_ip_address_acquired_listener(void (*listener)(void))
{
	wifi_ip_address_acquired_listener = listener; 
}

void add_wifi_disconnected_listener(void (*listener)(void))
{
	wifi_disconnected_listener = listener; 
}

void add_wifi_error_listener(void (*listener)(void))
{
	wifi_error_listener = listener; 
}

void add_wifi_platform_specific_error_code_listener(void (*listener)(uint32_t error_code))
{
	wifi_platform_specific_error_code_listener = listener;
}

void add_wifi_socket_closed_listener(void (*listener)(void))
{
	wifi_socket_closed_listener = listener;
}

void my_callback_os_CC3000_TCP_close_wait(unsigned char data)
{
	LOG(1, "Wifi socket closed callback called");
	
	wifi_socket_closed_listener();
}