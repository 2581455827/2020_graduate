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
char PcdComMF522(unsigned char Command, 		//RC522������
                 unsigned char *pInData, 		//ͨ��RC522���͵���Ƭ������
                 unsigned char InLenByte,		//�������ݵ��ֽڳ���
                 unsigned char *pOutData, 		//���յ��Ŀ�Ƭ��������
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
//��    �ܣ���RC632�Ĵ���
//����˵����Address[IN]:�Ĵ�����ַ
//��    �أ�������ֵ
/////////////////////////////////////////////////////////////////////
unsigned char ReadRawRC(unsigned char Address)
{
  unsigned char ucAddr;
  unsigned char ucResult=0;
  IC_CS = 0;
  ucAddr = ((Address<<1)&0x7E)|0x80;//��ַ�任��SPI�Ķ�д��ַ��Ҫ��
  SPIWriteByte(ucAddr);
  ucResult=SPIReadByte();
  IC_CS = 1;
  return ucResult;
}
/////////////////////////////////////////////////////////////////////
//��    �ܣ�дRC632�Ĵ���
//����˵����Address[IN]:�Ĵ�����ַ
//          value[IN]:д���ֵ
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
//��    �ܣ���RC522�Ĵ���λ
//����˵����reg[IN]:�Ĵ�����ַ
//          mask[IN]:��λֵ
/////////////////////////////////////////////////////////////////////
void SetBitMask(unsigned char reg,unsigned char mask)  
{
  char tmp = 0x0;
  tmp = ReadRawRC(reg);
  WriteRawRC(reg,tmp | mask);  // set bit mask
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ���RC522�Ĵ���λ
//����˵����reg[IN]:�Ĵ�����ַ
//          mask[IN]:��λֵ
/////////////////////////////////////////////////////////////////////
void ClearBitMask(unsigned char reg,unsigned char mask)  
{
  char tmp = 0x0;
  tmp = ReadRawRC(reg);
  WriteRawRC(reg, tmp & ~mask);  // clear bit mask
} 




/////////////////////////////////////////////////////////////////////
//��������  
//ÿ��������ر����շ���֮��Ӧ������1ms�ļ��
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
//�ر�����
/////////////////////////////////////////////////////////////////////
void PcdAntennaOff(void)
{
  ClearBitMask(TxControlReg, 0x03);
}


/////////////////////////////////////////////////////////////////////
//��    �ܣ���λRC522
//��    ��: �ɹ�����MI_OK
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
  //while(ReadRawRC(0x01)&0x10);   //��鿨Ƭ
  Delay_I_1us(1);
  
  WriteRawRC(ModeReg,0x3D);             //���巢�ͺͽ��ճ���ģʽ ��Mifare��ͨѶ��CRC��ʼֵ0x6363
  WriteRawRC(TReloadRegL,30);           //16λ��ʱ����λ
  WriteRawRC(TReloadRegH,0);		//16λ��ʱ����λ
  WriteRawRC(TModeReg,0x8D);		//�����ڲ���ʱ��������
  WriteRawRC(TPrescalerReg,0x3E);	//���ö�ʱ����Ƶϵ��
  WriteRawRC(TxAutoReg,0x40);		//	���Ʒ����ź�Ϊ100%ASK

  //return MI_OK;
}


//////////////////////////////////////////////////////////////////////
//����RC632�Ĺ�����ʽ 
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
    PcdAntennaOn();//������
  }
  //  else return (-1); 
  
  //return MI_OK;
}

/////////////////////////////////////////////////////////////////////
//��    �ܣ�ͨ��RC522��ISO14443��ͨѶ
//����˵����Command[IN]:RC522������
//          pInData[IN]:ͨ��RC522���͵���Ƭ������
//          InLenByte[IN]:�������ݵ��ֽڳ���
//          pOutData[OUT]:���յ��Ŀ�Ƭ��������
//          *pOutLenBit[OUT]:�������ݵ�λ����
/////////////////////////////////////////////////////////////////////
char PcdComMF522(unsigned char Command, 		//RC522������
                 unsigned char *pInData, 		//ͨ��RC522���͵���Ƭ������
                 unsigned char InLenByte,		//�������ݵ��ֽڳ���
                 unsigned char *pOutData, 		//���յ��Ŀ�Ƭ��������
                 unsigned int  *pOutLenBit)		//�������ݵ�λ����
{
  char status = MI_ERR;
  unsigned char irqEn   = 0x00;
  unsigned char waitFor = 0x00;
  unsigned char lastBits;
  unsigned char n;
  unsigned int i;
  switch (Command)
  {
  case PCD_AUTHENT:		//Mifare��֤
    irqEn   = 0x12;		//��������ж�����ErrIEn  ��������ж�IdleIEn
    waitFor = 0x10;		//��֤Ѱ���ȴ�ʱ�� ��ѯ�����жϱ�־λ
    break;
  case PCD_TRANSCEIVE:		//���շ��� ���ͽ���
    irqEn   = 0x77;		//����TxIEn RxIEn IdleIEn LoAlertIEn ErrIEn TimerIEn
    waitFor = 0x30;		//Ѱ���ȴ�ʱ�� ��ѯ�����жϱ�־λ�� �����жϱ�־λ
    break;
  default:
    break;
  }
  
  WriteRawRC(ComIEnReg,irqEn|0x80);		//IRqInv��λ�ܽ�IRQ��Status1Reg��IRqλ��ֵ�෴ 
  ClearBitMask(ComIrqReg,0x80);			//Set1��λ����ʱ��CommIRqReg������λ����
  WriteRawRC(CommandReg,PCD_IDLE);		//д��������
  SetBitMask(FIFOLevelReg,0x80);			//��λFlushBuffer����ڲ�FIFO�Ķ���дָ���Լ�ErrReg��BufferOvfl��־λ�����
  
  for (i=0; i<InLenByte; i++)
  {   WriteRawRC(FIFODataReg, pInData[i]);    }		//д���ݽ�FIFOdata
  WriteRawRC(CommandReg, Command);					//д����
  
  
  if (Command == PCD_TRANSCEIVE)
  {    SetBitMask(BitFramingReg,0x80);  }				//StartSend��λ�������ݷ��� ��λ���շ�����ʹ��ʱ����Ч
  
  i = 1000;//����ʱ��Ƶ�ʵ���������M1�����ȴ�ʱ��25ms
  do 														//��֤ ��Ѱ���ȴ�ʱ��	
  {
    n = ReadRawRC(ComIrqReg);							//��ѯ�¼��ж�
    i--;
  }
  while ((i!=0) && !(n&0x01) && !(n&waitFor));		//�˳�����i=0,��ʱ���жϣ���д��������
  ClearBitMask(BitFramingReg,0x80);					//��������StartSendλ
  if (i!=0)
  {    
    if(!(ReadRawRC(ErrorReg)&0x1B))			//�������־�Ĵ���BufferOfI CollErr ParityErr ProtocolErr
    {
      status = MI_OK;
      if (n & irqEn & 0x01)					//�Ƿ�����ʱ���ж�
      {   status = MI_NOTAGERR;   }
      if (Command == PCD_TRANSCEIVE)
      {
        n = ReadRawRC(FIFOLevelReg);			//��FIFO�б�����ֽ���
        lastBits = ReadRawRC(ControlReg) & 0x07;	//�����յ����ֽڵ���Чλ��
        if (lastBits)
        {   *pOutLenBit = (n-1)*8 + lastBits;   }	//N���ֽ�����ȥ1�����һ���ֽڣ�+���һλ��λ�� ��ȡ����������λ��
        else
        {   *pOutLenBit = n*8;   }					//�����յ����ֽ������ֽ���Ч
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
//��    �ܣ�Ѱ��
//����˵��: req_code[IN]:Ѱ����ʽ
//                0x52 = Ѱ��Ӧ�������з���14443A��׼�Ŀ�
//                0x26 = Ѱδ��������״̬�Ŀ�
//          pTagType[OUT]����Ƭ���ʹ���
//                0x4400 = Mifare_UltraLight
//                0x0400 = Mifare_One(S50)
//                0x0200 = Mifare_One(S70)
//                0x0800 = Mifare_Pro(X)
//                0x4403 = Mifare_DESFire
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////
char PcdRequest(unsigned char req_code,unsigned char *pTagType)
{
  char status;  
  uint i;
  unsigned int  unLen;
  unsigned char ucComMF522Buf[MAXRLEN]; 
  
  ClearBitMask(Status2Reg,0x08);	//����ָʾMIFARECyptol��Ԫ��ͨ�Լ����п�������ͨ�ű����ܵ����
  WriteRawRC(BitFramingReg,0x07);	//	���͵����һ���ֽڵ� ��λ
  SetBitMask(TxControlReg,0x03);	//TX1,TX2�ܽŵ�����źŴ��ݾ����͵��Ƶ�13.56�������ز��ź�
  
  ucComMF522Buf[0] = req_code;		//���� ��Ƭ������
  
  status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen);	//Ѱ��    
  if ((status == MI_OK) && (unLen == 0x10))	//Ѱ���ɹ����ؿ����� 
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
//��    �ܣ�����ײ
//����˵��: pSnr[OUT]:��Ƭ���кţ�4�ֽ�
//��    ��: �ɹ�����MI_OK
/////////////////////////////////////////////////////////////////////  
char PcdAnticoll(unsigned char *pSnr)
{
  char status;
  unsigned char i,snr_check=0;
  unsigned int  unLen;
  unsigned char ucComMF522Buf[MAXRLEN]; 
  
  
  ClearBitMask(Status2Reg,0x08);		//��MFCryptol Onλ ֻ�гɹ�ִ��MFAuthent����󣬸�λ������λ
  WriteRawRC(BitFramingReg,0x00);		//����Ĵ��� ֹͣ�շ�
  ClearBitMask(CollReg,0x80);			//��ValuesAfterColl���н��յ�λ�ڳ�ͻ�����
  
  // WriteRawRC(BitFramingReg,0x07);	//	���͵����һ���ֽڵ� ��λ
  // SetBitMask(TxControlReg,0x03);	//TX1,TX2�ܽŵ�����źŴ��ݾ����͵��Ƶ�13.56�������ز��ź�
  
  ucComMF522Buf[0] = 0x93;	//��Ƭ����ͻ����
  ucComMF522Buf[1] = 0x20;
  
  status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,2,ucComMF522Buf,&unLen);//�뿨Ƭͨ��
  if (status == MI_OK)		//ͨ�ųɹ�
  {
    for (i=0; i<4; i++)
    {   
      *(pSnr+i)  = ucComMF522Buf[i];			//����UID
      snr_check ^= ucComMF522Buf[i];
      
    }
    if (snr_check != ucComMF522Buf[i])
    {   status = MI_ERR;    }
  }
  
  SetBitMask(CollReg,0x80);
  return status;
}
/////////////////////////////////////////////////////////////////////
//��MF522����CRC16����
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
//��    �ܣ�ѡ����Ƭ
//����˵��: pSnr[IN]:��Ƭ���кţ�4�ֽ�
//��    ��: �ɹ�����MI_OK
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
//��    �ܣ���֤��Ƭ����
//����˵��: auth_mode[IN]: ������֤ģʽ
//                 0x60 = ��֤A��Կ
//                 0x61 = ��֤B��Կ 
//          addr[IN]�����ַ
//          pKey[IN]������
//          pSnr[IN]����Ƭ���кţ�4�ֽ�
//��    ��: �ɹ�����MI_OK
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
//��    �ܣ�д���ݵ�M1��һ��
//����˵��: addr[IN]�����ַ
//          pData[IN]��д������ݣ�16�ֽ�
//��    ��: �ɹ�����MI_OK
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
//��    �ܣ���ȡM1��һ������
//����˵��: addr[IN]�����ַ
//          pData[OUT]�����������ݣ�16�ֽ�
//��    ��: �ɹ�����MI_OK
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
//��    �ܣ����Ƭ��������״̬
//��    ��: �ɹ�����MI_OK
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
  uchar TagType[16]={0};//IC��������
  uchar IC_uid[16]={0};//IC����UID
  
  UartSend(PcdRequest(0x52,TagType));//Ѱ��
  UartSend(PcdAnticoll(IC_uid));//����ײ
  
  UartSend(PcdSelect(UID));//ѡ����
  
  UartSend(PcdAuthState(0x60,0x10,KEY,UID));//У��
  if(RW)//��дѡ��1�Ƕ���0��д
  {
    UartSend (PcdRead(0x10,Dat));
  }
  else 
  {
    UartSend(PcdWrite(0x10,Dat));
  } 
  UartSend(PcdHalt());
}
