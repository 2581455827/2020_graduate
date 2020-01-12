#include <macros.h>
#include "delay.h"

//使用AVR内部硬件iic，引脚定义
//PC0->SCL  ;  PC1->SDA
//I2C 状态定义
//MT 主方式传输 MR 主方式接受
#define START			0x08
#define RE_START		0x10
#define MT_SLA_ACK		0x18
#define MT_SLA_NOACK 	0x20
#define MT_DATA_ACK		0x28
#define MT_DATA_NOACK	0x30
#define MR_SLA_ACK		0x40
#define MR_SLA_NOACK	0x48
#define MR_DATA_ACK		0x50
#define MR_DATA_NOACK	0x58		

#define RD_DEVICE_ADDR  0x47   //ADDR脚接地时的读地址
#define WD_DEVICE_ADDR  0x46   //ADDR脚接地时的写地址

//常用TWI操作(主模式写和读)
#define Start()			(TWCR=(1<<TWINT)|(1<<TWSTA)|(1<<TWEN))		//启动I2C
#define Stop()			(TWCR=(1<<TWINT)|(1<<TWSTO)|(1<<TWEN))		//停止I2C
#define Wait()			{while(!(TWCR&(1<<TWINT)));}				//等待中断发生
#define TestAck()		(TWSR&0xf8)									//观察返回状态
#define SetAck			(TWCR|=(1<<TWEA))							//做出ACK应答
#define SetNoAck		(TWCR&=~(1<<TWEA))							//做出Not Ack应答
#define Twi()			(TWCR=(1<<TWINT)|(1<<TWEN))				    //启动I2C
#define Write8Bit(x)	{TWDR=(x);TWCR=(1<<TWINT)|(1<<TWEN);}		//写数据到TWDR

unsigned char I2C_Write(unsigned char Wdata);
unsigned int I2C_Read();

/*********************************************
I2C总线写一个字节
返回0:写成功
返回1:写失败
**********************************************/
unsigned char I2C_Write(unsigned char Wdata)
{
	  Start();						//I2C启动
	  Wait();
	  if(TestAck()!=START) 
	  return 1;					//ACK
	  
	  Write8Bit(WD_DEVICE_ADDR);	//写I2C从器件地址和写方式
	  Wait();
	  if(TestAck()!=MT_SLA_ACK) 
	  return 1;					//ACK  
	  
	  Write8Bit(Wdata);			 	//写数据到器件相应寄存器
	  Wait();
	  if(TestAck()!=MT_DATA_ACK) 
	  return 1;				    //ACK	 
	  Stop();  						//I2C停止 
	  return 0;
}

/*********************************************
I2C总线读一个字节
返回：16位数值
**********************************************/
unsigned int I2C_Read()
{
   unsigned int temp;
   
	  Start();						//I2C启动
	  Wait();
	  if(TestAck()!=START) 
	  return 1;					   //ACK  
   
      Write8Bit(RD_DEVICE_ADDR);   //写I2C从器件地址和写方式
	  Wait();
	  if(TestAck()!=MR_SLA_ACK) 
	  return 1;					   //ACK
	  
      Twi();                       //启动主I2C读方式
	  TWCR = 0xC4;                 //清中断标志，结果应答ACK
	  Wait();     
	  temp=TWDR;                   //读取I2C接收数据 第一字节 
	   
	  Twi();	 				   //启动主I2C读方式,结果应答NO_ACK
	  Wait();   
	  temp = (temp<<8)+TWDR;       //读第二字节 合成16位数值
      Stop();                      //I2C停止
	  return temp;
}
	  