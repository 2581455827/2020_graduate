#ifndef __DHT11_H__
#define __DHT11_H__

typedef unsigned char uchar;
typedef unsigned int  uint;

extern void Delay_ms(unsigned int xms);	//延时函数
extern void COM(void);                  // 温湿写入
extern uchar* DHT11(void);                //温湿传感启动

extern uchar temp[2]; 
extern uchar temp1[5];
extern uchar humidity[2];
extern uchar humidity1[9];
extern uchar shidu_shi,shidu_ge,wendu_shi,wendu_ge;

#endif