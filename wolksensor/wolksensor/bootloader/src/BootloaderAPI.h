#ifndef _BOOTLOADER_API_H_
#define _BOOTLOADER_API_H_

void    BootloaderAPI_ErasePage(const uint32_t Address);
void    BootloaderAPI_WritePage(const uint32_t Address);
void    BootloaderAPI_FillWord(const uint32_t Address, const uint16_t Word);
uint8_t BootloaderAPI_ReadSignature(const uint16_t Address);
uint8_t BootloaderAPI_ReadFuse(const uint16_t Address);
uint8_t BootloaderAPI_ReadLock(void);
void    BootloaderAPI_WriteLock(const uint8_t LockBits);
void	BootloaderAPI_WriteData(uint32_t Address, uint8_t *buffer, uint16_t length);
void	BootloaderAPI_WriteUserSignature(uint16_t address, uint8_t *data, uint16_t length);


#endif

