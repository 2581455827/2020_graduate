/*****************************************
* 基于AVR单片机GY-30模块通信程序 		 *
* 功    能：IIC通信读取数据并显示        *
* 时钟频率：内部11.0592M 						 *
* 设    计：广运电子					 *
* 修改日期：2011年4月20日				 *
* 编译环境：ICC-AVR7.14					 *
* 实验环境：ATmega16+1602    			 *
* 使用端口：PC0,PC1,PC6,PC7,PA4~PA7 	 *
* 参    考：莫锦攀实验程序24c02读取实验  *
*****************************************/
#include <iom16v.h>
#include "I2C.h"
#include "1602.h"
#include "delay.h"
void conversion(unsigned int i);
unsigned char display[9]={0,0,0,0,0,' ','l','u','x'};//显示数据

/*********************************************
数据转换,十六进制数据转换成10进制
输入十六进制范围：0x0000-0x270f（0-9999）
结果分成个十百千位，以ascii存入显示区
**********************************************/
void conversion(unsigned int i)  
{  
   	display[0]=i/10000+0x30 ;
    i=i%10000;    //取余运算
	display[1]=i/1000+0x30 ;
    i=i%1000;    //取余运算
    display[2]=i/100+0x30 ;
    i=i%100;    //取余运算
    display[3]=i/10+0x30 ;
    i=i%10;     //取余运算
    display[4]=i+0x30;  
}
/*******************************
主程序
*******************************/
void main(void)
{	
	unsigned char i;
	float  lux_data;                   //光数据   
	 
	 delay_nms(10);                    //lcd上电延时
	 LCD_init();                       //lcd初始化
     i=I2C_Write(0x01);                //BH1750 初始化            
	 delay_nms(10);          
	while(1){                          //循环   
	 i=I2C_Write(0x01);                //power on
	 i=I2C_Write(0x10);                //H- resolution mode
	 TWCR=0;                           //释放引脚
     delay_nms(180);                   //大约180ms
	   if(i==0){
	     lux_data=I2C_Read();          //从iic总线读取数值	
		 lux_data=(float)lux_data/1.2; //pdf文档第7页
	     conversion(lux_data);         //数据转换出个，十，百，千 位       
		 LCD_write_string(7,0,display);//显示数值，从第9列开始   
	   }  

    }
}

