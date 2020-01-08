#ifndef _GY30_H
#define _GY30_H

#include "gpio.h"
#include "tim.h"

#define SDA GPIO_PIN_14
#define SCL GPIO_PIN_15

uint16_t SlaveAddress = 0x46;
uint8_t mcy;

extern uint8_t BUF[4];
	
void BH1750_Start(void); //I2C开始
void Init_BH1750(void); 
void BH1750_Stop(void);//I2C停止
void mread(void);
void BH1750_SendACK(int ack);//发送应答信号
int BH1750_RecvACK(void);//接收应答信号
void BH1750_SendByte(uint8_t dat);//I2C发送一个字节
uint8_t BH1750_RecvByte(void);//I2C接收一个字节
#endif
