/*****************************************************************
*本程序是通过F340的uart0与PC进行串口通讯的例程，具体实现为当F340读取光传感器模块数据后
*发给PC机 ,端口为RX＝P0.5，TX＝P0.4
******************************************************************/
#include <c8051f340.h>
#include  <INTRINS.H>
#define uchar unsigned char
#define uint unsigned int

typedef   unsigned char BYTE;
typedef   unsigned short WORD;

#define	  SlaveAddress   0x46 
uchar  BUF[5];
uchar   ge,shi,bai,qian,wan;            //显示变量
//****************************************
/************************************************************************************
// 常量及全局变量定义
*************************************************************************************/
#define  IIC_WRITE      0                 // WRITE direction bit
#define  IIC_READ       1                 // READ direction bit

sbit	  SCL=P1^0;      //IIC时钟引脚定义
sbit  	  SDA=P1^1;      //IIC数据引脚定义



//*********************************************************
void conversion(uint temp_data)  //  数据转换出 个，十，百，千，万
{  
    wan=temp_data/10000+0x30 ;
    temp_data=temp_data%10000;   //取余运算
	qian=temp_data/1000+0x30 ;
    temp_data=temp_data%1000;    //取余运算
    bai=temp_data/100+0x30   ;
    temp_data=temp_data%100;     //取余运算
    shi=temp_data/10+0x30    ;
    temp_data=temp_data%10;      //取余运算
    ge=temp_data+0x30; 	
}
/***********************************************************************************
* Function: Delay_us;
*
* Description: 延时程序, 延时时间范围: 0~65535us;
*
* Input:  times, 延时时间变量;
*
* Output: none;
*
* Return: none;
*
* Note:   延时时间最大是65535us;
************************************************************************************/
void Delay_us(unsigned int times)
{
    unsigned int i;

	for (i=0; i<times; i++)
	{
		_nop_();	// 调用NOP,延时1us
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();
		_nop_();
        
	    _nop_();	
		_nop_();
	}
}


void Delay_ms(unsigned int times)
{
    unsigned int i;
	
	for (i=0; i<times; i++)	
		Delay_us(1000); 	// 调用延时函数,延时1ms		
}

void Delay_s(unsigned int times)
{
	unsigned int i;
	
	for (i=0; i<times; i++)
		Delay_ms(1000);	   // 调用延时函数,延时1s
}


void Delay5us()
{
    _nop_();_nop_();_nop_();_nop_();
    _nop_();_nop_();_nop_();_nop_();
	_nop_();_nop_();_nop_();_nop_();
	_nop_();_nop_();_nop_();_nop_();
}

void Delay5ms()
{
    int n = 560;

    while (n--);
}
/**************************************
起始信号
**************************************/
void BH1750_Start()
{
    SDA = 1;                    //拉高数据线
    SCL = 1;                    //拉高时钟线
    Delay5us();                 //延时
    SDA = 0;                    //产生下降沿
    Delay5us();                 //延时
    SCL = 0;                    //拉低时钟线
}

/**************************************
停止信号
**************************************/
void BH1750_Stop()
{
    SDA = 0;                    //拉低数据线
    SCL = 1;                    //拉高时钟线
    Delay5us();                 //延时
    SDA = 1;                    //产生上升沿
    Delay5us();                 //延时
}

/**************************************
发送应答信号
入口参数:ack (0:ACK 1:NAK)
**************************************/
void BH1750_SendACK(bit ack)
{
    SDA = ack;                  //写应答信号
    SCL = 1;                    //拉高时钟线
    Delay5us();                 //延时
    SCL = 0;                    //拉低时钟线
    Delay5us();                 //延时
}

/**************************************
接收应答信号
**************************************/
bit BH1750_RecvACK()
{
    SCL = 1;                    //拉高时钟线
    Delay5us();                 //延时
    CY = SDA;                   //读应答信号
    SCL = 0;                    //拉低时钟线
    Delay5us();                 //延时

    return CY;
}

/**************************************
向IIC总线发送一个字节数据
**************************************/
void BH1750_SendByte(uchar dat)
{
    uchar i;

    for (i=0; i<8; i++)         //8位计数器
    {
        dat <<= 1;              //移出数据的最高位
        SDA = CY;               //送数据口
        SCL = 1;                //拉高时钟线
        Delay5us();             //延时
        SCL = 0;                //拉低时钟线
        Delay5us();             //延时
    }
    BH1750_RecvACK();
}

/**************************************
从IIC总线接收一个字节数据
**************************************/
uchar BH1750_RecvByte()
{
    BYTE i;
    BYTE dat = 0;

    SDA = 1;                    //使能内部上拉,准备读取数据,
    for (i=0; i<8; i++)         //8位计数器
    {
        dat <<= 1;
        SCL = 1;                //拉高时钟线
        Delay5us();             //延时
        dat |= SDA;             //读数据               
        SCL = 0;                //拉低时钟线
        Delay5us();             //延时
    }
    return dat;
}

//*********************************

void Single_Write_BH1750(uchar REG_Address)
{
    BH1750_Start();                  //起始信号
    BH1750_SendByte(SlaveAddress);   //发送设备地址+写信号
    BH1750_SendByte(REG_Address);    //内部寄存器地址，请参考中文pdf22页 
  //  BH1750_SendByte(REG_data);       //内部寄存器数据，请参考中文pdf22页 
    BH1750_Stop();                   //发送停止信号
}


//*********************************************************
//
//连续读出BH1750内部数据
//
//*********************************************************
void Multiple_read_BH1750(void)
{   uchar i;	
    BH1750_Start();                          //起始信号
    BH1750_SendByte(SlaveAddress+1);         //发送设备地址+读信号
	
	 for (i=0; i<3; i++)                      //连续读取6个地址数据，存储中BUF
    {
        BUF[i] = BH1750_RecvByte();          //BUF[0]存储0x32地址中的数据
        if (i == 3)
        {

           BH1750_SendACK(1);                //最后一个数据需要回NOACK
        }
        else
        {		
          BH1750_SendACK(0);                //回应ACK
       }
   }

    BH1750_Stop();                          //停止信号
    Delay5ms();
}


//****************************************




/*****************************************************************
*端口初始化函数
*****************************************************************/
void PORT_Init (void)
{
 XBR0      = 0x01;//端口I/O交叉开关寄存器0，UART TX0, RX0 连到端口引脚 P0.4 和 P0.5                                          
 XBR1      = 0x40;//端口I/O交叉开关寄存器1,交叉开关使能                   
 P0MDOUT   = 0x10;//P0.4为推挽输出，其他的为漏极开路输出
}
/****************************************************************
*UART0初始化函数
*****************************************************************/
void UART0_Init (void)
{
 SCON0    |= 0x10;                                                            
 CKCON     = 0x01;
 TH1       = 0x64;   //波特率为9600
 TL1       = TH1;                         
 TMOD      = 0x20;                     
 TR1       = 1;//P235，定时器1运行控制，定时器1允许                           
 TI0       = 1;//P235，中断1类型选择，INT1为边沿触发                           
}

void tx_data(uchar g)  //发送8位数据
 {
   SBUF0=g;     
   while(TI0==0);  //发送
   TI0=0;
}

/*****************************************************************
*主函数
*****************************************************************/
void main()
{   
   uint  dis_data;
   float temp;
 PCA0MD    &= ~0x40;//关闭看门狗                  
 OSCICN    |= 0x03;//P126                     
 PORT_Init();//端口初始化                      
 UART0_Init();//UART0初始化

 while(1)
  {
 
    Single_Write_BH1750(0x01);   // power on
    Single_Write_BH1750(0x10);   // H- resolution mode
    Delay_ms(180);               //延时180ms
    Multiple_Read_BH1750();      //连续读出数据，存储在BUF中  
	dis_data=BUF[0];
    dis_data=(dis_data<<8)+BUF[1];//合成数据 

	temp=(float)dis_data/1.2;
	conversion(temp);         //计算数据和显示
	tx_data('L'); 
	tx_data('i'); 
	tx_data('g'); 
	tx_data('h'); 
	tx_data('t'); 
    tx_data(':'); 

    tx_data(wan); //显示数据
    tx_data(qian);  
    tx_data(bai); 
    tx_data(shi); 
	tx_data(ge); 

	tx_data('l'); //显示数单位
	tx_data('x');  

	tx_data(0x0d);
    tx_data(0x0a);

  }
}
 