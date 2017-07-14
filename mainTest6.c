#include<reg51.h>
#include<intrins.h>
#define uchar unsigned char
#define uint unsigned int
#define AT24C02_ADDR 0xa0

//��ѡλѡ
sbit dula=P2^6;
sbit wela=P2^7;

//DS1302
sbit TSCLK = P1^0;	//ʱ����������
sbit TIO=P1^1;		//�����������
sbit TRST=P1^2;		//ʹ�ܶ�

//I2C IO�ڶ���
sbit SDA = P2^0;
sbit SCL = P2^1;

//��������
sbit S4=P3^2;	//�ⲿ�ж�1
sbit S5=P3^3;	//�ⲿ�ж�2

//ȫ�ֱ���
uchar a1,a0,b1,b0,c1,c0,s,f,m,key=10,temp;
uchar ok=1,wei;

//����������Լ�������ʾ
unsigned char code table[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,
                        0x07,0x7f,0x6f,0x77,0x7c,0x39,0x5e,0x79,0x71,0x40};	//0-9

//��ʱ����
void delay(uchar i)
{
  uchar j, k;
  for(j=i;j>0;j--)
    for(k=125;k>0;k--);
}

//5us��ʱ
void delay_5us()  
{
	_nop_();
}

//�жϷ������⹦�ܼĴ�������
void initInterrupt()
{
	EX0=1;	//���ⲿ�ж�0
	IT0=0;	//�͵�ƽ������ʽ

	EX1=1;	//���ⲿ�ж�1
	IT0=0;	//�͵�ƽ������ʽ

	EA=1;	//�����ж�
}

//I2C��ʼ��
void I2C_init()	
{
	SDA = 1;
	_nop_();
	SCL = 1;
	_nop_();
}

//I2C��ʼ�ź�
void I2C_Start()  
{
	SCL = 1;
	_nop_();
	SDA = 1;
	delay_5us();
	SDA = 0;
	delay_5us();
}

//I2C��ֹ�ź�
void I2C_Stop()
{
	SDA = 0;
	_nop_();
	SCL = 1;
	delay_5us();
	SDA = 1;
	delay_5us();
}

//��������Ӧ��
void Master_ACK(bit i)		
{
	SCL = 0; // ����ʱ����������SDA���������ϵ����ݱ仯
	_nop_(); // �������ȶ�
	if (i)	 //���i = 1 ��ô������������ ��ʾ����Ӧ��
	{
		SDA = 0;
	}
	else	 
	{
		SDA = 1;	 //���ͷ�Ӧ��
	}
	_nop_();//�������ȶ�
	SCL = 1;//����ʱ������ �ôӻ���SDA���϶��� ������Ӧ���ź�
	delay_5us();
	SCL = 0;//����ʱ�����ߣ� ռ�����߼���ͨ��
	_nop_();
	SDA = 1;//�ͷ�SDA�������ߡ�
	_nop_();
}

//���ӻ�Ӧ��
bit Test_ACK()
{
	SCL = 1;
	delay_5us();
	if (SDA)	//�ӻ����ͷ�Ӧ���ź�
	{
		SCL = 0;
		_nop_();
		I2C_Stop();
		return(0);
	}
	else		//�ӻ�Ӧ��
	{
		SCL = 0;
		_nop_();
		return(1);
	}
}

//����һ���ֽ�
void I2C_send_byte(uchar byte)
{
	uchar i;
	for(i = 0 ; i < 8 ; i++) //�������ݸ�E2PROM���ȴ����λ��ʼ��
	{
		SCL = 0;
		_nop_();				
		if (byte & 0x80)  //1000 0000
		{ 
			_nop_();				   
			SDA = 1;	
		}
		else
		{				
			SDA = 0;
			_nop_();
		}
		SCL = 1;
		_nop_();
		byte <<= 1;	// 0101 0100B 
	}
	SCL = 0;
	_nop_();
	_nop_();
	SDA = 1;
}


//I2C ��һ�ֽ�
uchar I2C_read_byte()
{
	uchar dat,i;
	SCL = 0;
	_nop_();
	SDA = 1;   //������������
	_nop_();
	for(i = 0 ; i < 8 ; i++)
	{
		SCL = 1;
		_nop_();
		if (SDA)			    
		{
			 dat |= 0x01; //0000 0001 ��λ��ǰ��λͬdat, ���һλΪ1
		}
		else
		{
			dat &=  0xfe;	//1111 1110	 ��λ�룬ǰ��λͬdat, ���һλΪ0
		}
		_nop_();
		SCL = 0 ;
		_nop_();
		if(i < 7)
		{
			dat = dat << 1;	 //�ߵ�λ�������ӵ�λ���Ƶ���λ
		}
	}
	return(dat);
}

//I2C�����������ݣ�ÿһ��д�����ݶ���Ҫ���ӻ�Ӧ��
//ÿ��ʹ���������ֻ��д��һ�ֽڵ�����
bit I2C_transData(uchar ADDR, DAT)	//�ȷ���E2PROM��ַ���ٷ�������
{								 
	I2C_Start();					//������ʼ�ź�
	I2C_send_byte(AT24C02_ADDR+0); //д��������ַ
	if (!Test_ACK())			   //���ӻ�Ӧ�𣬾����Ƿ������������
	{
		return(0);
	}
	I2C_send_byte(ADDR); 		   //����д���׵�ַ���ɱ�д�߾���
	if (!Test_ACK())			   //���ӻ�Ӧ��
	{
		return(0);
	}
	I2C_send_byte(DAT);			 //����һ�ֽڵ�����	 
	if (!Test_ACK())		   //���ӻ�Ӧ��
	{
		return(0);
	}
	I2C_Stop();				   //I2C��ֹ�ź�
	return(1);	
}

//I2C������������
//������ȡE2PROM���ݣ�ÿ��ֻ�ܴ�E2PROM���ض���ַ��ȡһ�ֽ����ݣ������ظ�����
uchar I2C_receData(uchar ADDR)	 
{
	uchar DAT;
	I2C_Start();					 //��ʼ�ź�
	I2C_send_byte(AT24C02_ADDR+0);	 //����������ַ����������
	if (!Test_ACK())
	{
		return(0);
	}
	I2C_send_byte(ADDR);
	Master_ACK(0);		//����������Ӧ��
	//------------------
	I2C_Start();
	I2C_send_byte(AT24C02_ADDR+1); 	//����������ַ����������
	if (!Test_ACK())
	{
		return(0);
	}

	DAT = I2C_read_byte();	//������ȡ��һ�ֽ�֮��
	Master_ACK(0);		   //����������Ӧ��

	I2C_Stop();
	return(DAT);	
}

void display(uchar shi2,uchar shi1,uchar fen2,uchar fen1,uchar miao2,uchar miao1)
{
   dula=0;
   P0=table[shi2];
   dula=1;
   dula=0;
   
   wela=0;
   P0=0xfe;
   wela=1;
   wela=0;
   delay(5);
  
   P0=table[shi1];
   dula=1;
   dula=0;
   
   P0=0xfd;
   wela=1;
   wela=0;
   delay(5);
  //----

	P0=table[16];
   dula=1;
   dula=0;
   
   P0=0xfb;
   wela=1;
   wela=0;
   delay(5);
  //---
   P0=table[fen2];
   dula=1;
   dula=0;
   
   P0=0xf7;
   wela=1;
   wela=0;
   delay(5);
   
   P0=table[fen1];
   dula=1;
   dula=0;
   
   P0=0xef;
   wela=1;
   wela=0;
   delay(5);
   //----
   
	P0=table[16];
   dula=1;
   dula=0;
   
   P0=0xdf;
   wela=1;
   wela=0;
   delay(5);
  //---
   P0=table[miao2];
   dula=1;
   dula=0;
   
   P0=0xbf;
   wela=1;
   wela=0;
   delay(5);
   
   P0=table[miao1];
   dula=1;
   dula=0;
   
   P0=0x7f;
   wela=1;
   wela=0;
   delay(5);
}

//ȫ�ֱ���  temp, ok, 
void keyscan0()	  //�ж��ǰ�����S16����S17 ���ı��־λ
{
    P3=0xfb;	//P3=1111 1101
    temp=P3;
    temp=temp&0xf0;	  //temp=1111 0000 ��λ��
    if(temp!=0xf0)
    {
      delay(10);
      if(temp!=0xf0)
      {
        temp=P3;
        switch(temp)
        {
          case 0xbb:		//����S16  1011 1011
               ok=0;
               break;

          case 0x7b:	    //����S17	0111 1011
               ok=1;
               break;
         }
      }
	}	
}

//ȫ�ֱ��� temp, key(ȷ������һ������), wei, beep�����ԣ�
void keyscan()	 //����ɨ�裬�ж�����һ������������
{
 	
    P3=0xfe;	//P3=1111 1110	ǰ��Ϊ�У�����Ϊ��  һ����4�У�P3��ֵһ���ı�
    temp=P3;
    temp=temp&0xf0;	  //temp=1111 0000
    if(temp!=0xf0)	  //�ж��Ƿ��а���������
    {
      delay(2000);
      if(temp!=0xf0)
      {	
        temp=P3;
        switch(temp)
        {
          case 0xee:	  //S6
               key=0;
			   wei++;	 //�����λ��ֵ��һ��Ҳ�������ּ�һ
               break;

          case 0xde:	 //S7
               key=1;
			   wei++;
               break;

          case 0xbe:	//S8
               key=2;
			   wei++;
               break;

          case 0x7e:	 //S9
               key=3;
			   wei++;
               break;
         }
      }
    }
    P3=0xfd;   //�Ǻ͵ڶ����йص�һЩ��ֵ��
    temp=P3;
    temp=temp&0xf0;
    if(temp!=0xf0)
    {
      delay(1000);
      if(temp!=0xf0)
      {
        temp=P3;
        switch(temp)
        {
          case 0xed:	 //S10
               key=4;
			   wei++;
               break;

          case 0xdd:
               key=5;
			   wei++;
               break;

          case 0xbd:
               key=6;
			   wei++;
               break;

          case 0x7d:
               key=7;
			   wei++;
               break;
         }
      }
      }
    P3=0xfb;	   //�����е�ǰ��������
    temp=P3;
    temp=temp&0xf0;
    if(temp!=0xf0)
    {
      delay(1000);
      if(temp!=0xf0)
      {
        temp=P3;
        switch(temp)
        {
          case 0xeb:	//S14
               key=8;
			   wei++;
               break;

          case 0xdb:	//S15
               key=9;
			   wei++;
               break;
         }
      }
    }

}

//DS1302д����
void DS1302WriteDat(uchar cmd, uchar dat)
{
	uchar i;
	TRST=0;
	TSCLK=0;
	TRST=1;

	for(i=0;i<8;i++)	 //д����
	{
		TSCLK=0;
		TIO = cmd&0x01;
		TSCLK=1;
		cmd>>=1;
	}
	for(i=0;i<8;i++)	 //д����
	{
		TSCLK=0;
		TIO= dat&0x01;
		TSCLK=1;
		dat>>=1;
	}
}

//DS1302������
uchar DS1302ReadDat(uchar cmd)
{
	uchar dat, i;
	TRST=0;
	TSCLK=0;
	TRST=1;

	for(i=0;i<8;i++)   //д����
	{
		TSCLK=0;
		TIO = cmd&0x01;
		TSCLK=1;
		cmd>>=1;
	}
	for(i=0;i<8;i++)   //д����
	{
		TSCLK=0;
		dat>>=1;
		if(TIO)
		{
			dat |= 0x80;
		}
		TSCLK=1;
	}
	return dat;
}

//BCD��תʮ��������(ʮ������תʮ����)
uchar BCD_to_Ten(uchar dat)
{
	uchar dat1, dat2;
	dat1 = dat/16;
	dat2 = dat%16;
	dat2 = dat2+dat1*10;
	return dat2;
}

//ʮ��������תBCD��
uchar Ten_to_BCD(uchar dat)
{
	uchar dat1, dat2;
	dat1 = dat/10;
	dat2 = dat%10;
	dat2 = dat2+dat1*16;
	return dat2;
}


/*
DS1302��
��ɨ��S16��S17�ж��Ƿ�Ҫ��ͣ�������ʱ
�����S17�Ļ�����DS1302������ʱ������ʾ���������
�����S16�Ļ�����ֹͣDS1302����ʱ����ɨ��S6-S15��ʮ���������ֱ�Ϊ0-9��������ȷ����������
				������ֱ��key����10���˳��˹�������
���ż���ɨ�裬������ʱ
*/

void main()
{
	uchar hour, min, sec, i;
	//uchar index;
	
	//initInterrupt();   //��ʼ���ж�

	I2C_init();		//��ʼ��DS1302
	

   	hour=I2C_receData(0);
	delay(500);		 
	min=I2C_receData(1);	
	delay(500);	 
	sec=I2C_receData(2);
	delay(500);

	/*��ʱ��д���ʼʱ��*/
	DS1302WriteDat(0x8e, 0);	//���д����
	DS1302WriteDat(0x80, Ten_to_BCD(sec));	//��	 
	DS1302WriteDat(0x82, Ten_to_BCD(min));   //��
	DS1302WriteDat(0x84, Ten_to_BCD(hour));	//ʱ
	DS1302WriteDat(0x8e, 0x80);	//��д��������ֹ��д��


	while(1)
	{	
		
		keyscan0();	  //��ɨ��S16��S17
		
		if(ok==1)	//S17�����£�������ʱ
		{	
			wei=0;
			
			/*��ȡDS1302������ʾ���������*/
			DS1302WriteDat(0x8e, 0);	//���д����
			sec=BCD_to_Ten(DS1302ReadDat(0x81));	//��
			min=BCD_to_Ten(DS1302ReadDat(0x83));	//��
			hour=BCD_to_Ten(DS1302ReadDat(0x85));	//ʱ
			DS1302WriteDat(0x8e, 0x80);	//��д��������ֹ��д��

			a1=hour/10;
			a0=hour%10;
			b1=min/10;
			b0=min%10;
			c1=sec/10;
			c0=sec%10;
	
			for(i=0;i<20;i++)
				display(a1,a0,b1,b0,c1,c0);	 //�������ʾʱ��

			/*���罫ʱ�䱣����E2PROM��*/		// 12:08:36
			I2C_transData(0, hour);		//��E2PROM�д洢����
			delay(3);
			I2C_transData(1, min);		//��E2PROM�д洢����
			delay(3);
			I2C_transData(2, sec);		//��E2PROM�д洢����
			delay(3);
			
			/*׼������*/
			//if(hour==18)
			if(min==0 && sec==0)  //�߼���
			{
				P1 = 0;
				delay(200);	  //��ˮ����1��
				P1 = 0xff;
			}
		}

		else		//ok=0,��ʾS16�����£�Ҳ����ֹͣ��ʱ
		{	
			keyscan();			 //����ɨ������һ������������
			if(key!=10)			 //��Ч��Χ��1-9
			{
			
				switch(wei)	   //ȷ���� 00��00��00 �е���һλ
				{
					case 1: if(key<3)	//Сʱ���λΪ2	 Ҳ����24��00��00�е�2
							a1=key;		 //ȷ��ĳһλ������
							else
							wei--;
							break;
					case 2: if(a1==1|a1==0)
							a0=key;
							else
							if(key<5)
							a0=key;		   //��Сʱ���λΪ2ʱ����λ���Ϊ4
							break;
					case 3: if(key<7)		//�������λΪ6
							b1=key;
							else
							wei--;
							break;
					case 4: b0=key; break;
					case 5: if(key<7)		//�����λΪ6
							c1=key; 
							else
							wei--;
							break;
					case 6: c0=key; break;
				}
				key=10;
			}
			m=c1*10+c0;
			f=b1*10+b0;
			s=a1*10+a0;
			display(a1,a0,b1,b0,c1,c0);

			/*���µ�����д��DS1302��*/
			DS1302WriteDat(0x8e, 0);	//���д����
			DS1302WriteDat(0x80, Ten_to_BCD(m));	//��	 
			DS1302WriteDat(0x82, Ten_to_BCD(f));   //��
			DS1302WriteDat(0x84, Ten_to_BCD(s));	//ʱ
			DS1302WriteDat(0x8e, 0x80);	//��д��������ֹ��д��
		}	
	}
}

/*�����ж� S4���¸ı�Сʱ*/
void KeyH() interrupt 0
{
	 uchar hour;
	 if(S4==0)
	 {
	 	delay(50);
		if(!S4)
		{
			DS1302WriteDat(0x8e, 0);	//���д����
			hour=BCD_to_Ten(DS1302ReadDat(0x85)); //��ͣʱ��оƬDS1302��ʱ
			DS1302WriteDat(0x8e, 0x80);	//��д��������ֹ��д��
			hour++;

			if(hour==24)
				hour=0;

			DS1302WriteDat(0x8e, 0);	//���д����
			DS1302WriteDat(0x84, Ten_to_BCD(hour));		  
			DS1302WriteDat(0x8e, 0x80);	//��д��������ֹ��д��
		}
	 }
}

/*�����ж� S5���¸ı����*/
void KeyM() interrupt 2
{
	uchar min;
	if(S5==0)
	 {
	 	delay(50);
		if(!S5)
		{
			DS1302WriteDat(0x8e, 0);	//���д����
			min=BCD_to_Ten(DS1302ReadDat(0x83));
			DS1302WriteDat(0x8e, 0x80);	//��д��������ֹ��д��
			min++;

			if(min==60)
				min=0;

			DS1302WriteDat(0x8e, 0);	//���д����
			DS1302WriteDat(0x82, Ten_to_BCD(min));		  
			DS1302WriteDat(0x8e, 0x80);	//��д��������ֹ��д��
		}
	 }
}