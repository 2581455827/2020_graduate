#include "GY-30.h"

 
//SCL -> PC1
//SDA -> PC2
#define SCL_H()  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1,GPIO_PIN_SET)
#define SCL_L()  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1,GPIO_PIN_RESET)
#define SDA_H()  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2,GPIO_PIN_SET)
#define SDA_L()  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2,GPIO_PIN_RESET)
 
#define SDA  HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_2)
 
 
#define	  SlaveAddress   0x46 //定义器件在IIC总线中的从地址,根据ALT  ADDRESS地址引脚不同修改
                              //ALT  ADDRESS引脚接地时地址为0x46，接电源时地址为0xB8

uint8_t    BUF[8];                         //接收数据缓存区      	
//uint8_t   ge,shi,bai,qian,wan;            //显示变量
int     dis_data;                       //变量
float Value_GY_30;

//void Init_BH1750(void);
//void conversion(uint8_t temp_data);

//void  Single_Write_BH1750(uint8_t REG_Address);               //单个写入数据
//uint8_t Single_Read_BH1750(uint8_t REG_Address);                //单个读取内部寄存器数据
//void  Multiple_Read_BH1750();                               //连续的读取内部寄存器数据
 
//void IIC_Init(void)
//{
//    SCL_H();  //SCL = 1;
//    delay_us(5);
//    SDA_H();  //SDA = 1;
//    delay_us(5);    
//}
 
void IIC_Start(void)
{
    SDA_H();  //SDA = 1;
//    delay_us(5);
    SCL_H();  //SCL = 1;
    delay_us(5);
    SDA_L();  //SDA = 0;
    delay_us(5);    
	  SCL_L();  //SCL = 0;
}
 
void IIC_Stop(void)
{
    SDA_L();   //SDA = 0;
//    delay_us(5);
    SCL_H();   //SCL = 1;
    delay_us(5);
    SDA_H();   //SDA = 1;
    delay_us(5);
}
 
unsigned char IIC_ReceiveACK(void)
{
    unsigned char ACK;
 
    SDA_H();     //SDA=1;要读低电平需先拉高再读，否则读到的是错误数据
    SCL_H();     //SCL=1;
    delay_us(5);
 
    if (SDA==1)  //SDA??
    {
        ACK = 1;    
    }
    else ACK = 0;  //SDA??
 
 
    SCL_L();    //SCL = 0;SCL为低电平时SDA上的数据才允许变化，为传送下一个字节做准备
    delay_us(5);
     
    return ACK;                 
}
 
void IIC_SendACK(unsigned char ack)
{
    if (ack == 1)SDA_H();
    else if (ack == 0)SDA_L();
    //SDA = ack;
    SCL_H();   //SCL = 1;
    delay_us(5);
    SCL_L();   //SCL = 0;
    delay_us(5);
}
 
unsigned char IIC_SendByte(unsigned char dat)
{
    unsigned char i;
    unsigned char bResult=1;
	
    SDA_H();
    SCL_L();     //SCL = 0;拉低时钟线
	  
    delay_us(5);        
 
    for( i=0;i<8;i++ ) //一个SCK,把dat一位一位的移送到SDA上
    {
        if( (dat<<i)&0x80 )SDA_H();   //SDA = 1;先发高位
        else SDA_L();  //SDA = 0;
        delay_us(5);
 
        SCL_H();  //SCL = 1;
        delay_us(5);
        SCL_L();  //SCL = 0;
        delay_us(5);
    }
 
    bResult=IIC_ReceiveACK(); //发送完一个字节的数据，等待接收答应信号
 
    return bResult;  //返回答应信号
}
 
unsigned char IIC_ReadByte()
{
    unsigned char dat;
    unsigned char i;
     
    SCL_H();     //SCL = 1;时钟线拉高为读数据，做准备
	  SDA_H();
    delay_us(5);
 
    for( i=0;i<8;i++ )
    {
        dat <<= 1;
			  SCL_H();   //SCL = 1;
			  delay_us(5);
        dat = dat | (SDA);
        SCL_L();   //SCL = 0;
        delay_us(5);       
    }
    return dat;
}
//unsigned char IIC_ReadByte()  //错误反例，计算-低电平-高电平，正确的为 ：高电平-计算-低电平
//{
//    unsigned char dat;
//    unsigned char i;
//     
//    SCL_H();     //SCL = 1;时钟线拉高为读数据，做准备
//	  SDA_H();
//    delay_us(5);
// 
//    for( i=0;i<8;i++ )
//    {
//        dat <<= 1;
//        dat = dat | (SDA);
//        delay_us(5);
//         
//        SCL_L();   //SCL = 0;
//        delay_us(5);    
//        SCL_H();   //SCL = 1;
//        delay_us(5);    
//    }
//    return dat;
//}
//void conversion(uint8_t temp_data)  //  数据转换出 个，十，百，千，万
//{  
//    wan=temp_data/10000+0x30 ;
//    temp_data=temp_data%10000;   //取余运算
//	  qian=temp_data/1000+0x30 ;
//    temp_data=temp_data%1000;    //取余运算
//    bai=temp_data/100+0x30   ;
//    temp_data=temp_data%100;     //取余运算
//    shi=temp_data/10+0x30    ;
//    temp_data=temp_data%10;      //取余运算
//    ge=temp_data+0x30; 	
//}
void Single_Write_BH1750(uint8_t REG_Address)
{
    IIC_Start();                  //起始信号
    IIC_SendByte(SlaveAddress);   //发送设备地址+写信号
    IIC_SendByte(REG_Address);    //内部寄存器地址，
  //  BH1750_SendByte(REG_data);       //内部寄存器数据，
    IIC_Stop();                   //发送停止信号
}
//********单字节读取*****************************************
/*
uchar Single_Read_BH1750(uchar REG_Address)
{  
    uchar REG_data;
    BH1750_Start();                          //起始信号
    BH1750_SendByte(SlaveAddress);           //发送设备地址+写信号
    BH1750_SendByte(REG_Address);                   //发送存储单元地址，从0开始	
    BH1750_Start();                          //起始信号
    BH1750_SendByte(SlaveAddress+1);         //发送设备地址+读信号
    REG_data=BH1750_RecvByte();              //读出寄存器数据
	  BH1750_SendACK(1);   
	  BH1750_Stop();                           //停止信号
    return REG_data; 
}
*/
//*********************************************************
//
//连续读出BH1750内部数据
//
//*********************************************************
void Multiple_Read_BH1750()
{   
	uint8_t i;	
    IIC_Start();                          //起始信号
    IIC_SendByte(SlaveAddress+1);         //发送设备地址+读信号
	
	 for (i=0; i<3; i++)                      //连续读取2个地址数据，存储中BUF
    {
        BUF[i] = IIC_ReadByte();          //BUF[0]存储0x32地址中的数据
        if (i == 3)
        {

           IIC_SendACK(1);                //最后一个数据需要回NOACK
        }
        else
        {		
          IIC_SendACK(0);                //回应ACK
       }
   }

    IIC_Stop();                          //停止信号
    delay_us(5);
}
void Init_BH1750()
{
	 delay_ms(100);	    //延时100ms	
   Single_Write_BH1750(0x01);  
}
float Value_GY30()
{
	 Single_Write_BH1750(0x01);   // power on
   Single_Write_BH1750(0x10);   // H- resolution mode

   delay_us(180);              //延时180ms

   Multiple_Read_BH1750();       //连续读出数据，存储在BUF中
	
   dis_data=BUF[0];
   dis_data=(dis_data<<8)+BUF[1];//合成数据，即光照数据
    
   Value_GY_30=(float)dis_data/1.2;
	return Value_GY_30;
   // printf("GY-30=%.1f\r\n",Value_GY_30); 
   //conversion(temp);         //计算数据和显示
	
}
