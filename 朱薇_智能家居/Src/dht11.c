/**
  ******************************************************************************
  * ????: bsp_DHT11.c 
  * ?    ?: ?????????
  * ?    ?: V1.0
  * ????: 2015-10-04
  * ?    ?: DHT11????????????
  ******************************************************************************
  * ??:
  * ???????stm32???YS-F1Pro???
  * 
  * ??:
  * ??:http://www.ing10bbs.com
  * ??????????????,?????
  ******************************************************************************
  */
/* ????? ----------------------------------------------------------------*/
#include "dht11.h"
 
 
/* ?????? --------------------------------------------------------------*/
/* ????? ----------------------------------------------------------------*/
#define Delay_ms(x)   HAL_Delay(x)
/* ???? ------------------------------------------------------------------*/
/* ???? ------------------------------------------------------------------*/
/* ?????? --------------------------------------------------------------*/
static void DHT11_Mode_IPU(void);
static void DHT11_Mode_Out_PP(void);
static uint8_t DHT11_ReadByte(void);

/* ??? --------------------------------------------------------------------*/
/**
  * ????: 
  * ????: ?
  * ? ? ?: ?
  * ?    ?:?
  */
	/*
static void DHT11_Delay(uint16_t time)
{
	TIM_HandleTypeDef htim7;
	__HAL_TIM_SET_COUNTER(&htim7, 0);
	
	HAL_TIM_Base_Start(&htim7);
	
	while (__HAL_TIM_GET_COUNTER(&htim7) != us);
	
	HAL_TIM_Base_Stop(&htim7);
 	
 
        uint8_t i;

  while(time)
  {    
          for (i = 0; i < 10; i++)
    {
      
    }
    time--;
  }
	
}
*/
/**
  * ????: DHT11 ?????
  * ????: ?
  * ? ? ?: ?
  * ?    ?:?
  */
void DHT11_Init ( void )
{
	DHT11_Dout_GPIO_CLK_ENABLE();
	DHT11_Mode_Out_PP();
	DHT11_Dout_HIGH();  // ??GPIO
}

/**
  * ????: ?DHT11-DATA??????????
  * ????: ?
  * ? ? ?: ?
  * ?    ?:?
  */
static void DHT11_Mode_IPU(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  
  /* ??????GPIO?? */
  GPIO_InitStruct.Pin   = DHT11_Dout_PIN;
  GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  HAL_GPIO_Init(DHT11_Dout_PORT, &GPIO_InitStruct);
        
}

/**
  * ????: ?DHT11-DATA??????????
  * ????: ?
  * ? ? ?: ?
  * ?    ?:?
  */
static void DHT11_Mode_Out_PP(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  
  /* ??????GPIO?? */
  GPIO_InitStruct.Pin = DHT11_Dout_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = 	GPIO_SPEED_FREQ_HIGH;			//GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(DHT11_Dout_PORT, &GPIO_InitStruct);          
}

/**
  * ????: ?DHT11??????,MSB??
  * ????: ?
  * ? ? ?: ?
  * ?    ?:?
  */
static uint8_t DHT11_ReadByte ( void )
{
        uint8_t i, temp=0;
        

        for(i=0;i<8;i++)    
        {         
                /*?bit?50us???????,???????? ?50us ??? ??*/  
                while(DHT11_Data_IN()==GPIO_PIN_RESET);

                /*DHT11 ?26~28us??????�0�,?70us?????�1�,
                 *???? x us???????????? ,x ?????? 
                 */
                delay_us(40); //??x us ??????????0???????                     

                if(DHT11_Data_IN()==GPIO_PIN_SET)/* x us??????????�1� */
                {
                        /* ????1?????? */
                        while(DHT11_Data_IN()==GPIO_PIN_SET);

                        temp|=(uint8_t)(0x01<<(7-i));  //??7-i??1,MSB?? 
                }
                else         // x us?????????�0�
                {                           
                        temp&=(uint8_t)~(0x01<<(7-i)); //??7-i??0,MSB??
                }
        }
        return temp;
}


/*
 * 
 * 
 */
/**
  * ????: ??????????40bit,????
  * ????: DHT11_Data:DHT11????
  * ? ? ?: ERROR:  ????
  *           SUCCESS:????
  * ?    ?:8bit ???? + 8bit ???? + 8bit ???? + 8bit ???? + 8bit ??? 
  */
uint8_t DHT11_Read_TempAndHumidity(DHT11_Data_TypeDef *DHT11_Data)
{  
  uint8_t temp;
  uint16_t humi_temp;
  
        /*????*/
        DHT11_Mode_Out_PP();
        /*????*/
        DHT11_Dout_LOW();
        /*??18ms*/
			HAL_Delay(19);

        /*???? ????30us*/
        DHT11_Dout_HIGH(); 

        delay_us(30);   //??30us

        /*?????? ????????*/ 
        DHT11_Mode_IPU();
					 
        /*?????????????? ???????,???????*/   
		if(DHT11_Data_IN()==GPIO_PIN_RESET)     
		{
			/*???????? ?80us ??? ??????*/  
			while(DHT11_Data_IN()==GPIO_PIN_RESET);

			/*????????? 80us ??? ??????*/
			while(DHT11_Data_IN()==GPIO_PIN_SET);

			/*??????*/   
			DHT11_Data->humi_high8bit= DHT11_ReadByte();
			DHT11_Data->humi_low8bit = DHT11_ReadByte();
			DHT11_Data->temp_high8bit= DHT11_ReadByte();
			DHT11_Data->temp_low8bit = DHT11_ReadByte();
			DHT11_Data->check_sum    = DHT11_ReadByte();

			/*????,????????*/
			DHT11_Mode_Out_PP();
			/*????*/
			DHT11_Dout_HIGH();
			
			/* ??????? */
			humi_temp=DHT11_Data->humi_high8bit*100+DHT11_Data->humi_low8bit;
			DHT11_Data->humidity =(float)humi_temp/100;
			
			humi_temp=DHT11_Data->temp_high8bit*100+DHT11_Data->temp_low8bit;
			DHT11_Data->temperature=(float)humi_temp/100;    
			
			/*???????????*/
			temp = DHT11_Data->humi_high8bit + DHT11_Data->humi_low8bit + 
						 DHT11_Data->temp_high8bit+ DHT11_Data->temp_low8bit;
			if(DHT11_Data->check_sum==temp)
			{ 
				return 2;
			}
			else 
				return 1;
		}        
		else	
						return 0;
}

/******************* (C) COPYRIGHT 2015-2020 ????????? *****END OF FILE****/
