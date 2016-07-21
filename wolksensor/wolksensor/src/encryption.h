#include "platform_specific.h"

#ifndef ENCRYPTION_H_
#define ENCRYPTION_H_

uint16_t encrypt(uint8_t *buff, uint16_t size, uint8_t* key);
void decrypt(uint8_t *message, uint16_t message_len, uint8_t* key);

#endif /* ENCRYPTION_H_ */