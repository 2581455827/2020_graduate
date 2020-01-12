/*****************************************
* ����AVR��Ƭ��GY-30ģ��ͨ�ų��� 		 *
* ��    �ܣ�IICͨ�Ŷ�ȡ���ݲ���ʾ        *
* ʱ��Ƶ�ʣ��ڲ�11.0592M 						 *
* ��    �ƣ����˵���					 *
* �޸����ڣ�2011��4��20��				 *
* ���뻷����ICC-AVR7.14					 *
* ʵ�黷����ATmega16+1602    			 *
* ʹ�ö˿ڣ�PC0,PC1,PC6,PC7,PA4~PA7 	 *
* ��    ����Ī����ʵ�����24c02��ȡʵ��  *
*****************************************/
#include <iom16v.h>
#include "I2C.h"
#include "1602.h"
#include "delay.h"
void conversion(unsigned int i);
unsigned char display[9]={0,0,0,0,0,' ','l','u','x'};//��ʾ����

/*********************************************
����ת��,ʮ����������ת����10����
����ʮ�����Ʒ�Χ��0x0000-0x270f��0-9999��
����ֳɸ�ʮ��ǧλ����ascii������ʾ��
**********************************************/
void conversion(unsigned int i)  
{  
   	display[0]=i/10000+0x30 ;
    i=i%10000;    //ȡ������
	display[1]=i/1000+0x30 ;
    i=i%1000;    //ȡ������
    display[2]=i/100+0x30 ;
    i=i%100;    //ȡ������
    display[3]=i/10+0x30 ;
    i=i%10;     //ȡ������
    display[4]=i+0x30;  
}
/*******************************
������
*******************************/
void main(void)
{	
	unsigned char i;
	float  lux_data;                   //������   
	 
	 delay_nms(10);                    //lcd�ϵ���ʱ
	 LCD_init();                       //lcd��ʼ��
     i=I2C_Write(0x01);                //BH1750 ��ʼ��            
	 delay_nms(10);          
	while(1){                          //ѭ��   
	 i=I2C_Write(0x01);                //power on
	 i=I2C_Write(0x10);                //H- resolution mode
	 TWCR=0;                           //�ͷ�����
     delay_nms(180);                   //��Լ180ms
	   if(i==0){
	     lux_data=I2C_Read();          //��iic���߶�ȡ��ֵ	
		 lux_data=(float)lux_data/1.2; //pdf�ĵ���7ҳ
	     conversion(lux_data);         //����ת��������ʮ���٣�ǧ λ       
		 LCD_write_string(7,0,display);//��ʾ��ֵ���ӵ�9�п�ʼ   
	   }  

    }
}

