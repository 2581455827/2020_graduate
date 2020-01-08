#ifndef _GY30_H
#define _GY30_H

#include "gpio.h"
#include "tim.h"

#define SDA GPIO_PIN_14
#define SCL GPIO_PIN_15

uint16_t SlaveAddress = 0x46;
uint8_t mcy;

extern uint8_t BUF[4];
	
void BH1750_Start(void); //I2C��ʼ
void Init_BH1750(void); 
void BH1750_Stop(void);//I2Cֹͣ
void mread(void);
void BH1750_SendACK(int ack);//����Ӧ���ź�
int BH1750_RecvACK(void);//����Ӧ���ź�
void BH1750_SendByte(uint8_t dat);//I2C����һ���ֽ�
uint8_t BH1750_RecvByte(void);//I2C����һ���ֽ�
#endif
