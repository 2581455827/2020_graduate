#include "HX711.h"


//****************************************************
//延时函数
//****************************************************
void Delay__hx711_us(void)
{
    asm("nop");
    asm("nop");
    
}
/*
void DelayMS(unsigned int msec)
{ 
  unsigned int i,j;
  for(i=0; i<msec; i++)
    for(j=0; j<1060;j++);
}
*/
//****************************************************
//读取HX711
//****************************************************
unsigned long HX711_Read(void)	//增益128
{
 
        unsigned long count; 
	unsigned char i; 
  	HX711_DOUT=1; 
	Delay__hx711_us();
  	HX711_SCK=0; 
       Delay__hx711_us();
  	count=0;
	//EA = 1; 
  	while(HX711_DOUT); 
        //EA = 0;
  	for(i=0;i<24;i++)
	{ 
	  	HX711_SCK=1; 
               Delay__hx711_us();
	  	count=count<<1; 
		HX711_SCK=0; 
                Delay__hx711_us();
	  	if(HX711_DOUT)
			count++; 
	} 
 	HX711_SCK=1; 
        count=count^0x800000;//第25个脉冲下降沿来时，转换数据
	Delay__hx711_us();
	HX711_SCK=0;  
        Delay__hx711_us();
	return(count);
 
}






