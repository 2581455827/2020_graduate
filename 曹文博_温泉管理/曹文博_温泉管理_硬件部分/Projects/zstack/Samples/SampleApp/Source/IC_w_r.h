#ifndef __IC_W_R_H__
#define __IC_W_R_H__

extern void PcdReset(void);//��λ
extern void M500PcdConfigISOType(unsigned char type);//������ʽ
extern char PcdRequest(unsigned char req_code,unsigned char *pTagType);//Ѱ��
extern char PcdAnticoll(unsigned char *pSnr);//������

#endif 