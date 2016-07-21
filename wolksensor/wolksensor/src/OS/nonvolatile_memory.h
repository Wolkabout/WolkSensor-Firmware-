/*
 * nvm.h
 *
 * Created: 7/17/2015 11:49:32 AM
 *  Author: btomic
 */ 


#ifndef NVM_H_
#define NVM_H_

bool config_read(void *data, uint8_t type, uint8_t version, uint8_t length);
bool config_write(void *data, uint8_t type, uint8_t version, uint8_t length);

#endif /* NVM_H_ */