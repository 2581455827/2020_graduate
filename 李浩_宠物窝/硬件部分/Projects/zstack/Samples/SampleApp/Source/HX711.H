#ifndef __HX711_H__
#define __HX711_H__


#include <ioCC2530.h>

//IO设置
 #define HX711_DOUT P0_6
#define HX711_SCK P0_5
  

//函数或者变量声明
//extern void DelayMS(unsigned int ms);
extern void Delay__hx711_us(void);
extern unsigned long HX711_Read(void);

#endif