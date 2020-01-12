	.module AVR_GY-30.c
	.area text(rom, con, rel)
	.dbfile D:\MCU_Project\MCU_AVR\AVR_GY-30\AVR_GY-30.c
	.dbfile D:\MCU_Project\MCU_AVR\AVR_GY-30\delay.h
	.dbfunc e delay_1us _delay_1us fV
	.even
_delay_1us::
	.dbline -1
	.dbline 15
; /*-----------------------------------------------------------------------
; ��ʱ����
; ��������ICC-AVR7.14
; Ŀ��оƬ : M16
; ʱ��: 11.0592Mhz
; -----------------------------------------------------------------------*/
; #ifndef __delay_h
; #define __delay_h
; void delay_nus(unsigned int n);
; void delay_nms(unsigned int n);
; void delay_1us(void);
; void delay_1ms(void) ; 
; 
; void delay_1us(void)                 //1us��ʱ����
;   {
	.dbline 16
;    asm("nop");
	nop
	.dbline -2
L1:
	.dbline 0 ; func end
	ret
	.dbend
	.dbfunc e delay_nus _delay_nus fV
;              i -> R20,R21
;              n -> R10,R11
	.even
_delay_nus::
	xcall push_xgset300C
	movw R10,R16
	.dbline -1
	.dbline 20
;   }
; 
; void delay_nus(unsigned int n)       //N us��ʱ����
;   {
	.dbline 21
;    unsigned int i=0;
	clr R20
	clr R21
	.dbline 22
;    for (i=0;i<n;i++)
	xjmp L6
L3:
	.dbline 23
;    delay_1us();
	xcall _delay_1us
L4:
	.dbline 22
	subi R20,255  ; offset = 1
	sbci R21,255
L6:
	.dbline 22
	cp R20,R10
	cpc R21,R11
	brlo L3
X0:
	.dbline -2
L2:
	.dbline 0 ; func end
	xjmp pop_xgset300C
	.dbsym r i 20 i
	.dbsym r n 10 i
	.dbend
	.dbfunc e delay_1ms _delay_1ms fV
;              i -> R16,R17
	.even
_delay_1ms::
	.dbline -1
	.dbline 27
;   }
;   
; void delay_1ms(void)                 //1ms��ʱ����
;   {
	.dbline 29
;    unsigned int i;
;    for (i=0;i<1500;i++);
	clr R16
	clr R17
	xjmp L11
L8:
	.dbline 29
L9:
	.dbline 29
	subi R16,255  ; offset = 1
	sbci R17,255
L11:
	.dbline 29
	cpi R16,220
	ldi R30,5
	cpc R17,R30
	brlo L8
X1:
	.dbline -2
L7:
	.dbline 0 ; func end
	ret
	.dbsym r i 16 i
	.dbend
	.dbfunc e delay_nms _delay_nms fV
;              i -> R20,R21
;              n -> R10,R11
	.even
_delay_nms::
	xcall push_xgset300C
	movw R10,R16
	.dbline -1
	.dbline 33
;   }
;   
; void delay_nms(unsigned int n)       //N ms��ʱ����
;   {
	.dbline 34
;    unsigned int i=0;
	clr R20
	clr R21
	.dbline 35
;    for (i=0;i<n;i++)
	xjmp L16
L13:
	.dbline 36
;    delay_1ms();
	xcall _delay_1ms
L14:
	.dbline 35
	subi R20,255  ; offset = 1
	sbci R21,255
L16:
	.dbline 35
	cp R20,R10
	cpc R21,R11
	brlo L13
X2:
	.dbline -2
L12:
	.dbline 0 ; func end
	xjmp pop_xgset300C
	.dbsym r i 20 i
	.dbsym r n 10 i
	.dbend
	.dbfile D:\MCU_Project\MCU_AVR\AVR_GY-30\I2C.h
	.dbfunc e I2C_Write _I2C_Write fc
;          Wdata -> R16
	.even
_I2C_Write::
	.dbline -1
	.dbline 41
; #include <macros.h>
; #include "delay.h"
; 
; //ʹ��AVR�ڲ�Ӳ��iic�����Ŷ���
; //PC0->SCL  ;  PC1->SDA
; //I2C ״̬����
; //MT ����ʽ���� MR ����ʽ����
; #define START			0x08
; #define RE_START		0x10
; #define MT_SLA_ACK		0x18
; #define MT_SLA_NOACK 	0x20
; #define MT_DATA_ACK		0x28
; #define MT_DATA_NOACK	0x30
; #define MR_SLA_ACK		0x40
; #define MR_SLA_NOACK	0x48
; #define MR_DATA_ACK		0x50
; #define MR_DATA_NOACK	0x58		
; 
; #define RD_DEVICE_ADDR  0x47   //ADDR�Žӵ�ʱ�Ķ���ַ
; #define WD_DEVICE_ADDR  0x46   //ADDR�Žӵ�ʱ��д��ַ
; 
; //����TWI����(��ģʽд�Ͷ�)
; #define Start()			(TWCR=(1<<TWINT)|(1<<TWSTA)|(1<<TWEN))		//����I2C
; #define Stop()			(TWCR=(1<<TWINT)|(1<<TWSTO)|(1<<TWEN))		//ֹͣI2C
; #define Wait()			{while(!(TWCR&(1<<TWINT)));}				//�ȴ��жϷ���
; #define TestAck()		(TWSR&0xf8)									//�۲췵��״̬
; #define SetAck			(TWCR|=(1<<TWEA))							//����ACKӦ��
; #define SetNoAck		(TWCR&=~(1<<TWEA))							//����Not AckӦ��
; #define Twi()			(TWCR=(1<<TWINT)|(1<<TWEN))				    //����I2C
; #define Write8Bit(x)	{TWDR=(x);TWCR=(1<<TWINT)|(1<<TWEN);}		//д���ݵ�TWDR
; 
; unsigned char I2C_Write(unsigned char Wdata);
; unsigned int I2C_Read();
; 
; /*********************************************
; I2C����дһ���ֽ�
; ����0:д�ɹ�
; ����1:дʧ��
; **********************************************/
; unsigned char I2C_Write(unsigned char Wdata)
; {
	.dbline 42
; 	  Start();						//I2C����
	ldi R24,164
	out 0x36,R24
	.dbline 43
; 	  Wait();
L18:
	.dbline 43
L19:
	.dbline 43
	in R2,0x36
	sbrs R2,7
	rjmp L18
X3:
	.dbline 43
	.dbline 43
	.dbline 44
; 	  if(TestAck()!=START) 
	in R24,0x1
	andi R24,248
	cpi R24,8
	breq L21
X4:
	.dbline 45
; 	  return 1;					//ACK
	ldi R16,1
	xjmp L17
L21:
	.dbline 47
; 	  
; 	  Write8Bit(WD_DEVICE_ADDR);	//дI2C��������ַ��д��ʽ
	.dbline 47
	ldi R24,70
	out 0x3,R24
	.dbline 47
	ldi R24,132
	out 0x36,R24
	.dbline 47
	.dbline 47
	.dbline 48
; 	  Wait();
L23:
	.dbline 48
L24:
	.dbline 48
	in R2,0x36
	sbrs R2,7
	rjmp L23
X5:
	.dbline 48
	.dbline 48
	.dbline 49
; 	  if(TestAck()!=MT_SLA_ACK) 
	in R24,0x1
	andi R24,248
	cpi R24,24
	breq L26
X6:
	.dbline 50
; 	  return 1;					//ACK  
	ldi R16,1
	xjmp L17
L26:
	.dbline 52
; 	  
; 	  Write8Bit(Wdata);			 	//д���ݵ�������Ӧ�Ĵ���
	.dbline 52
	out 0x3,R16
	.dbline 52
	ldi R24,132
	out 0x36,R24
	.dbline 52
	.dbline 52
	.dbline 53
; 	  Wait();
L28:
	.dbline 53
L29:
	.dbline 53
	in R2,0x36
	sbrs R2,7
	rjmp L28
X7:
	.dbline 53
	.dbline 53
	.dbline 54
; 	  if(TestAck()!=MT_DATA_ACK) 
	in R24,0x1
	andi R24,248
	cpi R24,40
	breq L31
X8:
	.dbline 55
; 	  return 1;				    //ACK	 
	ldi R16,1
	xjmp L17
L31:
	.dbline 56
; 	  Stop();  						//I2Cֹͣ 
	ldi R24,148
	out 0x36,R24
	.dbline 57
; 	  return 0;
	clr R16
	.dbline -2
L17:
	.dbline 0 ; func end
	ret
	.dbsym r Wdata 16 c
	.dbend
	.dbfunc e I2C_Read _I2C_Read fi
;           temp -> R16,R17
	.even
_I2C_Read::
	.dbline -1
	.dbline 65
; }
; 
; /*********************************************
; I2C���߶�һ���ֽ�
; ���أ�16λ��ֵ
; **********************************************/
; unsigned int I2C_Read()
; {
	.dbline 68
;    unsigned int temp;
;    
; 	  Start();						//I2C����
	ldi R24,164
	out 0x36,R24
	.dbline 69
; 	  Wait();
L34:
	.dbline 69
L35:
	.dbline 69
	in R2,0x36
	sbrs R2,7
	rjmp L34
X9:
	.dbline 69
	.dbline 69
	.dbline 70
; 	  if(TestAck()!=START) 
	in R24,0x1
	andi R24,248
	cpi R24,8
	breq L37
X10:
	.dbline 71
; 	  return 1;					   //ACK  
	ldi R16,1
	ldi R17,0
	xjmp L33
L37:
	.dbline 73
;    
;       Write8Bit(RD_DEVICE_ADDR);   //дI2C��������ַ��д��ʽ
	.dbline 73
	ldi R24,71
	out 0x3,R24
	.dbline 73
	ldi R24,132
	out 0x36,R24
	.dbline 73
	.dbline 73
	.dbline 74
; 	  Wait();
L39:
	.dbline 74
L40:
	.dbline 74
	in R2,0x36
	sbrs R2,7
	rjmp L39
X11:
	.dbline 74
	.dbline 74
	.dbline 75
; 	  if(TestAck()!=MR_SLA_ACK) 
	in R24,0x1
	andi R24,248
	cpi R24,64
	breq L42
X12:
	.dbline 76
; 	  return 1;					   //ACK
	ldi R16,1
	ldi R17,0
	xjmp L33
L42:
	.dbline 78
; 	  
;       Twi();                       //������I2C����ʽ
	ldi R24,132
	out 0x36,R24
	.dbline 79
; 	  TWCR = 0xC4;                 //���жϱ�־�����Ӧ��ACK
	ldi R24,196
	out 0x36,R24
	.dbline 80
; 	  Wait();     
L44:
	.dbline 80
L45:
	.dbline 80
	in R2,0x36
	sbrs R2,7
	rjmp L44
X13:
	.dbline 80
	.dbline 80
	.dbline 81
; 	  temp=TWDR;                   //��ȡI2C�������� ��һ�ֽ� 
	in R16,0x3
	clr R17
	.dbline 83
; 	   
; 	  Twi();	 				   //������I2C����ʽ,���Ӧ��NO_ACK
	ldi R24,132
	out 0x36,R24
	.dbline 84
; 	  Wait();   
L47:
	.dbline 84
L48:
	.dbline 84
	in R2,0x36
	sbrs R2,7
	rjmp L47
X14:
	.dbline 84
	.dbline 84
	.dbline 85
; 	  temp = (temp<<8)+TWDR;       //���ڶ��ֽ� �ϳ�16λ��ֵ
	in R2,0x3
	mov R17,R16
	mov R16,R2
	.dbline 86
;       Stop();                      //I2Cֹͣ
	ldi R24,148
	out 0x36,R24
	.dbline 87
; 	  return temp;
	.dbline -2
L33:
	.dbline 0 ; func end
	ret
	.dbsym r temp 16 i
	.dbend
	.dbfile D:\MCU_Project\MCU_AVR\AVR_GY-30\1602.h
	.dbfunc e LCD_init _LCD_init fV
	.even
_LCD_init::
	sbiw R28,2
	.dbline -1
	.dbline 44
; /* �÷���
;    LCD_init();
;    LCD_write_string(��,��,"�ַ���");
;    LCD_write_char(��,��,'�ַ�'); 
;  ---------------------------------------------------------------
; ������AVR��LCD������Ϣ
;   PC6 ->RS
;   PC7 ->EN
;   ��  ->RW
;   PA4 ->D4
;   PA5 ->D5
;   PA6 ->D6
;   PA7 ->D7
; ʹ�ö˿ڣ�1602:PC6,PC7,PA4~PA7 	
; Ҫʹ�ñ��������ı�����������Ϣ����
; -----------------------------------------------------------------*/
; #define LCD_EN_PORT    PORTC   //����2��Ҫ��Ϊͬһ����
; #define LCD_EN_DDR     DDRC
; #define LCD_RS_PORT    PORTC   //����2��Ҫ��Ϊͬһ����
; #define LCD_RS_DDR     DDRC
; #define LCD_DATA_PORT  PORTA   //����3��Ҫ��Ϊͬһ����
; #define LCD_DATA_DDR   DDRA    //Ĭ����������߱���ʹ�ø���λ�˿�,���������ע���޸�
; #define LCD_DATA_PIN   PINA
; #define LCD_RS         (1<<PC6) //0x20   portC6       out
; #define LCD_EN         (1<<PC7) //0x40   portC7       out
; #define LCD_DATA       ((1<<PA4)|(1<<PA5)|(1<<PA6)|(1<<PA7)) //0xf0   portA 4/5/6/7 out
; /*--------------------------------------------------------------------------------------------------
; ����˵��
; --------------------------------------------------------------------------------------------------*/
; void LCD_init(void);
; void LCD_en_write(void);
; void LCD_write_command(unsigned  char command) ;
; void LCD_write_data(unsigned char data);
; void LCD_set_xy (unsigned char x, unsigned char y);
; void LCD_write_string(unsigned char X,unsigned char Y,unsigned char *s);
; void LCD_write_char(unsigned char X,unsigned char Y,unsigned char data);
; 
; //-----------------------------------------------------------------------------------------
; 
; #include <macros.h>
; #include "delay.h"
; 
; void LCD_init(void)         //Һ����ʼ��
; {
	.dbline 45
;   LCD_DATA_DDR|=LCD_DATA;   //���ݿڷ���Ϊ���
	in R24,0x1a
	ori R24,240
	out 0x1a,R24
	.dbline 46
;   LCD_EN_DDR|=LCD_EN;       //����EN����Ϊ���
	sbi 0x14,7
	.dbline 47
;   LCD_RS_DDR|=LCD_RS;       //����RS����Ϊ���
	sbi 0x14,6
	.dbline 48
;   LCD_write_command(0x28); 
	ldi R16,40
	xcall _LCD_write_command
	.dbline 49
;   LCD_en_write();
	xcall _LCD_en_write
	.dbline 50
;   delay_nus(100);
	ldi R16,100
	ldi R17,0
	xcall _delay_nus
	.dbline 51
;   LCD_write_command(0x28);  //4λ��ʾ
	ldi R16,40
	xcall _LCD_write_command
	.dbline 52
;   LCD_write_command(0x0c);  //��ʾ��
	ldi R16,12
	xcall _LCD_write_command
	.dbline 53
;   LCD_write_command(0x01);  //����
	ldi R16,1
	xcall _LCD_write_command
	.dbline 54
;   delay_nms(10);
	ldi R16,10
	ldi R17,0
	xcall _delay_nms
	.dbline 55
;   LCD_write_string(0,0,"Light:       "); 
	ldi R24,<L51
	ldi R25,>L51
	std y+1,R25
	std y+0,R24
	clr R18
	clr R16
	xcall _LCD_write_string
	.dbline 56
;   delay_nms(10);
	ldi R16,10
	ldi R17,0
	xcall _delay_nms
	.dbline -2
L50:
	.dbline 0 ; func end
	adiw R28,2
	ret
	.dbend
	.dbfunc e LCD_en_write _LCD_en_write fV
	.even
_LCD_en_write::
	.dbline -1
	.dbline 60
; }
; 
; void LCD_en_write(void)  //Һ��ʹ��
; {
	.dbline 61
;   LCD_EN_PORT|=LCD_EN;
	sbi 0x15,7
	.dbline 62
;   delay_nus(10);
	ldi R16,10
	ldi R17,0
	xcall _delay_nus
	.dbline 63
;   LCD_EN_PORT&=~LCD_EN;
	cbi 0x15,7
	.dbline -2
L52:
	.dbline 0 ; func end
	ret
	.dbend
	.dbfunc e LCD_write_command _LCD_write_command fV
;        command -> R20
	.even
_LCD_write_command::
	st -y,R20
	mov R20,R16
	.dbline -1
	.dbline 67
; }
; 
; void LCD_write_command(unsigned char command) //дָ��
; {
	.dbline 69
;   //����Ϊ��4λ��д��
;   delay_nus(16);
	ldi R16,16
	ldi R17,0
	xcall _delay_nus
	.dbline 70
;   LCD_RS_PORT&=~LCD_RS;        //RS=0
	cbi 0x15,6
	.dbline 71
;   LCD_DATA_PORT&=0X0f;         //�����λ
	in R24,0x1b
	andi R24,15
	out 0x1b,R24
	.dbline 72
;   LCD_DATA_PORT|=command&0xf0; //д����λ
	mov R24,R20
	andi R24,240
	in R2,0x1b
	or R2,R24
	out 0x1b,R2
	.dbline 73
;   LCD_en_write();
	xcall _LCD_en_write
	.dbline 74
;   command=command<<4;          //����λ�Ƶ�����λ
	mov R24,R20
	andi R24,#0x0F
	swap R24
	mov R20,R24
	.dbline 75
;   LCD_DATA_PORT&=0x0f;         //�����λ
	in R24,0x1b
	andi R24,15
	out 0x1b,R24
	.dbline 76
;   LCD_DATA_PORT|=command&0xf0; //д����λ
	mov R24,R20
	andi R24,240
	in R2,0x1b
	or R2,R24
	out 0x1b,R2
	.dbline 77
;   LCD_en_write();
	xcall _LCD_en_write
	.dbline -2
L53:
	.dbline 0 ; func end
	ld R20,y+
	ret
	.dbsym r command 20 c
	.dbend
	.dbfunc e LCD_write_data _LCD_write_data fV
;           data -> R20
	.even
_LCD_write_data::
	st -y,R20
	mov R20,R16
	.dbline -1
	.dbline 94
;  
; /*
;   //����Ϊ����λ��д��
;   delay_nus(16);
;   LCD_RS_PORT&=~LCD_RS;        //RS=0
;   LCD_DATA_PORT&=0xf0;         //�����λ
;   LCD_DATA_PORT|=(command>>4)&0x0f; //д����λ
;   LCD_en_write();
;   LCD_DATA_PORT&=0xf0;         //�����λ
;   LCD_DATA_PORT|=command&0x0f; //д����λ
;   LCD_en_write(); 
; */
;   
; }
; 
; void LCD_write_data(unsigned char data) //д����
; {
	.dbline 96
;   //����Ϊ��4λ��д��
;   delay_nus(16);
	ldi R16,16
	ldi R17,0
	xcall _delay_nus
	.dbline 97
;   LCD_RS_PORT|=LCD_RS;       //RS=1
	sbi 0x15,6
	.dbline 98
;   LCD_DATA_PORT&=0X0f;       //�����λ
	in R24,0x1b
	andi R24,15
	out 0x1b,R24
	.dbline 99
;   LCD_DATA_PORT|=data&0xf0;  //д����λ
	mov R24,R20
	andi R24,240
	in R2,0x1b
	or R2,R24
	out 0x1b,R2
	.dbline 100
;   LCD_en_write();
	xcall _LCD_en_write
	.dbline 101
;   data=data<<4;               //����λ�Ƶ�����λ
	mov R24,R20
	andi R24,#0x0F
	swap R24
	mov R20,R24
	.dbline 102
;   LCD_DATA_PORT&=0X0f;        //�����λ
	in R24,0x1b
	andi R24,15
	out 0x1b,R24
	.dbline 103
;   LCD_DATA_PORT|=data&0xf0;   //д����λ
	mov R24,R20
	andi R24,240
	in R2,0x1b
	or R2,R24
	out 0x1b,R2
	.dbline 104
;   LCD_en_write();
	xcall _LCD_en_write
	.dbline -2
L54:
	.dbline 0 ; func end
	ld R20,y+
	ret
	.dbsym r data 20 c
	.dbend
	.dbfunc e LCD_set_xy _LCD_set_xy fV
;        address -> R20
;              y -> R10
;              x -> R22
	.even
_LCD_set_xy::
	xcall push_xgsetF00C
	mov R10,R18
	mov R22,R16
	.dbline -1
	.dbline 123
;   
; /*
;   //����Ϊ����λ��д�� 
;   delay_nus(16);
;   LCD_RS_PORT|=LCD_RS;       //RS=1
;   LCD_DATA_PORT&=0Xf0;       //�����λ
;   LCD_DATA_PORT|=(data>>4)&0x0f;  //д����λ
;   LCD_en_write();
;  
;   LCD_DATA_PORT&=0Xf0;        //�����λ
;   LCD_DATA_PORT|=data&0x0f;   //д����λ
;   LCD_en_write();
; */
;   
; }
; 
; 
; void LCD_set_xy( unsigned char x, unsigned char y )  //д��ַ����
; {
	.dbline 125
;     unsigned char address;
;     if (y == 0) address = 0x80 + x;
	tst R10
	brne L56
X15:
	.dbline 125
	mov R20,R22
	subi R20,128    ; addi 128
	xjmp L57
L56:
	.dbline 126
;     else   address = 0xc0 + x;
	mov R20,R22
	subi R20,64    ; addi 192
L57:
	.dbline 127
;     LCD_write_command( address);
	mov R16,R20
	xcall _LCD_write_command
	.dbline -2
L55:
	.dbline 0 ; func end
	xjmp pop_xgsetF00C
	.dbsym r address 20 c
	.dbsym r y 10 c
	.dbsym r x 22 c
	.dbend
	.dbfunc e LCD_write_string _LCD_write_string fV
;              s -> R20,R21
;              Y -> R12
;              X -> R10
	.even
_LCD_write_string::
	xcall push_xgset303C
	mov R12,R18
	mov R10,R16
	ldd R20,y+6
	ldd R21,y+7
	.dbline -1
	.dbline 131
; }
;   
; void LCD_write_string(unsigned char X,unsigned char Y,unsigned char *s) //��x=0~15,��y=0,1
; {
	.dbline 132
;     LCD_set_xy( X, Y ); //д��ַ    
	mov R18,R12
	mov R16,R10
	xcall _LCD_set_xy
	xjmp L60
L59:
	.dbline 134
;     while (*s)  // д��ʾ�ַ�
;     {
	.dbline 135
;       LCD_write_data( *s );
	movw R30,R20
	ldd R16,z+0
	xcall _LCD_write_data
	.dbline 136
;       s ++;
	subi R20,255  ; offset = 1
	sbci R21,255
	.dbline 137
;     }
L60:
	.dbline 133
	movw R30,R20
	ldd R2,z+0
	tst R2
	brne L59
X16:
	.dbline -2
L58:
	.dbline 0 ; func end
	xjmp pop_xgset303C
	.dbsym r s 20 pc
	.dbsym r Y 12 c
	.dbsym r X 10 c
	.dbend
	.dbfunc e LCD_write_char _LCD_write_char fV
;           data -> y+2
;              Y -> R12
;              X -> R10
	.even
_LCD_write_char::
	st -y,R10
	st -y,R12
	mov R12,R18
	mov R10,R16
	.dbline -1
	.dbline 142
;       
; }
; 
; void LCD_write_char(unsigned char X,unsigned char Y,unsigned char data) //��x=0~15,��y=0,1
; {
	.dbline 143
;   LCD_set_xy( X, Y ); //д��ַ
	mov R18,R12
	mov R16,R10
	xcall _LCD_set_xy
	.dbline 144
;   LCD_write_data( data);
	ldd R16,y+2
	xcall _LCD_write_data
	.dbline -2
L62:
	.dbline 0 ; func end
	ld R12,y+
	ld R10,y+
	ret
	.dbsym l data 2 c
	.dbsym r Y 12 c
	.dbsym r X 10 c
	.dbend
	.area data(ram, con, rel)
	.dbfile D:\MCU_Project\MCU_AVR\AVR_GY-30\1602.h
_display::
	.blkb 2
	.area idata
	.byte 0,0
	.area data(ram, con, rel)
	.dbfile D:\MCU_Project\MCU_AVR\AVR_GY-30\1602.h
	.blkb 2
	.area idata
	.byte 0,0
	.area data(ram, con, rel)
	.dbfile D:\MCU_Project\MCU_AVR\AVR_GY-30\1602.h
	.blkb 2
	.area idata
	.byte 0,32
	.area data(ram, con, rel)
	.dbfile D:\MCU_Project\MCU_AVR\AVR_GY-30\1602.h
	.blkb 2
	.area idata
	.byte 'l,'u
	.area data(ram, con, rel)
	.dbfile D:\MCU_Project\MCU_AVR\AVR_GY-30\1602.h
	.blkb 1
	.area idata
	.byte 'x
	.area data(ram, con, rel)
	.dbfile D:\MCU_Project\MCU_AVR\AVR_GY-30\1602.h
	.dbfile D:\MCU_Project\MCU_AVR\AVR_GY-30\AVR_GY-30.c
	.dbsym e display _display A[9:9]c
	.area text(rom, con, rel)
	.dbfile D:\MCU_Project\MCU_AVR\AVR_GY-30\AVR_GY-30.c
	.dbfunc e conversion _conversion fV
;              i -> R20,R21
	.even
_conversion::
	st -y,R20
	st -y,R21
	movw R20,R16
	.dbline -1
	.dbline 25
; /*****************************************
; * ����AVR��Ƭ��GY-30ģ��ͨ�ų��� 		 *
; * ��    �ܣ�IICͨ�Ŷ�ȡ���ݲ���ʾ        *
; * ʱ��Ƶ�ʣ��ڲ�11.0592M 						 *
; * ��    �ƣ����˵���					 *
; * �޸����ڣ�2011��4��20��				 *
; * ���뻷����ICC-AVR7.14					 *
; * ʵ�黷����ATmega16+1602    			 *
; * ʹ�ö˿ڣ�PC0,PC1,PC6,PC7,PA4~PA7 	 *
; * ��    ����Ī����ʵ�����24c02��ȡʵ��  *
; *****************************************/
; #include <iom16v.h>
; #include "I2C.h"
; #include "1602.h"
; #include "delay.h"
; void conversion(unsigned int i);
; unsigned char display[9]={0,0,0,0,0,' ','l','u','x'};//��ʾ����
; 
; /*********************************************
; ����ת��,ʮ����������ת����10����
; ����ʮ�����Ʒ�Χ��0x0000-0x270f��0-9999��
; ����ֳɸ�ʮ��ǧλ����ascii������ʾ��
; **********************************************/
; void conversion(unsigned int i)  
; {  
	.dbline 26
;    	display[0]=i/10000+0x30 ;
	ldi R18,10000
	ldi R19,39
	movw R16,R20
	xcall div16u
	movw R24,R16
	adiw R24,48
	sts _display,R24
	.dbline 27
;     i=i%10000;    //ȡ������
	ldi R18,10000
	ldi R19,39
	movw R16,R20
	xcall mod16u
	movw R20,R16
	.dbline 28
; 	display[1]=i/1000+0x30 ;
	ldi R18,1000
	ldi R19,3
	xcall div16u
	movw R24,R16
	adiw R24,48
	sts _display+1,R24
	.dbline 29
;     i=i%1000;    //ȡ������
	ldi R18,1000
	ldi R19,3
	movw R16,R20
	xcall mod16u
	movw R20,R16
	.dbline 30
;     display[2]=i/100+0x30 ;
	ldi R18,100
	ldi R19,0
	xcall div16u
	movw R24,R16
	adiw R24,48
	sts _display+2,R24
	.dbline 31
;     i=i%100;    //ȡ������
	ldi R18,100
	ldi R19,0
	movw R16,R20
	xcall mod16u
	movw R20,R16
	.dbline 32
;     display[3]=i/10+0x30 ;
	ldi R18,10
	ldi R19,0
	xcall div16u
	movw R24,R16
	adiw R24,48
	sts _display+3,R24
	.dbline 33
;     i=i%10;     //ȡ������
	ldi R18,10
	ldi R19,0
	movw R16,R20
	xcall mod16u
	movw R20,R16
	.dbline 34
;     display[4]=i+0x30;  
	movw R24,R20
	adiw R24,48
	sts _display+4,R24
	.dbline -2
L63:
	.dbline 0 ; func end
	ld R21,y+
	ld R20,y+
	ret
	.dbsym r i 20 i
	.dbend
	.dbfunc e main _main fV
;       lux_data -> y+2
;              i -> R10
	.even
_main::
	sbiw R28,6
	.dbline -1
	.dbline 40
; }
; /*******************************
; ������
; *******************************/
; void main(void)
; {	
	.dbline 44
; 	unsigned char i;
; 	float  lux_data;                   //������   
; 	 
; 	 delay_nms(10);                    //lcd�ϵ���ʱ
	ldi R16,10
	ldi R17,0
	xcall _delay_nms
	.dbline 45
; 	 LCD_init();                       //lcd��ʼ��
	xcall _LCD_init
	.dbline 46
;      i=I2C_Write(0x01);                //BH1750 ��ʼ��            
	ldi R16,1
	xcall _I2C_Write
	mov R10,R16
	.dbline 47
; 	 delay_nms(10);          
	ldi R16,10
	ldi R17,0
	xcall _delay_nms
	xjmp L70
L69:
	.dbline 48
; 	while(1){                          //ѭ��   
	.dbline 49
; 	 i=I2C_Write(0x01);                //power on
	ldi R16,1
	xcall _I2C_Write
	.dbline 50
; 	 i=I2C_Write(0x10);                //H- resolution mode
	ldi R16,16
	xcall _I2C_Write
	mov R12,R16
	mov R10,R12
	.dbline 51
; 	 TWCR=0;                           //�ͷ�����
	clr R2
	out 0x36,R2
	.dbline 52
;      delay_nms(180);                   //��Լ180ms
	ldi R16,180
	ldi R17,0
	xcall _delay_nms
	.dbline 53
; 	   if(i==0){
	tst R12
	brne L72
X17:
	.dbline 53
	.dbline 54
; 	     lux_data=I2C_Read();          //��iic���߶�ȡ��ֵ	
	xcall _I2C_Read
	movw R10,R16
	xcall uint2fp
	std y+2,R16
	std y+3,R17
	std y+4,R18
	std y+5,R19
	.dbline 55
; 		 lux_data=(float)lux_data/1.2; //pdf�ĵ���7ҳ
	ldd R2,y+2
	ldd R3,y+3
	ldd R4,y+4
	ldd R5,y+5
	ldi R16,<L74
	ldi R17,>L74
	xcall elpm32
	st -y,R19
	st -y,R18
	st -y,R17
	st -y,R16
	movw R16,R2
	movw R18,R4
	xcall fpdiv2
	std y+2,R16
	std y+3,R17
	std y+4,R18
	std y+5,R19
	.dbline 56
; 	     conversion(lux_data);         //����ת��������ʮ���٣�ǧ λ       
	ldd R16,y+2
	ldd R17,y+3
	ldd R18,y+4
	ldd R19,y+5
	xcall fpint
	xcall _conversion
	.dbline 57
; 		 LCD_write_string(7,0,display);//��ʾ��ֵ���ӵ�9�п�ʼ   
	ldi R24,<_display
	ldi R25,>_display
	std y+1,R25
	std y+0,R24
	clr R18
	ldi R16,7
	xcall _LCD_write_string
	.dbline 58
; 	   }  
L72:
	.dbline 60
; 
;     }
L70:
	.dbline 48
	xjmp L69
X18:
	.dbline -2
L68:
	.dbline 0 ; func end
	adiw R28,6
	ret
	.dbsym l lux_data 2 D
	.dbsym r i 10 c
	.dbend
	.area lit(rom, con, rel)
L74:
	.word 0x999a,0x3f99
	.area data(ram, con, rel)
	.dbfile D:\MCU_Project\MCU_AVR\AVR_GY-30\AVR_GY-30.c
L51:
	.blkb 14
	.area idata
	.byte 'L,'i,'g,'h,'t,58,32,32,32,32,32,32,32,0
	.area data(ram, con, rel)
	.dbfile D:\MCU_Project\MCU_AVR\AVR_GY-30\AVR_GY-30.c
; }
; 
