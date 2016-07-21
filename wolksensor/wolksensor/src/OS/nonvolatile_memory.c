#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <avr/pgmspace.h>

// OS
#include "brd.h"
#include "nonvolatile_memory.h"

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
	switch(type)
	{
		//case CFG_MQTT_ID:
		//{
		//	ReadUserSignature(data, (void *)offsetof(NVM_USER_SIGNATURES_t, MQTTID), length);
		//	return (*(uint8_t *)data) != 0xff;
		//}
		//case CFG_MQTT_PRESHARED_KEY:
		//{
		//	ReadUserSignature(data, (void *)offsetof(NVM_USER_SIGNATURES_t, MQTTPSK), length);
		//	return (*(uint8_t *)data) != 0xff;
		//}
		default:
		{
			return config_read_eeprom(data, type, version, length);
		}
	}
}

bool config_write(void *data, uint8_t type, uint8_t version, uint8_t length)
{
	switch(type)
	{
		//case CFG_MQTT_ID:
		//{
		//	BootloaderAPI_WriteUserSignature(offsetof(NVM_USER_SIGNATURES_t, MQTTID), data, length);
		//	return true;
		//}
		//case CFG_MQTT_PRESHARED_KEY:
		//{
		//	BootloaderAPI_WriteUserSignature(offsetof(NVM_USER_SIGNATURES_t, MQTTPSK), data, length);
		//	return true;
		//}
		default:
		{
			return config_write_eeprom(data, type, version, length);
		}
	}
}

