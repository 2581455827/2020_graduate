#ifndef __FLASH_H__
#define __FLASH_H__

/* ????? ----------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_flash_ex.h"

 
 
#define DATA_32                 ((uint32_t)0x12345678)

extern uint32_t startAddress;
extern uint32_t endAddress;
extern uint32_t data32;
extern uint32_t MemoryProgramStatus;

static FLASH_EraseInitTypeDef EraseInitStruct;

void writeFlashTest(uint32_t addr,unsigned char data[],uint8_t len);
uint32_t printFlashTest(uint32_t addr);


 


#endif
