#ifndef TWI_MASTER_DRIVER_H_
#define TWI_MASTER_DRIVER_H_

/*! Baud register setting calculation. Formula described in datasheet. */
#define TWI_BAUD(F_SYS, F_TWI) ((F_SYS / (2 * F_TWI)) - 5)

/*! Transaction status defines. */
#define TWIM_STATUS_READY              0
#define TWIM_STATUS_BUSY               1

/*! Transaction result enumeration. */
typedef enum TWIM_RESULT_enum {
	TWIM_RESULT_UNKNOWN          = (0x00<<0),
	TWIM_RESULT_OK               = (0x01<<0),
	TWIM_RESULT_BUFFER_OVERFLOW  = (0x02<<0), // if the slave has returned more data than requested
	TWIM_RESULT_ARBITRATION_LOST = (0x03<<0), // can happen if there is noise on the bus, but theoretically should not happen
	TWIM_RESULT_BUS_ERROR        = (0x04<<0), // noise or while changing the sensor
	TWIM_RESULT_NACK_RECEIVED    = (0x05<<0), // nack from slave
	TWIM_RESULT_FAIL             = (0x06<<0),
} TWIM_RESULT_t;

/*! Buffer size defines */
#define TWIM_WRITE_BUFFER_SIZE         32
#define TWIM_READ_BUFFER_SIZE          32


/*! \brief TWI master driver struct */
typedef struct TWI_Master {
	TWI_t *interface;								/*!< Pointer to what interface to use */
	register8_t address;                            /*!< Slave address */
	register8_t writeData[TWIM_WRITE_BUFFER_SIZE];  /*!< Data to write */
	register8_t readData[TWIM_READ_BUFFER_SIZE];    /*!< Read data */
	register8_t bytesToWrite;                       /*!< Number of bytes to write */
	register8_t bytesToRead;                        /*!< Number of bytes to read */
	register8_t bytesWritten;                       /*!< Number of bytes written */
	register8_t bytesRead;                          /*!< Number of bytes read */
	register8_t status;                             /*!< Status of transaction */
	register8_t result;                             /*!< Result of transaction */
} TWI_Master_t;

// This is all atmels code that they require to be in this form
void TWI_MasterInit(TWI_Master_t *twi, TWI_t *module, TWI_MASTER_INTLVL_t intLevel, uint8_t baudRateRegisterSetting);
TWI_MASTER_BUSSTATE_t TWI_MasterState(TWI_Master_t *twi);
bool TWI_MasterReady(TWI_Master_t *twi);
bool TWI_MasterWrite(TWI_Master_t *twi, uint8_t address, uint8_t * writeData, uint8_t bytesToWrite);
bool TWI_MasterRead(TWI_Master_t *twi, uint8_t address, uint8_t bytesToRead);
bool TWI_MasterWriteRead(TWI_Master_t *twi, uint8_t address, uint8_t *writeData, uint8_t bytesToWrite, uint8_t bytesToRead);
void TWI_MasterInterruptHandler(TWI_Master_t *twi);
void TWI_MasterArbitrationLostBusErrorHandler(TWI_Master_t *twi);
void TWI_MasterWriteHandler(TWI_Master_t *twi);
void TWI_MasterReadHandler(TWI_Master_t *twi);
void TWI_MasterTransactionFinished(TWI_Master_t *twi, uint8_t result);
uint8_t TWI_MasterBaud(speed_t speed);

#endif /* TWI_MASTER_DRIVER_H_ */
