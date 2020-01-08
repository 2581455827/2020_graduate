#include "gy30.h"

uint8_t BUF[4]={0};

void BH1750_Start()
{
    HAL_GPIO_WritePin(GPIOE, SDA,GPIO_PIN_SET);                   
    HAL_GPIO_WritePin(GPIOE, SCL,GPIO_PIN_SET);                   
    delay_us(5);                 
    HAL_GPIO_WritePin(GPIOE, SDA,GPIO_PIN_RESET);                    
    delay_us(5);                 //??
    HAL_GPIO_WritePin(GPIOE, SCL,GPIO_PIN_RESET);                    
}
void BH1750_Stop(){
	HAL_GPIO_WritePin(GPIOE, SDA,GPIO_PIN_RESET);                   
    HAL_GPIO_WritePin(GPIOE, SCL,GPIO_PIN_SET);                    
    delay_us(5);                 
    HAL_GPIO_WritePin(GPIOE, SDA,GPIO_PIN_SET);                    
    delay_us(5);                 
}
void mread(void){
		uint8_t i;	
    BH1750_Start();                          //????
    BH1750_SendByte(SlaveAddress+1);         //??????+???
	
	 for (i=0; i<3; i++)                      //????6?????,???BUF
    {
        BUF[i] = BH1750_RecvByte();          //BUF[0]??0x32??????
        if (i == 3)
        {
           BH1750_SendACK(1);                //?????????NOACK
        }
        else
        {		
          BH1750_SendACK(0);                //??ACK
        }
   }
 
    BH1750_Stop();                          //????
    HAL_Delay(5);
 
}
void BH1750_SendACK(int ack)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct); 
	
	
    if(ack == 1)   
			HAL_GPIO_WritePin(GPIOE, SDA,GPIO_PIN_SET); 
		else if(ack == 0)
			HAL_GPIO_WritePin(GPIOE, SDA,GPIO_PIN_RESET);
		else
			return;
			
    HAL_GPIO_WritePin(GPIOB, SCL,GPIO_PIN_SET);      
    delay_us(5);                
    HAL_GPIO_WritePin(GPIOB, SCL,GPIO_PIN_RESET);      
    delay_us(5);                 
}
int BH1750_RecvACK()
{
	  GPIO_InitTypeDef GPIO_InitStruct;
	
	  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;  /*???????????,????????*/
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pin = SDA;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct); 	
	
    HAL_GPIO_WritePin(GPIOB, SCL,GPIO_PIN_SET);            //?????
    delay_us(5);                 //??
	
	  if(HAL_GPIO_ReadPin( GPIOB, SDA ) == 1 )//?????
        mcy = 1 ;  
    else
        mcy = 0 ;			
	
    HAL_GPIO_WritePin(GPIOB, SCL,GPIO_PIN_RESET);                    //?????
    delay_us(5);                 //??
  
   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
   HAL_GPIO_Init( GPIOE, &GPIO_InitStruct );
	
    return mcy;
}
void BH1750_SendByte(uint8_t dat)
{
    uint8_t i;
 
    for (i=0; i<8; i++)         //8????
      {
				if( 0X80 & dat )
          HAL_GPIO_WritePin(GPIOE, SDA,GPIO_PIN_SET);
        else
          HAL_GPIO_WritePin(GPIOE, SDA,GPIO_PIN_RESET);
			 
       dat <<= 1;
        HAL_GPIO_WritePin(GPIOE, SCL,GPIO_PIN_SET);               //?????
        delay_us(5);             //??
        HAL_GPIO_WritePin(GPIOE, SCL,GPIO_PIN_RESET);                //?????
        delay_us(5);             //??
      }
    BH1750_RecvACK();
}
uint8_t  BH1750_RecvByte()
{
		uint8_t i;
    uint8_t dat = 0;
	  uint8_t bit;
	  
	 GPIO_InitTypeDef GPIO_InitStruct;
	
	 GPIO_InitStruct.Mode = GPIO_MODE_INPUT;   /*???????????,????????*/
   GPIO_InitStruct.Pin = SDA;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
   HAL_GPIO_Init( GPIOE, &GPIO_InitStruct );
	
    HAL_GPIO_WritePin(GPIOE, SDA,GPIO_PIN_SET);          //??????,??????,
    for (i=0; i<8; i++)         //8????
    {
        dat <<= 1;
        HAL_GPIO_WritePin(GPIOB, SCL,GPIO_PIN_SET);               //?????
        delay_us(5);             //??
			
			  if( SET == HAL_GPIO_ReadPin( GPIOE, SDA ) )
             bit = 0X01;
       else
             bit = 0x00;  
			
        dat |= bit;             //???    
			
        HAL_GPIO_WritePin(GPIOB, SCL,GPIO_PIN_RESET);                //?????
        delay_us(5);             //??
    }
		
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init( GPIOE, &GPIO_InitStruct );
    return dat;
}

void Init_BH1750(){
	BH1750_Start();//ÆðÊ¼ÐÅºÅ
	BH1750_SendByte(SlaveAddress);
	BH1750_SendByte(0x01);
	BH1750_Stop();
}
