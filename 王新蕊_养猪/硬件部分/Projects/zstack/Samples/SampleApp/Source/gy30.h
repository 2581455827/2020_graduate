#ifndef _GY30_H
#define _GY30_H
 
 
#include <ioCC2530.h>
 
#define uint unsigned int
#define uchar unsigned char
 
#define SCL P0_5
#define SDA P0_4
 
#define set_sda_in()  (P0DIR &=~(1<<4))
#define set_sda_out() (P0DIR |= 1<<4)
 
 
#define LIGHT_INIT()                           \
do{                                          \
  P0SEL &= ~0x30;                        \
  P0DIR |=0x30;                           \
  P0_5 = 1;                                  \
  P0_4 = 1;                              \
}while(0);               
 
#define SlaveAddress   0x46 //ADDR引脚接地时地址为0X23,接电源时地址为0X5C
#define PowerOn        0x01  
#define ContinuHMode          0x10
 
unsigned short get_light(void);
uint Light(void);
void Delay_1us(void);
void IIC_MasterACK(uint ack);
uchar IIC_SalveCK(void);


#endif