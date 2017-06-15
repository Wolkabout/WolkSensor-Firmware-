#include <stdbool.h>
#include <util/crc16.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "string.h"
#include "sp_driver.h"
#include "BootloaderAPI.h"
#include "Bootloader.h"
#include "brd.h"

#define pageSize 512
//const char fusedata[] __attribute__ ((section (".fuse"))) = { 0xff, 0xff, 0xbd, 0xFF, 0xF7, 0xd7 };// 1V6 BROWNOUT LEVEL

const char fusedata[] __attribute__ ((section (".fuse"))) = { 0xff, 0xff, 0xbd, 0xFF, 0xF7, 0xd5 };// 2V0 BROWNOUT LEVEL
	
//const char fusedata[] __attribute__ ((section (".fuse"))) = { 0xff, 0xff, 0xbd, 0xFF, 0xF7, 0xd1 };// 2V8 BROWNOUT LEVEL
const char eeprom[] __attribute__ ((section (".eeprom"))) = { [0 ... (EEPROM_SIZE - 1)] = 0xff };
const uint8_t signature[] __attribute__ ((section (".user_signatures"))) = { [0 ... (USER_SIGNATURES_SIZE - 1)] = 0xff };

bool program_loaded;
uint32_t current_adress;
bool count_timeout_start_program;


/* System functions ******************************************************************************/
static inline void wdt_wait_while_busy(void)
{
	while ((WDT.STATUS & WDT_SYNCBUSY_bm) == WDT_SYNCBUSY_bm)
	{		/* Wait until synchronization */
	}
}


void watchdog_enable(uint8_t period)
{
	uint8_t temp = (WDT_PER_gm & period) | (1 << WDT_ENABLE_bp) | (1 << WDT_CEN_bp);
	ccp_write_IO((void *)&WDT.CTRL, temp);
	wdt_wait_while_busy();
}

void watchdog_disable(void) {
	uint8_t temp = (WDT.CTRL & ~WDT_ENABLE_bm) | (1 << WDT_CEN_bp);
	ccp_write_IO((void *)&WDT.CTRL, temp);
	wdt_wait_while_busy();
}

#define watchdog_reset() __asm__ __volatile__ ("wdr")


void ccp_write_IO( volatile uint8_t * address, uint8_t value )
{
	register8_t saved_sreg = SREG;
	cli();
	volatile uint8_t * tmpAddr = address;
#ifdef RAMPZ
	RAMPZ = 0;
#endif
	asm volatile(
	"movw r30,  %0"       "\n\t"
	"ldi  r16,  %2"       "\n\t"
	"out   %3, r16"       "\n\t"
	"st     Z,  %1"       "\n\t"
	:
	: "r" (tmpAddr), "r" (value), "M" (CCP_IOREG_gc), "i" (&CCP)
	: "r16", "r30", "r31"
	);
	SREG = saved_sreg;
}


/* Init ******************************************************************************************/
void init(void) 
{
	register8_t saved_sreg = SREG;
	
	/* LED-s (not used) */
	PORTF.DIR = 0b00000011;
	PORTF.PIN0CTRL = 0b00000000;		// output, RED
	PORTF.PIN1CTRL = 0b00000000;		// output, GREEN
	PORTF.OUTCLR = 0x03;				// turn ON red and green led

	/* Enable internal 2MHz clock, wait until ready */
	OSC.CTRL |= OSC_RC2MEN_bm;
	while (!(OSC.STATUS & OSC_RC2MRDY_bm)) {};

	/* Enable PLL0 (driven by 2MHz internal calibrated clock), wait until ready */
	if (!(OSC.STATUS & OSC_PLLRDY_bm))
	{
		OSC.PLLCTRL = OSC_PLLSRC_RC2M_gc | (12 << OSC_PLLFAC_gp);
		OSC.CTRL |= OSC_PLLEN_bm;
		while (!(OSC.STATUS & OSC_PLLRDY_bm));
	}

	/* System clock is 24MHz, using PLL0, driven by 2MHz internal calibrated clock */
	ccp_write_IO((uint8_t *) & CLK.CTRL, CLK_SCLKSEL_PLL_gc);
	
	/* UART pins */
	PORTD.OUTSET = PIN3_bm;		// Set TXD high
	PORTD.DIRSET = PIN3_bm;		// PC3 (TXD0) as output.
	PORTD.DIRCLR = PIN2_bm;		// PC2 (RXD0) as input.
	
	/* Enable power for USART0 on port D */
	PR.PRPD &= ~PR_USART0_bm;
	
	/* Baud rate 115200 */
	USARTD0.BAUDCTRLA = 0x19;
	USARTD0.BAUDCTRLB = 0x00;
	
	USARTD0.CTRLA = 0;			// Disable interrupts
	USARTD0.CTRLC = (USARTD0.CTRLC & ~USART_CHSIZE_gm) |  USART_CHSIZE_8BIT_gc;
	USARTD0.CTRLB = (USART_TXEN_bm | USART_CLK2X_bm | USART_RXEN_bm);
	
	SREG = saved_sreg;
}


/* Firmware status flags *************************************************************************/
void checkFirstStart()
{
	if (eeprom_read_byte((uint8_t*)0x0FFF) == 0xFF)
	{
		writeProgramFinishDownloadFlag();
		asm("jmp 0;");
	}
}


bool programDoesntExistFlag()
{
	return eeprom_read_byte((uint8_t*)0x0FFF) == 0xFF;
}


bool programExistsFlag()
{
	return eeprom_read_byte((uint8_t*)0x0FFF) == 0;
}


bool firmwareUpdateErrorFlag()
{
	return eeprom_read_byte((uint8_t*)0x0FFF) == 0x55;
}


void writeProgramStartDownloadFlag()
{
	eeprom_write_byte((uint8_t*)0x0FFF, 0x55);
}


void writeProgramFinishDownloadFlag()
{
	eeprom_write_byte((uint8_t*)0x0FFF, 0);
}

void sendStatus()
{
	if (programExistsFlag())
	{
		sendString("STATUS BOOT_MODE;");
	}
	else if (firmwareUpdateErrorFlag())
	{
		sendString("STATUS FIRMWARE_ERROR;");
	}	
}

/* Checksum **************************************************************************************/
bool checkSum(uint8_t* data)
{
	int i;
	uint8_t cs = 0;
	
	for (i=0; i<pageSize+1; i++)
	{
		cs += data[i];
	}
	return (cs == 0);
	
}


/* Send string ***********************************************************************************/
void sendString(char* str)
{
	while ((*str))
	{
		while (!(USARTD0.STATUS & USART_DREIF_bm));
		
		USARTD0.DATA = *str;
		str++;
	}
}


/* Decode command ********************************************************************************/
void decodeCommand(uint8_t* str)
{
	if (strcmp((char*)str, "RELOAD") == 0)
	{
		sendString("DONE;");
		for(;;);	// Reset using watchdog
	}
	
	else if (strcmp((char*)str, "END_BOOT") == 0)
	{
		writeProgramFinishDownloadFlag();
		sendString("DONE;");
		while (!(USARTD0.STATUS & USART_DREIF_bm));
		watchdog_disable();
		asm("jmp 0;");
	}
	
	else if (strcmp((char*)str, "STATUS") == 0)
	{
		sendStatus();
	}
	
	else if (strcmp((char*)str, "START_BOOT") == 0)
	{
		count_timeout_start_program = false;
		program_loaded = false;
		current_adress = 0;
		writeProgramStartDownloadFlag();
		sendString("DONE;");
	}
	else
	{
		sendString("BAD_REQUEST;");
		sendStatus();
	}
}


/* Decode program ********************************************************************************/
void decodeProgram(uint8_t* str)
{
	uint8_t dataReadFromMemory[pageSize];
	int i;	
		
	BootloaderAPI_WriteData(current_adress, str, pageSize);
	SP_ReadFlashPage(dataReadFromMemory, current_adress);
	for(i=0;i<pageSize;i++)
	{
		if(dataReadFromMemory[i]!=str[i])
		{
			sendString("BAD_REQUEST;");
			sendStatus();
			return;
		}
		
	}
	
	current_adress += pageSize;
	sendString("DONE;");
}


/* Main ******************************************************************************************/
int main(void)
{
	uint8_t buffer[pageSize + 10];
	uint8_t temp_data;
	int number_received;
	long counter_timeout_communication;
	long counter_timeout_start_program;
	
	bool count_timeout_communication;
	
	/* if first boot, just start program */
	
	/*
	
	Conclusion about checkFirstStart() function:
	
	* Solution Configuration: Release
	
	if this function is commented out, it`s not possible to programme
	device firmware by Device Programming tool under Atmel Device Studio.
	No problem programming by DFU (BLesser).
	After that it is possible to programme device firmware
	by Device Programming tool under Atmel Device Studio.
	
	Otherwise if this function is NOT commented
	out, bootloader.elf is the main Production
	file that programme both bootloader and firmware.
	
	*/
	checkFirstStart();	/* 0x0FFF = 0 */
	
	number_received = 0;
	program_loaded = programExistsFlag();
	
	counter_timeout_communication = 0;
	counter_timeout_start_program = 0;
	count_timeout_start_program = program_loaded;
	count_timeout_communication = false;
	
	current_adress = 0;
	
	watchdog_enable(WDT_WPER_1KCLK_gc);
	
	init();
	
	sendStatus();
		
/*		int flag = 0;*/
		
		for (;;)
		{
			if (USARTD0.STATUS & USART_RXCIF_bm) /* New byte received */
			{
				count_timeout_communication = true;
				temp_data = USARTD0.DATA;
				buffer[number_received] = temp_data;
				number_received++;
				
				if (temp_data == ';')	/* command; or PROGRAM 512cs; */
				{
					if (strncmp((char*)buffer, "PROGRAM", 7) == 0)
					{
						if (number_received == pageSize + 10)
						{
							if (checkSum(buffer+8))
							{
								counter_timeout_communication = 0;
								count_timeout_communication = false;
								buffer[number_received - 1] = 0;
								decodeProgram(buffer + 8);
								number_received = 0;
							}
							else
							{	// error in data transfer from host computer to device (checksum failed)
								counter_timeout_communication = 0;
								count_timeout_communication = false;
								number_received = 0;
								sendString("BAD_REQUEST;");
								sendStatus();
							}
						}
					}
					else
					{
						counter_timeout_communication = 0;
						count_timeout_communication = false;
						buffer[number_received - 1] = 0;
						number_received = 0;
						decodeCommand(buffer);
					}
				}
			}
			
			if (counter_timeout_start_program > 1000000)
			{
				count_timeout_start_program = false;
				
				if (program_loaded)
				{
					watchdog_disable();
					asm("jmp 0;");
				}
			}
			
			if (counter_timeout_communication > 500000)
			{
				sendString("BAD_REQUEST;");
				count_timeout_communication = false;
				counter_timeout_communication = 0;
				number_received = 0;
				sendStatus();
			}
			
			if (count_timeout_communication)
			{
				counter_timeout_communication++;
			}
			else
			{
				counter_timeout_communication = 0;
			}
			
			if (count_timeout_start_program)
			{
				counter_timeout_start_program++;
			}
			else
			{
				counter_timeout_start_program = 0;
			}
			
			watchdog_reset();
		}
	}
	