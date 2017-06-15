#include <avr/io.h>

#include <stdbool.h>

#include "sp_driver.h"
#include "BootloaderAPI.h"


void BootloaderAPI_ErasePage(const uint32_t Address) {
	SP_EraseApplicationPage(Address);
	SP_WaitForSPM();
}

void BootloaderAPI_WritePage(const uint32_t Address) {
	SP_WriteApplicationPage(Address);
	SP_WaitForSPM();
}

void BootloaderAPI_FillWord(const uint32_t Address, const uint16_t Word) {
	SP_LoadFlashWord(Address, Word);
}

uint8_t BootloaderAPI_ReadSignature(const uint16_t Address) {
	return SP_ReadUserSignatureByte(Address);
}

uint8_t BootloaderAPI_ReadFuse(const uint16_t Address) {
	return SP_ReadFuseByte(Address);
}

uint8_t BootloaderAPI_ReadLock(void) {
	return SP_ReadLockBits();
}

void BootloaderAPI_WriteLock(const uint8_t LockBits) {
	SP_WriteLockBits(LockBits);
}


void BootloaderAPI_WriteData(uint32_t address, uint8_t *buffer, uint16_t length) {
	uint8_t page[APP_SECTION_PAGE_SIZE];
	SP_ReadFlashPage(page, address & ~(APP_SECTION_PAGE_SIZE - 1));
	uint16_t i=0;
	uint16_t add = address & (APP_SECTION_PAGE_SIZE - 1);
	for (; (add < APP_SECTION_PAGE_SIZE) && length; add++, length--, i++) {
		page[add] = buffer[i];
	}
	SP_LoadFlashPage(page);
	SP_EraseWriteApplicationPage(address);
	SP_WaitForSPM();
}


void BootloaderAPI_WriteUserSignature(uint16_t address, uint8_t *data, uint16_t length) {
	uint8_t page[USER_SIGNATURES_PAGE_SIZE];
	for (int i=0; i<USER_SIGNATURES_PAGE_SIZE; i++) {
		page[i] = SP_ReadUserSignatureByte(i);
	}
	for (int i=0; i<length; i++) {
		page[address+i] = data[i];
	}
	SP_EraseUserSignatureRow();
	SP_WaitForSPM();
	SP_LoadFlashPage(page);
	SP_WaitForSPM();
	SP_WriteUserSignatureRow();
	SP_WaitForSPM();
}
	