#ifndef __GY_30_H__
#define __GY_30_H__

#include "stm32f4xx_hal.h"
//#include <math.h>
#include "tim.h"

#define delay_ms(a) HAL_Delay(a)

extern void Init_BH1750(void);
extern float Value_GY30(void);

#endif
