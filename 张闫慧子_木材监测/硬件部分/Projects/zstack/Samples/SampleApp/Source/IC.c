#include "variable.h"
#include "IC_w_r.h"
#include "MT_UART.h"


uchar ucTagType[4]={0};
uchar buf[4];

void Initial(void);
uchar IC_Test(void);
void IC_Init(void);


void Initial(void)
{
  //IC_SDA P2_0
  P2DIR |= 1<<0;
  P2INP |= 1<<0;
  P2SEL &= ~(1<<0);
  
  // IC_SCK  P0_7
  P0DIR |= 1<<7;
  P0INP |= 1<<7;
  P0SEL &= ~(1<<7);
  
  // IC_MOSI P0_6
  P0DIR |= 1<<6;
  P0INP |= 1<<6;
  P0SEL &= ~(1<<6);
  
  // IC_MISO P0_5
  P0DIR |= 1<<5;
  P0INP |= 1<<5;
  P0SEL &= ~(1<<5);  

  // IC_RST P0_4
  P0DIR &= ~(1<<4);
  P0INP &= ~(1<<4);
  P0SEL &= ~(1<<4);
  
  IC_SCK = 1;
  IC_CS = 1;
}


uchar IC_Test(void)
{
  uchar find=0xaa;
  uchar ret;   
  
  ret = PcdRequest(0x52,buf);//Ñ°¿¨
  if(ret != 0x26)
    ret = PcdRequest(0x52,buf);
  if(ret != 0x26)
    find = 0xaa;
  if((ret == 0x26)&&(find == 0xaa))
  {
    if(PcdAnticoll(ucTagType) == 0x26);//·À³å×²
    {
      // HalUARTWrite(0,ucTagType,4);
      find = 0x00;
      return 1;
    }
  }
 
   return 0;
}

void IC_Init(void)
{
  Initial();
  PcdReset();
  M500PcdConfigISOType('A');
}