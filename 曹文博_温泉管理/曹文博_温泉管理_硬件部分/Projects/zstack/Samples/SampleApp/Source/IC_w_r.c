#include"variable.h"
#include"rc522.h"
#include"UART.h"

void Delay_I_1us(unsigned int k);
void SPIWriteByte(uchar infor);
unsigned char SPIReadByte(void);
unsigned char ReadRawRC(unsigned char Address);
void WriteRawRC(unsigned char Address, unsigned char value);
void SetBitMask(unsigned char reg,unsigned char mask) ;
void ClearBitMask(unsigned char reg,unsigned char mask)  ;
void PcdAntennaOn(void);
void PcdAntennaOff(void);
void PcdReset(void);
void IC_CMT(uchar *UID,uchar *KEY,uchar RW,char *Dat);
void M500PcdConfigISOType(unsigned char type);
char PcdComMF522(unsigned char Command, 		//RC522命令字
                 unsigned char *pInData, 		//通过RC522发送到卡片的数据
                 unsigned char InLenByte,		//发送数据的字节长度
                 unsigned char *pOutData, 		//接收到的卡片返回数据
                 unsigned int  *pOutLenBit)	;
char PcdRequest(unsigned char req_code,unsigned char *pTagType);
char PcdAnticoll(unsigned char *pSnr);
void CalulateCRC(unsigned char *pIndata,unsigned char len,unsigned char *pOutData);
char PcdRead(unsigned char addr,unsigned char *pData);
char PcdSelect(unsigned char *pSnr);
char PcdHalt(void);
char PcdHalt(void);
char PcdWrite(unsigned char addr,unsigned char *pData);
char PcdAuthState(unsigned char auth_mode,unsigned char addr,unsigned char *pKey,unsigned char *pSnr);


void Delay_I_1us(unsigned int k)
{
  uint i,j;
  for(i=0;i<k;i++)
    for(j=0;j<32;j++);
}

void SPIWriteByte(uchar infor)
{
  unsigned int counter;
  for(counter=0;counter<8;counter++)
  {
    
    if(infor&0x80)
      IC_MOSI = 1;
    else 
      IC_MOSI = 0;
    Delay_I_1us(3);
    
    IC_SCK = 0;
    Delay_I_1us(1);
    
    
    IC_SCK = 1; 
    Delay_I_1us(3);
    
    
    infor <<= 1; 
  } 
}

unsigned char SPIReadByte(void)
{
  unsigned int counter;
  unsigned char SPI_Data;
  for(counter=0;counter<8;counter++)
  {
    SPI_Data<<=1;
    
    IC_SCK = 0;
    Delay_I_1us(3);  
    
    
    if(IC_MISO == 1)
      SPI_Data |= 0x01;
    Delay_I_1us(2);
    
    IC_SCK = 1;
    Delay_I_1us(3);  
    
  }
  return SPI_Data;
}

/////////////////////////////////////////////////////////////////////
//功    能：读RC632寄存器
//参数说明：Address[IN]:寄存器地址
//返    回：读出的值
/////////////////////////////////////////////////////////////////////
unsigned char ReadRawRC(unsigned char Address)
{
  unsigned char ucAddr;
  unsigned char ucResult=0;
  IC_CS = 0;
  ucAddr = ((Address<<1)&0x7E)|0x80;//地址变换，SPI的读写地址有要求
  SPIWriteByte(ucAddr);
  ucResult=SPIReadByte();
  IC_CS = 1;
  return ucResult;
}
/////////////////////////////////////////////////////////////////////
//功    能：写RC632寄存器
//参数说明：Address[IN]:寄存器地址
//          value[IN]:写入的值
/////////////////////////////////////////////////////////////////////
void WriteRawRC(unsigned char Address, unsigned char value)
{  
  unsigned char ucAddr;
  Address <<= 1;
  ucAddr = (Address&0x7e);
  IC_CS = 0;
  
  SPIWriteByte(ucAddr);
  SPIWriteByte(value);
  IC_CS = 1;
}

/////////////////////////////////////////////////////////////////////
//功    能：置RC522寄存器位
//参数说明：reg[IN]:寄存器地址
//          mask[IN]:置位值
/////////////////////////////////////////////////////////////////////
void SetBitMask(unsigned char reg,unsigned char mask)  
{
  char tmp = 0x0;
  tmp = ReadRawRC(reg);
  WriteRawRC(reg,tmp | mask);  // set bit mask
}

/////////////////////////////////////////////////////////////////////
//功    能：清RC522寄存器位
//参数说明：reg[IN]:寄存器地址
//          mask[IN]:清位值
/////////////////////////////////////////////////////////////////////
void ClearBitMask(unsigned char reg,unsigned char mask)  
{
  char tmp = 0x0;
  tmp = ReadRawRC(reg);
  WriteRawRC(reg, tmp & ~mask);  // clear bit mask
} 




/////////////////////////////////////////////////////////////////////
//开启天线  
//每次启动或关闭天险发射之间应至少有1ms的间隔
/////////////////////////////////////////////////////////////////////
void PcdAntennaOn(void)
{
  unsigned char i;
  i = ReadRawRC(TxControlReg);
  if (!(i & 0x03))
  {
    SetBitMask(TxControlReg, 0x03);
  }
}

/////////////////////////////////////////////////////////////////////
//关闭天线
/////////////////////////////////////////////////////////////////////
void PcdAntennaOff(void)
{
  ClearBitMask(TxControlReg, 0x03);
}


/////////////////////////////////////////////////////////////////////
//功    能：复位RC522
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
void PcdReset(void)
{
  //PORTD|=(1<<RC522RST);
  IC_REST = 1;
  Delay_I_1us(1);
  //PORTD&=~(1<<RC522RST);
  IC_REST = 0;
  Delay_I_1us(1);
  //PORTD|=(1<<RC522RST);
  IC_REST = 1;
  Delay_I_1us(1);
  WriteRawRC(0x01,0x0f);
  //while(ReadRawRC(0x01)&0x10);   //检查卡片
  Delay_I_1us(1);
  
  WriteRawRC(ModeReg,0x3D);             //定义发送和接收常用模式 和Mifare卡通讯，CRC初始值0x6363
  WriteRawRC(TReloadRegL,30);           //16位定时器低位
  WriteRawRC(TReloadRegH,0);		//16位定时器高位
  WriteRawRC(TModeReg,0x8D);		//定义内部定时器的设置
  WriteRawRC(TPrescalerReg,0x3E);	//设置定时器分频系数
  WriteRawRC(TxAutoReg,0x40);		//	调制发送信号为100%ASK

  //return MI_OK;
}


//////////////////////////////////////////////////////////////////////
//设置RC632的工作方式 
//////////////////////////////////////////////////////////////////////
void M500PcdConfigISOType(unsigned char type)
{
  if (type == 'A')                     //ISO14443_A
  { 
    ClearBitMask(Status2Reg,0x08);
    WriteRawRC(ModeReg,0x3D);//3F
    WriteRawRC(RxSelReg,0x86);//84
    WriteRawRC(RFCfgReg,0x7F);   //4F
    WriteRawRC(TReloadRegL,30);//tmoLength);// TReloadVal = 'h6a =tmoLength(dec) 
    WriteRawRC(TReloadRegH,0);
    WriteRawRC(TModeReg,0x8D);
    WriteRawRC(TPrescalerReg,0x3E);
    Delay_I_1us(2);
    PcdAntennaOn();//开天线
  }
  //  else return (-1); 
  
  //return MI_OK;
}

/////////////////////////////////////////////////////////////////////
//功    能：通过RC522和ISO14443卡通讯
//参数说明：Command[IN]:RC522命令字
//          pInData[IN]:通过RC522发送到卡片的数据
//          InLenByte[IN]:发送数据的字节长度
//          pOutData[OUT]:接收到的卡片返回数据
//          *pOutLenBit[OUT]:返回数据的位长度
/////////////////////////////////////////////////////////////////////
char PcdComMF522(unsigned char Command, 		//RC522命令字
                 unsigned char *pInData, 		//通过RC522发送到卡片的数据
                 unsigned char InLenByte,		//发送数据的字节长度
                 unsigned char *pOutData, 		//接收到的卡片返回数据
                 unsigned int  *pOutLenBit)		//返回数据的位长度
{
  char status = MI_ERR;
  unsigned char irqEn   = 0x00;
  unsigned char waitFor = 0x00;
  unsigned char lastBits;
  unsigned char n;
  unsigned int i;
  switch (Command)
  {
  case PCD_AUTHENT:		//Mifare认证
    irqEn   = 0x12;		//允许错误中断请求ErrIEn  允许空闲中断IdleIEn
    waitFor = 0x10;		//认证寻卡等待时候 查询空闲中断标志位
    break;
  case PCD_TRANSCEIVE:		//接收发送 发送接收
    irqEn   = 0x77;		//允许TxIEn RxIEn IdleIEn LoAlertIEn ErrIEn TimerIEn
    waitFor = 0x30;		//寻卡等待时候 查询接收中断标志位与 空闲中断标志位
    break;
  default:
    break;
  }
  
  WriteRawRC(ComIEnReg,irqEn|0x80);		//IRqInv置位管脚IRQ与Status1Reg的IRq位的值相反 
  ClearBitMask(ComIrqReg,0x80);			//Set1该位清零时，CommIRqReg的屏蔽位清零
  WriteRawRC(CommandReg,PCD_IDLE);		//写空闲命令
  SetBitMask(FIFOLevelReg,0x80);			//置位FlushBuffer清除内部FIFO的读和写指针以及ErrReg的BufferOvfl标志位被清除
  
  for (i=0; i<InLenByte; i++)
  {   WriteRawRC(FIFODataReg, pInData[i]);    }		//写数据进FIFOdata
  WriteRawRC(CommandReg, Command);					//写命令
  
  
  if (Command == PCD_TRANSCEIVE)
  {    SetBitMask(BitFramingReg,0x80);  }				//StartSend置位启动数据发送 该位与收发命令使用时才有效
  
  i = 1000;//根据时钟频率调整，操作M1卡最大等待时间25ms
  do 														//认证 与寻卡等待时间	
  {
    n = ReadRawRC(ComIrqReg);							//查询事件中断
    i--;
  }
  while ((i!=0) && !(n&0x01) && !(n&waitFor));		//退出条件i=0,定时器中断，与写空闲命令
  ClearBitMask(BitFramingReg,0x80);					//清理允许StartSend位
  if (i!=0)
  {    
    if(!(ReadRawRC(ErrorReg)&0x1B))			//读错误标志寄存器BufferOfI CollErr ParityErr ProtocolErr
    {
      status = MI_OK;
      if (n & irqEn & 0x01)					//是否发生定时器中断
      {   status = MI_NOTAGERR;   }
      if (Command == PCD_TRANSCEIVE)
      {
        n = ReadRawRC(FIFOLevelReg);			//读FIFO中保存的字节数
        lastBits = ReadRawRC(ControlReg) & 0x07;	//最后接收到得字节的有效位数
        if (lastBits)
        {   *pOutLenBit = (n-1)*8 + lastBits;   }	//N个字节数减去1（最后一个字节）+最后一位的位数 读取到的数据总位数
        else
        {   *pOutLenBit = n*8;   }					//最后接收到的字节整个字节有效
        if (n == 0)									
        {   n = 1;    }
        if (n > MAXRLEN)
        {   n = MAXRLEN;   }
        for (i=0; i<n; i++)
        {   pOutData[i] = ReadRawRC(FIFODataReg);    }
      }
    }
    else
    {   status = MI_ERR;   }
  }
  
  SetBitMask(ControlReg,0x80);           // stop timer now
  WriteRawRC(CommandReg,PCD_IDLE); 
  return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：寻卡
//参数说明: req_code[IN]:寻卡方式
//                0x52 = 寻感应区内所有符合14443A标准的卡
//                0x26 = 寻未进入休眠状态的卡
//          pTagType[OUT]：卡片类型代码
//                0x4400 = Mifare_UltraLight
//                0x0400 = Mifare_One(S50)
//                0x0200 = Mifare_One(S70)
//                0x0800 = Mifare_Pro(X)
//                0x4403 = Mifare_DESFire
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdRequest(unsigned char req_code,unsigned char *pTagType)
{
  char status;  
  uint i;
  unsigned int  unLen;
  unsigned char ucComMF522Buf[MAXRLEN]; 
  
  ClearBitMask(Status2Reg,0x08);	//清理指示MIFARECyptol单元接通以及所有卡的数据通信被加密的情况
  WriteRawRC(BitFramingReg,0x07);	//	发送的最后一个字节的 七位
  SetBitMask(TxControlReg,0x03);	//TX1,TX2管脚的输出信号传递经发送调制的13.56的能量载波信号
  
  ucComMF522Buf[0] = req_code;		//存入 卡片命令字
  
  status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen);	//寻卡    
  if ((status == MI_OK) && (unLen == 0x10))	//寻卡成功返回卡类型 
  {    
    *pTagType     = ucComMF522Buf[0];
    *(pTagType+1) = ucComMF522Buf[1];
  }
  else
  {   
    status = MI_ERR;
  }
  
  return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：防冲撞
//参数说明: pSnr[OUT]:卡片序列号，4字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////  
char PcdAnticoll(unsigned char *pSnr)
{
  char status;
  unsigned char i,snr_check=0;
  unsigned int  unLen;
  unsigned char ucComMF522Buf[MAXRLEN]; 
  
  
  ClearBitMask(Status2Reg,0x08);		//清MFCryptol On位 只有成功执行MFAuthent命令后，该位才能置位
  WriteRawRC(BitFramingReg,0x00);		//清理寄存器 停止收发
  ClearBitMask(CollReg,0x80);			//清ValuesAfterColl所有接收的位在冲突后被清除
  
  // WriteRawRC(BitFramingReg,0x07);	//	发送的最后一个字节的 七位
  // SetBitMask(TxControlReg,0x03);	//TX1,TX2管脚的输出信号传递经发送调制的13.56的能量载波信号
  
  ucComMF522Buf[0] = 0x93;	//卡片防冲突命令
  ucComMF522Buf[1] = 0x20;
  
  status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,2,ucComMF522Buf,&unLen);//与卡片通信
  if (status == MI_OK)		//通信成功
  {
    for (i=0; i<4; i++)
    {   
      *(pSnr+i)  = ucComMF522Buf[i];			//读出UID
      snr_check ^= ucComMF522Buf[i];
      
    }
    if (snr_check != ucComMF522Buf[i])
    {   status = MI_ERR;    }
  }
  
  SetBitMask(CollReg,0x80);
  return status;
}
/////////////////////////////////////////////////////////////////////
//用MF522计算CRC16函数
/////////////////////////////////////////////////////////////////////
void CalulateCRC(unsigned char *pIndata,unsigned char len,unsigned char *pOutData)
{
  unsigned char i,n;
  ClearBitMask(DivIrqReg,0x04);
  WriteRawRC(CommandReg,PCD_IDLE);
  SetBitMask(FIFOLevelReg,0x80);
  for (i=0; i<len; i++)
  {   WriteRawRC(FIFODataReg, *(pIndata+i));   }
  WriteRawRC(CommandReg, PCD_CALCCRC);
  i = 0xFF;
  do 
  {
    n = ReadRawRC(DivIrqReg);
    i--;
  }
  while ((i!=0) && !(n&0x04));
  pOutData[0] = ReadRawRC(CRCResultRegL);
  pOutData[1] = ReadRawRC(CRCResultRegM);
}
/////////////////////////////////////////////////////////////////////
//功    能：选定卡片
//参数说明: pSnr[IN]:卡片序列号，4字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdSelect(unsigned char *pSnr)
{
  char status;
  unsigned char i;
  unsigned int  unLen;
  unsigned char ucComMF522Buf[MAXRLEN]; 
  
  ucComMF522Buf[0] = PICC_ANTICOLL1;
  ucComMF522Buf[1] = 0x70;
  ucComMF522Buf[6] = 0;
  for (i=0; i<4; i++)
  {
    ucComMF522Buf[i+2] = *(pSnr+i);
    ucComMF522Buf[6]  ^= *(pSnr+i);
  }
  CalulateCRC(ucComMF522Buf,7,&ucComMF522Buf[7]);
  
  ClearBitMask(Status2Reg,0x08);
  
  status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,9,ucComMF522Buf,&unLen);
  
  if ((status == MI_OK) && (unLen == 0x18))
  {   status = MI_OK;  }
  else
  {   status = MI_ERR;    }
  
  return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：验证卡片密码
//参数说明: auth_mode[IN]: 密码验证模式
//                 0x60 = 验证A密钥
//                 0x61 = 验证B密钥 
//          addr[IN]：块地址
//          pKey[IN]：密码
//          pSnr[IN]：卡片序列号，4字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////               
char PcdAuthState(unsigned char auth_mode,unsigned char addr,unsigned char *pKey,unsigned char *pSnr)
{
  char status;
  unsigned int  unLen;
  unsigned char i,ucComMF522Buf[MAXRLEN]; 
  
  ucComMF522Buf[0] = auth_mode;
  ucComMF522Buf[1] = addr;
  for (i=0; i<6; i++)
  {    ucComMF522Buf[i+2] = *(pKey+i);   }
  for (i=0; i<6; i++)
  {    ucComMF522Buf[i+8] = *(pSnr+i);   }
  //   memcpy(&ucComMF522Buf[2], pKey, 6); 
  //   memcpy(&ucComMF522Buf[8], pSnr, 4); 
  
  status = PcdComMF522(PCD_AUTHENT,ucComMF522Buf,12,ucComMF522Buf,&unLen);
  if ((status != MI_OK) || (!(ReadRawRC(Status2Reg) & 0x08)))
  {   status = MI_ERR;   }
  
  return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：写数据到M1卡一块
//参数说明: addr[IN]：块地址
//          pData[IN]：写入的数据，16字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////                  
char PcdWrite(unsigned char addr,unsigned char *pData)
{
  char status;
  unsigned int  unLen;
  unsigned char i,ucComMF522Buf[MAXRLEN]; 
  
  ucComMF522Buf[0] = PICC_WRITE;
  ucComMF522Buf[1] = addr;
  CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
  
  status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);
  
  if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
  {   status = MI_ERR;   }
  
  if (status == MI_OK)
  {
    //memcpy(ucComMF522Buf, pData, 16);
    for (i=0; i<16; i++)
    {    ucComMF522Buf[i] = *(pData+i);   }
    CalulateCRC(ucComMF522Buf,16,&ucComMF522Buf[16]);
    
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,18,ucComMF522Buf,&unLen);
    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }
  } 
  return status;
}
/////////////////////////////////////////////////////////////////////
//功    能：读取M1卡一块数据
//参数说明: addr[IN]：块地址
//          pData[OUT]：读出的数据，16字节
//返    回: 成功返回MI_OK
///////////////////////////////////////////////////////////////////// 
char PcdRead(unsigned char addr,unsigned char *pData)
{
  char status;
  unsigned int  unLen;
  unsigned char i,ucComMF522Buf[MAXRLEN]; 
  
  ucComMF522Buf[0] = PICC_READ;
  ucComMF522Buf[1] = addr;
  CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
  
  status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);
  if ((status == MI_OK) && (unLen == 0x90))
    //   {   memcpy(pData, ucComMF522Buf, 16);   }
  {
    for (i=0; i<16; i++)
    {    *(pData+i) = ucComMF522Buf[i];   }
  }
  else
  {   status = MI_ERR;   }
  
  return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：命令卡片进入休眠状态
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdHalt(void)
{
  //    char status;
  unsigned int  unLen;
  unsigned char ucComMF522Buf[MAXRLEN]; 
  
  ucComMF522Buf[0] = PICC_HALT;
  ucComMF522Buf[1] = 0;
  CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
  PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);
  // status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);
  
  return MI_OK;
}


void IC_CMT(uchar *UID,uchar *KEY,uchar RW,char *Dat)
{
  uchar status = 0xab;
  uchar TagType[16]={0};//IC卡的类型
  uchar IC_uid[16]={0};//IC卡的UID
  
  UartSend(PcdRequest(0x52,TagType));//寻卡
  UartSend(PcdAnticoll(IC_uid));//防冲撞
  
  UartSend(PcdSelect(UID));//选定卡
  
  UartSend(PcdAuthState(0x60,0x10,KEY,UID));//校验
  if(RW)//读写选择，1是读，0是写
  {
    UartSend (PcdRead(0x10,Dat));
  }
  else 
  {
    UartSend(PcdWrite(0x10,Dat));
  } 
  UartSend(PcdHalt());
}
