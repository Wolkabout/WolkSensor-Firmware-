#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "brd.h"


// static reset_t reasonFlag __attribute__ ((section (".noinit")));



/*
static inline void nvm_wait_until_ready( void ) {
	do {} while ((NVM.STATUS & NVM_NVMBUSY_bm) == NVM_NVMBUSY_bm);
}


uint8_t ReadUserSignatureByte(uint16_t Address) {
	nvm_wait_until_ready();
	NVM_CMD = NVM_CMD_READ_USER_SIG_ROW_gc;
	uint8_t Result;
	__asm__ ("lpm %0, Z\n" : "=r" (Result) : "z" (Address));
	NVM_CMD = NVM_CMD_NO_OPERATION_gc;
	return Result;
}

void ReadUserSignature(void *dst, void *src, size_t size) {
	uint8_t *dest = (uint8_t*)dst;
	uint8_t *source = (uint8_t*)src;
	while (size) {
		*(dest++) = ReadUserSignatureByte((uint16_t)source++);
		size--;
	}
}

uint8_t ReadSignatureByte(uint16_t Address) {
	nvm_wait_until_ready();
	NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
	uint8_t Result;
	__asm__ ("lpm %0, Z\n" : "=r" (Result) : "z" (Address));
	NVM_CMD = NVM_CMD_NO_OPERATION_gc;
	return Result;
}



static inline void eeprom_enable_mapping(void) {
	NVM_CTRLB = NVM_CTRLB | NVM_EEMAPEN_bm;
}

static inline void eeprom_disable_mapping(void) {
	NVM_CTRLB = NVM_CTRLB & ~NVM_EEMAPEN_bm;
}

static inline void nvm_issue_command(NVM_CMD_t nvm_command) {
	uint8_t old_cmd;

	old_cmd = NVM.CMD;
	NVM.CMD = nvm_command;
	ccp_write_IO((uint8_t *)&NVM.CTRLA, NVM_CMDEX_bm);
	NVM.CMD = old_cmd;
}


void nvm_eeprom_update_buffer(uint16_t address, const uint8_t *buf, uint16_t len) {
	for (uint16_t i=0; i<len; i += EEPROM_PAGE_SIZE) {
		nvm_wait_until_ready();
		eeprom_enable_mapping();
		int res = memcmp(buf + i, (uint8_t *)(address + i + MAPPED_EEPROM_START), EEPROM_PAGE_SIZE);
		eeprom_disable_mapping();
		if (res) {
			eeprom_enable_mapping();
			for (uint8_t j = 0; j < EEPROM_PAGE_SIZE; j++) {
				*(uint8_t*)(j + MAPPED_EEPROM_START) = *(buf+i+j);
			}
			eeprom_disable_mapping();
			NVM.ADDR2 = 0x00;
			NVM.ADDR1 = (address >> 8) & 0xFF;
			NVM.ADDR0 = address & 0xFF;
			nvm_issue_command(NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc);
		}
		address += EEPROM_PAGE_SIZE;
	}
}


void nvm_eeprom_readall_buffer(uint16_t address, void *buf, uint16_t len) {
	nvm_wait_until_ready();
	eeprom_enable_mapping();
	memcpy(buf, (void*)(address + MAPPED_EEPROM_START), len);
	eeprom_disable_mapping();
}


void    (*BootloaderAPI_ErasePage)(uint32_t Address)               = BOOTLOADER_API_CALL(0);
void    (*BootloaderAPI_WritePage)(uint32_t Address)               = BOOTLOADER_API_CALL(1);
void    (*BootloaderAPI_FillWord)(uint32_t Address, uint16_t Word) = BOOTLOADER_API_CALL(2);
uint8_t (*BootloaderAPI_ReadSignature)(uint16_t Address)           = BOOTLOADER_API_CALL(3);
uint8_t (*BootloaderAPI_ReadFuse)(uint16_t Address)                = BOOTLOADER_API_CALL(4);
uint8_t (*BootloaderAPI_ReadLock)(void)                            = BOOTLOADER_API_CALL(5);
void    (*BootloaderAPI_WriteLock)(uint8_t LockBits)               = BOOTLOADER_API_CALL(6);

*/
