#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <avr/pgmspace.h>

// OS
#include "brd.h"
#include "nonvolatile_memory.h"
#include "logger.h"
#include "config.h"

#define CFG_EMPTY	255

#define EEPROM_CHUNK_SIZE EEPROM_PAGE_SIZE - 2

static inline void eeprom_enable_mapping(void) {
	NVM_CTRLB = NVM_CTRLB | NVM_EEMAPEN_bm;
}

static inline void eeprom_disable_mapping(void) {
	NVM_CTRLB = NVM_CTRLB & ~NVM_EEMAPEN_bm;
}

static inline void nvm_wait_until_ready( void ) {
	do {} while ((NVM.STATUS & NVM_NVMBUSY_bm) == NVM_NVMBUSY_bm);
}

static inline void nvm_issue_command(NVM_CMD_t nvm_command) {
	uint8_t old_cmd;

	old_cmd = NVM.CMD;
	NVM.CMD = nvm_command;
	ccp_write_IO((uint8_t *)&NVM.CTRLA, NVM_CMDEX_bm);
	NVM.CMD = old_cmd;
}

// data is stored in chunks of 32 bytes, first byte is the field type and second is version of the field
static bool config_read_eeprom(void *data, uint8_t type, uint8_t version, uint8_t length) {
	if (length > (EEPROM_PAGE_SIZE - 2))
		return false;
	nvm_wait_until_ready();
	eeprom_enable_mapping();
	uint16_t addr = 0;
	// go through all fields in eeprom until we find one with the matching type and version
	for (; addr < (EEPROM_SIZE-EEPROM_PAGE_SIZE); addr += EEPROM_PAGE_SIZE) {
		if ((*(uint8_t *)(addr + MAPPED_EEPROM_START) == type) && 
		    (*(uint8_t *)(addr + MAPPED_EEPROM_START + 1) == version)) {
			memcpy(data, (void*)(addr + MAPPED_EEPROM_START + 2), length);
			break;
		}
	}
	eeprom_disable_mapping();
	if (addr == (EEPROM_SIZE-EEPROM_PAGE_SIZE))
		return false;
	return true;
}


bool config_write_eeprom(void *data, uint8_t type, uint8_t version, uint8_t length) {
	if (length > (EEPROM_PAGE_SIZE - 2))
		return false;
	nvm_wait_until_ready();
	eeprom_enable_mapping();
	int empty = -1;
	uint16_t addr = 0;
	for (; addr < (EEPROM_SIZE-EEPROM_PAGE_SIZE); addr += EEPROM_PAGE_SIZE) {
		if (*(uint8_t *)(addr + MAPPED_EEPROM_START) == CFG_EMPTY) {
			if (empty == -1)
				empty = addr;
		} else if (*(uint8_t *)(addr + MAPPED_EEPROM_START) == type) {
			break;
		}
	}
	if (addr == (EEPROM_SIZE-EEPROM_PAGE_SIZE)) {
		if (empty == -1) {
			return false;
		} else {
			addr = empty;
		}
	}
	*(uint8_t *)(MAPPED_EEPROM_START) = type;
	*(uint8_t *)(MAPPED_EEPROM_START + 1) = version;
	memcpy((void *)(MAPPED_EEPROM_START + 2), data, length);
	eeprom_disable_mapping();
	NVM.ADDR2 = 0x00;
	NVM.ADDR1 = (addr >> 8) & 0xFF;
	NVM.ADDR0 = addr & 0xFF;
	nvm_issue_command(NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc);

	return true;
}


bool config_read(void *data, uint8_t type, uint8_t version, uint8_t length)
{
	char tmp_string[EEPROM_PAGE_SIZE];

	switch(type)
	{
		case CFG_WIFI_SSID:
		{
			if(!config_read_eeprom(tmp_string, type, version, (EEPROM_PAGE_SIZE - 2)))
				return false;
			LOG_PRINT(1, PSTR("Read first page from EEPROM: %s | len: %d\n\r"), tmp_string, strlen(tmp_string));

			memset(data, 0, sizeof(data));
			strncpy(data, tmp_string, (EEPROM_PAGE_SIZE - 2));
			memset(tmp_string, 0, sizeof(tmp_string));

			if(!config_read_eeprom(tmp_string, CFG_WIFI_SSID_SEC, version, (EEPROM_PAGE_SIZE - 2)))
			{
				LOG(1, "Unable to read from CFG_WIFI_SSID_SEC");
				//return false;
			}
			LOG_PRINT(1, PSTR("Read second page from EEPROM: %s | len: %d\n\r"), tmp_string, strlen(tmp_string));

			strcat(data, tmp_string);
			return true;
		}
		case CFG_WIFI_PASS:
		{
			memset(data, 0, sizeof(data));
			if(!config_read_eeprom(tmp_string, type, version, (EEPROM_PAGE_SIZE - 2)))
				return false;
			LOG_PRINT(1, PSTR("Read first page from EEPROM: %s | len: %d\n\r"), tmp_string, strlen(tmp_string));
			strncpy(data, tmp_string, (EEPROM_PAGE_SIZE - 2));

			memset(tmp_string, 0, sizeof(tmp_string));
			if(!config_read_eeprom(tmp_string, CFG_WIFI_PASS_SEC, version, (EEPROM_PAGE_SIZE - 2)))
			{
				LOG(1, "Unable to read from CFG_WIFI_PASS_SEC");
				//return false;
			}
			LOG_PRINT(1, PSTR("Read second page from EEPROM: %s | len: %d\n\r"), tmp_string, strlen(tmp_string));
			strcat(data, tmp_string);

			memset(tmp_string, 0, sizeof(tmp_string));
			if(!config_read_eeprom(tmp_string, CFG_WIFI_PASS_THIRD, version, 3))
			{
				LOG(1, "Unable to read from CFG_WIFI_PASS_THIRD");
				//return false;
			}
			LOG_PRINT(1, PSTR("Read third page from EEPROM: %s | len: %d\n\r"), tmp_string, strlen(tmp_string));

			strcat(data, tmp_string);
			return true;
		}
		default:
		{
			return config_read_eeprom(data, type, version, length);
		}
	}
}

bool config_write(void *data, uint8_t type, uint8_t version, uint8_t length)
{
	char tmp_string[EEPROM_PAGE_SIZE];

	switch(type)
	{
		case CFG_WIFI_SSID:
		{
			if(length > (EEPROM_PAGE_SIZE - 2))
			{
				strncpy(tmp_string, data, (EEPROM_PAGE_SIZE - 2));
				if (!config_write_eeprom(tmp_string, type, version, (EEPROM_PAGE_SIZE - 2)))
					return false;

				data += (EEPROM_PAGE_SIZE - 2);
				memset(tmp_string, 0, sizeof(tmp_string));
				strncpy(tmp_string, data, (length - (EEPROM_PAGE_SIZE - 2)));
				return config_write_eeprom(tmp_string, CFG_WIFI_SSID_SEC, version, (length - (EEPROM_PAGE_SIZE - 2)));
			}
			else
			{
				return config_write_eeprom(data, type, version, length);
			}
		}
		case CFG_WIFI_PASS:
		{
			if(length > (EEPROM_PAGE_SIZE - 2))
			{
				memset(tmp_string, 0, sizeof(tmp_string));
				strncpy(tmp_string, data, (EEPROM_PAGE_SIZE - 2));
				if (!config_write_eeprom(tmp_string, type, version, (EEPROM_PAGE_SIZE - 2)))
					return false;

				data += (EEPROM_PAGE_SIZE - 2);
				memset(tmp_string, 0, sizeof(tmp_string));
				strncpy(tmp_string, data, (EEPROM_PAGE_SIZE - 2));
				if (!config_write_eeprom(tmp_string, CFG_WIFI_PASS_SEC, version, (EEPROM_PAGE_SIZE - 2)))
					return false;

				data += (EEPROM_PAGE_SIZE - 2);
				memset(tmp_string, 0, sizeof(tmp_string));
				strncpy(tmp_string, data, (length - 2*(EEPROM_PAGE_SIZE - 2)));
				return config_write_eeprom(tmp_string, CFG_WIFI_PASS_THIRD, version, (length - 2*(EEPROM_PAGE_SIZE - 2)));
			}
			else
			{
				return config_write_eeprom(data, type, version, length);
			}
		}
		default:
		{
			return config_write_eeprom(data, type, version, length);
		}
	}
}

