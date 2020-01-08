#include "gy30.h"
 


 
/****************
*  延时函数1us
****************/
void Delay_1us(void) 
{
 
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
 
}
 
/****************
*  延时函数5us
****************/
static void delay_nus(void)
{
 
  Delay_1us();
  Delay_1us();
  Delay_1us();
  Delay_1us();
  Delay_1us(); 
 
}
 
/****************
*   延时函数ms
****************/
static void delay_nms(uint ms)
{
 
  uint i;
  while(ms--)
  {
 
    for(i=0;i<200;i++)
      delay_nus();
    
} 
 
}
 
/****************
*  开始函数
****************/
static void start_i2c(void)
{
 
  set_sda_out() ;
  SDA=1;
  SCL=1 ;
  delay_nus() ;
  SDA=0 ;
  delay_nus()  ;
  SCL=0;
 
}
 
/****************
*   停止函数
****************/
static void stop_i2c(void)
{
 
  set_sda_out() ;
  SDA=0;
  delay_nus();
  SCL=1;
  delay_nus();
  SDA=1;;
  delay_nus(); 
 
}
 
/*********************
*  从机应答 1：不应答
*           0：应答
**********************/
uchar IIC_SalveCK(void)
{
 
   uchar error;
   set_sda_in();
   SCL=1;
   delay_nus();
   if(SDA==1)
       error=1;
   else
       error=0;
   delay_nus();
   SCL=0 ;
   return error;
 
}
 
/************************
* 主机向从机写数据 
*  val：要写的数据         
************************/
static void i2c_send(uchar val)                 
{
 
        int i;
        set_sda_out();
        for(i=0;i<8;i++)
{
 
  if(val&0x80)
   SDA=1;
  else
     SDA=0;
   val<<=1;
   delay_nus();
   SCL=1; ; 
   delay_nus();
   SCL=0;;
   delay_nus();
 
}
        IIC_SalveCK();      
 
}
 
/*********************
*  主机应答 1：应答
*           0：不应答
**********************/
void IIC_MasterACK(uint ack)
{
 
    set_sda_out();
    if(ack)
         SDA=0;
     else
         SDA=1;
    delay_nus();
    SCL=1;
    delay_nus();
    SCL=0;
    SDA=1;
 
}
 
/*************************
*  主机读取从机发来的数据 
*     val：读取到的值 
************************/
static char i2c_read()
{
        int i;
        char val=0;
        SDA=1;
        set_sda_in();
        for(i=0;i<8;i++)
        {
          val<<=1;
          SCL=1;
          delay_nus();
          val |=SDA;
          delay_nus();
          SCL=0;
          delay_nus();                                       
        }
        return val;
         
 
}
 
/*************************
*  读取传感器的数据 
*    t:光感数值
************************/
unsigned short get_light(void)
{
  unsigned char t0;
  unsigned char t1;
  unsigned short t;
  start_i2c();
  i2c_send(SlaveAddress);
  i2c_send(PowerOn);
  stop_i2c();            
  start_i2c();
  i2c_send(SlaveAddress);
  i2c_send(ContinuHMode);
  stop_i2c();                     
  delay_nms(1); 
  start_i2c();
  i2c_send(SlaveAddress+1);                     
  t0 = i2c_read();
  IIC_MasterACK(1);
  t1 = i2c_read();
  IIC_MasterACK(0);
  stop_i2c();
  t =  ((short)t0)<<8;
  t |= t1;
  return t;
}
 
uint Light(void)
{
 
    uint w=0;
    LIGHT_INIT();
    w = (uint)(get_light()/1.2);
    uint lightData = w;  
    return lightData;    
 
}