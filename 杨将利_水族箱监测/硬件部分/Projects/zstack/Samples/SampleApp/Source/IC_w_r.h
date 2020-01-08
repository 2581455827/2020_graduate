#ifndef __IC_W_R_H__
#define __IC_W_R_H__

extern void PcdReset(void);//复位
extern void M500PcdConfigISOType(unsigned char type);//工作方式
extern char PcdRequest(unsigned char req_code,unsigned char *pTagType);//寻卡
extern char PcdAnticoll(unsigned char *pSnr);//读卡号

#endif 