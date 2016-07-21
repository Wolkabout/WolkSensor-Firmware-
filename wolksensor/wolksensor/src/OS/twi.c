#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stddef.h>

#include "clock.h"
#include "twi.h"
#include "test.h"


void TWI_MasterInit(TWI_Master_t *twi,
                    TWI_t *module,
                    TWI_MASTER_INTLVL_t intLevel,
                    uint8_t baudRateRegisterSetting)
{
	twi->interface = module;
	twi->interface->MASTER.CTRLA = intLevel |
                                   TWI_MASTER_RIEN_bm |
                                   TWI_MASTER_WIEN_bm |
                                   TWI_MASTER_ENABLE_bm;
	twi->interface->MASTER.BAUD = baudRateRegisterSetting;//	
	twi->interface->MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc;
}


TWI_MASTER_BUSSTATE_t TWI_MasterState(TWI_Master_t *twi) {
	TWI_MASTER_BUSSTATE_t twi_status;
	twi_status = (TWI_MASTER_BUSSTATE_t) (twi->interface->MASTER.STATUS &
	                                      TWI_MASTER_BUSSTATE_gm);
	return twi_status;
}


bool TWI_MasterReady(TWI_Master_t *twi) {
	bool twi_status = (twi->status & TWIM_STATUS_READY);
	return twi_status;
}


bool TWI_MasterWrite(TWI_Master_t *twi, uint8_t address, uint8_t *writeData, uint8_t bytesToWrite) {
	bool twi_status = TWI_MasterWriteRead(twi, address, writeData, bytesToWrite, 0);
	return twi_status;
}


bool TWI_MasterRead(TWI_Master_t *twi, uint8_t address, uint8_t bytesToRead) {
	bool twi_status = TWI_MasterWriteRead(twi, address, 0, 0, bytesToRead);
	return twi_status;
}


bool TWI_MasterWriteRead(TWI_Master_t *twi, uint8_t address, uint8_t *writeData, uint8_t bytesToWrite, uint8_t bytesToRead) {
	/*Parameter sanity check. */
	if (bytesToWrite > TWIM_WRITE_BUFFER_SIZE) {
		return false;
	}
	if (bytesToRead > TWIM_READ_BUFFER_SIZE) {
		return false;
	}

	/*Initiate transaction if bus is ready. */
	if (twi->status == TWIM_STATUS_READY) {

		twi->status = TWIM_STATUS_BUSY;
		twi->result = TWIM_RESULT_UNKNOWN;

		twi->address = address<<1;

		/* Fill write data buffer. */
		for (uint8_t bufferIndex=0; bufferIndex < bytesToWrite; bufferIndex++) {
			twi->writeData[bufferIndex] = writeData[bufferIndex];
		}

		twi->bytesToWrite = bytesToWrite;
		twi->bytesToRead = bytesToRead;
		twi->bytesWritten = 0;
		twi->bytesRead = 0;

		/* If write command, send the START condition + Address + 'R/_W = 0' */
		if (twi->bytesToWrite > 0) {
			uint8_t writeAddress = twi->address & ~0x01;
			twi->interface->MASTER.ADDR = writeAddress;
		}

		/* If read command, send the START condition + Address + 'R/_W = 1' */
		else if (twi->bytesToRead > 0) {
			uint8_t readAddress = twi->address | 0x01;
			twi->interface->MASTER.ADDR = readAddress;
		}
		return true;
	} else {
		return false;
	}
}


void TWI_MasterInterruptHandler(TWI_Master_t *twi) {
	uint8_t currentStatus = twi->interface->MASTER.STATUS;
	/* If arbitration lost or bus error. */
	if ((currentStatus & TWI_MASTER_ARBLOST_bm) ||
	    (currentStatus & TWI_MASTER_BUSERR_bm)) {

		TWI_MasterArbitrationLostBusErrorHandler(twi);
	}

	/* If master write interrupt. */
	else if (currentStatus & TWI_MASTER_WIF_bm) {
		TWI_MasterWriteHandler(twi);
	}

	/* If master read interrupt. */
	else if (currentStatus & TWI_MASTER_RIF_bm) {
		TWI_MasterReadHandler(twi);
	}

	/* If unexpected state. */
	else {
		TWI_MasterTransactionFinished(twi, TWIM_RESULT_FAIL);
	}
}


void TWI_MasterArbitrationLostBusErrorHandler(TWI_Master_t *twi) {
	uint8_t currentStatus = twi->interface->MASTER.STATUS;

	/* If bus error. */
	if (currentStatus & TWI_MASTER_BUSERR_bm) {
		twi->result = TWIM_RESULT_BUS_ERROR;
	}
	/* If arbitration lost. */
	else {
		twi->result = TWIM_RESULT_ARBITRATION_LOST;
	}

	/* Clear interrupt flag. */
	twi->interface->MASTER.STATUS = currentStatus | TWI_MASTER_ARBLOST_bm;

	twi->status = TWIM_STATUS_READY;
}


void TWI_MasterWriteHandler(TWI_Master_t *twi) {
	/* Local variables used in if tests to avoid compiler warning. */
	uint8_t bytesToWrite  = twi->bytesToWrite;
	uint8_t bytesToRead   = twi->bytesToRead;
	/* If NOT acknowledged (NACK) by slave cancel the transaction. */
	if (twi->interface->MASTER.STATUS & TWI_MASTER_RXACK_bm) {
		twi->interface->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
		twi->result = TWIM_RESULT_NACK_RECEIVED;
		twi->status = TWIM_STATUS_READY;
	}

	/* If more bytes to write, send data. */
	else if (twi->bytesWritten < bytesToWrite) {
		uint8_t data = twi->writeData[twi->bytesWritten];
		twi->interface->MASTER.DATA = data;
		++twi->bytesWritten;
	}

	/* If bytes to read, send repeated START condition + Address + 'R/_W = 1' */
	else if (twi->bytesRead < bytesToRead) {
		uint8_t readAddress = twi->address | 0x01;
		twi->interface->MASTER.ADDR = readAddress;
	}

	/* If transaction finished, send STOP condition and set RESULT OK. */
	else {
		twi->interface->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
		TWI_MasterTransactionFinished(twi, TWIM_RESULT_OK);
	}
}


void TWI_MasterReadHandler(TWI_Master_t *twi) {
	/* Fetch data if bytes to be read. */
	if (twi->bytesRead < TWIM_READ_BUFFER_SIZE) {
		uint8_t data = twi->interface->MASTER.DATA;
		twi->readData[twi->bytesRead] = data;
		twi->bytesRead++;
	}

	/* If buffer overflow, issue STOP and BUFFER_OVERFLOW condition. */
	else {
		twi->interface->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
		TWI_MasterTransactionFinished(twi, TWIM_RESULT_BUFFER_OVERFLOW);
	}

	/* Local variable used in if test to avoid compiler warning. */
	uint8_t bytesToRead = twi->bytesToRead;

	/* If more bytes to read, issue ACK and start a byte read. */
	if (twi->bytesRead < bytesToRead) {
		twi->interface->MASTER.CTRLC = TWI_MASTER_CMD_RECVTRANS_gc;
	}

	/* If transaction finished, issue NACK and STOP condition. */
	else {
		twi->interface->MASTER.CTRLC = TWI_MASTER_ACKACT_bm |
		                               TWI_MASTER_CMD_STOP_gc;
		TWI_MasterTransactionFinished(twi, TWIM_RESULT_OK);
	}
}


void TWI_MasterTransactionFinished(TWI_Master_t *twi, uint8_t result) {
	twi->result = result;
	twi->status = TWIM_STATUS_READY;
}


uint8_t TWI_MasterBaud(speed_t speed) {
	if (speed == CLK_24MHZ)
		return TWI_BAUD(24000000, 100000);
	return 0;
}
