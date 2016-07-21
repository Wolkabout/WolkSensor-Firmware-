#ifndef BOARD_H_
#define BOARD_H_

#include "platform_specific.h"

#define  MSB(u16)          (((uint8_t* )&u16)[1])
#define  LSB(u16)          (((uint8_t* )&u16)[0])

#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)

typedef enum {
	RST_POWER_ON = 1,
	RST_PROGRAMMING_DEBUG = 2,
	RST_EXTERNAL = 3,
	RST_BROWN_OUT = 4,
	RST_WATCHDOG = 5,
	RST_CC3000_TX_ERROR = 10,
	RST_CC3000_RX_ERROR = 11,
	RST_RELOAD = 20,
} reset_reason_t;

#define USB_ON_STATE		((PORTE.IN & 0b00000100)?1:0)		// USB connected
#define CHG_STATE		0

void brd_init(void);
void reset(reset_reason_t reason);
void ccp_write_IO( volatile uint8_t * address, uint8_t value );
uint8_t ReadUserSignatureByte(uint16_t Address);
void ReadUserSignature(void *dst, void *src, size_t size);
uint8_t ReadSignatureByte(uint16_t Address);

void system_reset(void);

extern void BootloaderAPI_WriteData(uint32_t Address, uint8_t *buffer, uint16_t length);
extern void BootloaderAPI_WriteUserSignature(uint16_t Address, uint8_t *data, uint16_t length);

extern reset_reason_t reset_reason;

#define watchdog_reset() __asm__ __volatile__ ("wdr")
void watchdog_enable(uint8_t period);
void watchdog_disable(void);

#endif /* BOARD_H_ */
