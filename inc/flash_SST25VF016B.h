#ifndef FLASH_H_SST25VF016B 
#define FLASH_H_SST25VF016B 

#include "stm32f4xx_hal.h"

void SendByte(uint8_t);
uint8_t ReceiveByte(void);

void initFlash(void);
void ReadDataArrayFromAddress(uint32_t,uint8_t*,uint16_t);
void WriteDataArrayWithAAI(uint32_t,uint8_t*,uint8_t);
void WriteToStatusRegister(uint8_t);
uint8_t readStatusRegister(void);
void Clear(void);
void disableFlash(void);
void enableFlash(void);


#endif