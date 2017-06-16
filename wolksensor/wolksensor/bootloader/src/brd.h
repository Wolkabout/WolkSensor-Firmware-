#ifndef BOARD_H_
#define BOARD_H_

#define  MSB(u16)          (((uint8_t* )&u16)[1])
#define  LSB(u16)          (((uint8_t* )&u16)[0])

#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)

typedef enum {
	RST_COLD_BOOT = 1,
	RST_BROWN_OUT = 2,
	RST_WATCHDOG = 3,
	RST_CC3000_VOLTAGE = 4,
	RST_CC3000_WAIT_ERROR = 5,
	RST_CC3000_TX_ERROR = 6, 
	RST_CC3000_RX_ERROR = 7,
	RST_CC3000_PATCH = 8,
	RST_STORAGE = 9,
	RST_DEFAULT_CFG = 10,
} reset_t;


#define PGOOD_STATE		((PORTD.IN & 0b00100000)?0:1)
#define CHG_STATE		((PORTD.IN & 0b00010000)?0:1)
#define KEY_STATE		((PORTF.IN & 0b00000100)?0:1)

void brd_init(void);

void ccp_write_IO( volatile uint8_t * address, uint8_t value );
void reset(void);

/*
uint8_t ReadUserSignatureByte(uint16_t Address);
void ReadUserSignature(void *dst, void *src, size_t size);
uint8_t ReadSignatureByte(uint16_t Address);

reset_t resetReason(void);

void nvm_eeprom_readall_buffer(uint16_t address, void *buf, uint16_t len);
void nvm_eeprom_update_buffer(uint16_t address, const uint8_t *buf, uint16_t len);
*/

#endif /* BOARD_H_ */
