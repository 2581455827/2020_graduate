#include"ioCC2530.h"
#include"variable.h"

void UartInitial(void);
void UartSend(uchar infor);

void UartInitial(void)
{
  PERCFG = 0x00;
  P0SEL = 0x0c;
  P2DIR &= ~0xc0;
   U0CSR |= 0x80;//ÉèÖÃ´®¿Ú
 // U0UCR = 0x00;
  U0GCR |= 11;
  U0BAUD |= 216;
  U0CSR |= 0x40;
  UTX0IF = 0;
  
}


void UartSend(uchar infor)
{
  U0DBUF = infor;
  while(UTX0IF == 0);
  UTX0IF = 0;
}



