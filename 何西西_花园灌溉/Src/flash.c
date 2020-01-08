#include "flash.h"

 

uint32_t startAddress = 0;
uint32_t endAddress = 0;
uint32_t data32 = 0;
uint32_t MemoryProgramStatus = 0;

void writeFlashTest(uint32_t addr,unsigned char data[],uint8_t len){
	
	HAL_FLASH_Unlock();   
	EraseInitStruct.TypeErase = TYPEERASE_SECTORS;
	EraseInitStruct .VoltageRange =VOLTAGE_RANGE_3 ;
	EraseInitStruct.Sector = FLASH_SECTOR_2 ;
	EraseInitStruct.NbSectors = 2;

	uint32_t SectorError = 0;
	
	HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);

	startAddress = 0x08008000;
	endAddress = 0x0800C000;
	
	for(int i = 0;i<len;i++){
		if (HAL_FLASH_Program(TYPEPROGRAM_WORD, startAddress, data[i]) == HAL_OK)
    {
      startAddress = startAddress + 4;
    }
	}
	
	
	
	/*
	while (startAddress < endAddress)
  {
    if (HAL_FLASH_Program(TYPEPROGRAM_WORD, startAddress, DATA_32) == HAL_OK)
    {
      startAddress = startAddress + 4;
    }
  }
*/
	HAL_FLASH_Lock();
/*
	startAddress = 0x08008000;
	 MemoryProgramStatus = 0x0;
  
  while (startAddress < endAddress)
  {
    data32 = startAddress;

    if (data32 != DATA_32)
    {
      MemoryProgramStatus++;  
    }

    startAddress = startAddress + 4;
  }  
 
 //[i]Check if there is an issue to program data[/i]/
  if (MemoryProgramStatus == 0)
  {
   //[i] No error detected. Switch on LED4[/i]/
   HAL_GPIO_WritePin (GPIOF,GPIO_PIN_9,GPIO_PIN_SET);
  }
  else
  {
    //[i] Error detected. Switch on LED5[/i]/
   // Error_Handler();
  }
 */
}

uint32_t printFlashTest(uint32_t addr){
	uint32_t temp = *(__IO uint32_t*)(addr);
	return temp;
}
