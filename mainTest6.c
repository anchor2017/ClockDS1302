#include<reg51.h>
#include<intrins.h>
#define uchar unsigned char
#define uint unsigned int
#define AT24C02_ADDR 0xa0

//段选位选
sbit dula=P2^6;
sbit wela=P2^7;

//DS1302
sbit TSCLK = P1^0;	//时钟总线引脚
sbit TIO=P1^1;		//输入输出引脚
sbit TRST=P1^2;		//使能端

//I2C IO口定义
sbit SDA = P2^0;
sbit SCL = P2^1;

//独立键盘
sbit S4=P3^2;	//外部中断1
sbit S5=P3^3;	//外部中断2

//全局变量
uchar a1,a0,b1,b0,c1,c0,s,f,m,key=10,temp;
uchar ok=1,wei;

//数码管数字以及符号显示
unsigned char code table[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,
                        0x07,0x7f,0x6f,0x77,0x7c,0x39,0x5e,0x79,0x71,0x40};	//0-9

//延时函数
void delay(uchar i)
{
  uchar j, k;
  for(j=i;j>0;j--)
    for(k=125;k>0;k--);
}

//5us延时
void delay_5us()  
{
	_nop_();
}

//中断服务特殊功能寄存器配置
void initInterrupt()
{
	EX0=1;	//开外部中断0
	IT0=0;	//低电平触发方式

	EX1=1;	//开外部中断1
	IT0=0;	//低电平触发方式

	EA=1;	//开总中断
}

//I2C初始化
void I2C_init()	
{
	SDA = 1;
	_nop_();
	SCL = 1;
	_nop_();
}

//I2C起始信号
void I2C_Start()  
{
	SCL = 1;
	_nop_();
	SDA = 1;
	delay_5us();
	SDA = 0;
	delay_5us();
}

//I2C终止信号
void I2C_Stop()
{
	SDA = 0;
	_nop_();
	SCL = 1;
	delay_5us();
	SDA = 1;
	delay_5us();
}

//主机发送应答
void Master_ACK(bit i)		
{
	SCL = 0; // 拉低时钟总线允许SDA数据总线上的数据变化
	_nop_(); // 让总线稳定
	if (i)	 //如果i = 1 那么拉低数据总线 表示主机应答
	{
		SDA = 0;
	}
	else	 
	{
		SDA = 1;	 //发送非应答
	}
	_nop_();//让总线稳定
	SCL = 1;//拉高时钟总线 让从机从SDA线上读走 主机的应答信号
	delay_5us();
	SCL = 0;//拉低时钟总线， 占用总线继续通信
	_nop_();
	SDA = 1;//释放SDA数据总线。
	_nop_();
}

//检测从机应答
bit Test_ACK()
{
	SCL = 1;
	delay_5us();
	if (SDA)	//从机发送非应答信号
	{
		SCL = 0;
		_nop_();
		I2C_Stop();
		return(0);
	}
	else		//从机应答
	{
		SCL = 0;
		_nop_();
		return(1);
	}
}

//发送一个字节
void I2C_send_byte(uchar byte)
{
	uchar i;
	for(i = 0 ; i < 8 ; i++) //发送数据给E2PROM，先从最高位开始发
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


//I2C 读一字节
uchar I2C_read_byte()
{
	uchar dat,i;
	SCL = 0;
	_nop_();
	SDA = 1;   //主机接收数据
	_nop_();
	for(i = 0 ; i < 8 ; i++)
	{
		SCL = 1;
		_nop_();
		if (SDA)			    
		{
			 dat |= 0x01; //0000 0001 按位或，前八位同dat, 最后一位为1
		}
		else
		{
			dat &=  0xfe;	//1111 1110	 按位与，前八位同dat, 最后一位为0
		}
		_nop_();
		SCL = 0 ;
		_nop_();
		if(i < 7)
		{
			dat = dat << 1;	 //高低位互换，从低位左移到高位
		}
	}
	return(dat);
}

//I2C主机发送数据，每一次写入数据都需要检测从机应答
//每次使用这个函数只能写入一字节的数据
bit I2C_transData(uchar ADDR, DAT)	//先发送E2PROM地址，再发送数据
{								 
	I2C_Start();					//发送起始信号
	I2C_send_byte(AT24C02_ADDR+0); //写入器件地址
	if (!Test_ACK())			   //检测从机应答，决定是否继续发送数据
	{
		return(0);
	}
	I2C_send_byte(ADDR); 		   //发送写入首地址，由编写者决定
	if (!Test_ACK())			   //检测从机应答
	{
		return(0);
	}
	I2C_send_byte(DAT);			 //发送一字节的数据	 
	if (!Test_ACK())		   //检测从机应答
	{
		return(0);
	}
	I2C_Stop();				   //I2C终止信号
	return(1);	
}

//I2C主机接收数据
//主机读取E2PROM数据，每次只能从E2PROM的特定地址读取一字节数据，并返回该数据
uchar I2C_receData(uchar ADDR)	 
{
	uchar DAT;
	I2C_Start();					 //起始信号
	I2C_send_byte(AT24C02_ADDR+0);	 //发送器件地址并发送数据
	if (!Test_ACK())
	{
		return(0);
	}
	I2C_send_byte(ADDR);
	Master_ACK(0);		//主机发出非应答
	//------------------
	I2C_Start();
	I2C_send_byte(AT24C02_ADDR+1); 	//发送器件地址并接受数据
	if (!Test_ACK())
	{
		return(0);
	}

	DAT = I2C_read_byte();	//主机读取到一字节之后
	Master_ACK(0);		   //主机发出非应答

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

//全局变量  temp, ok, 
void keyscan0()	  //判断是按下了S16还是S17 并改变标志位
{
    P3=0xfb;	//P3=1111 1101
    temp=P3;
    temp=temp&0xf0;	  //temp=1111 0000 按位与
    if(temp!=0xf0)
    {
      delay(10);
      if(temp!=0xf0)
      {
        temp=P3;
        switch(temp)
        {
          case 0xbb:		//按键S16  1011 1011
               ok=0;
               break;

          case 0x7b:	    //按键S17	0111 1011
               ok=1;
               break;
         }
      }
	}	
}

//全局变量 temp, key(确定是哪一个按键), wei, beep（忽略）
void keyscan()	 //键盘扫描，判断是哪一个按键被按下
{
 	
    P3=0xfe;	//P3=1111 1110	前面为列，后面为行  一共有4列，P3的值一定改变
    temp=P3;
    temp=temp&0xf0;	  //temp=1111 0000
    if(temp!=0xf0)	  //判断是否有按键被按下
    {
      delay(2000);
      if(temp!=0xf0)
      {	
        temp=P3;
        switch(temp)
        {
          case 0xee:	  //S6
               key=0;
			   wei++;	 //数码管位的值加一，也就是数字加一
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
    P3=0xfd;   //是和第二行有关的一些键值码
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
    P3=0xfb;	   //第三行的前两个按键
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

//DS1302写数据
void DS1302WriteDat(uchar cmd, uchar dat)
{
	uchar i;
	TRST=0;
	TSCLK=0;
	TRST=1;

	for(i=0;i<8;i++)	 //写命令
	{
		TSCLK=0;
		TIO = cmd&0x01;
		TSCLK=1;
		cmd>>=1;
	}
	for(i=0;i<8;i++)	 //写数据
	{
		TSCLK=0;
		TIO= dat&0x01;
		TSCLK=1;
		dat>>=1;
	}
}

//DS1302读数据
uchar DS1302ReadDat(uchar cmd)
{
	uchar dat, i;
	TRST=0;
	TSCLK=0;
	TRST=1;

	for(i=0;i<8;i++)   //写命令
	{
		TSCLK=0;
		TIO = cmd&0x01;
		TSCLK=1;
		cmd>>=1;
	}
	for(i=0;i<8;i++)   //写数据
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

//BCD码转十进制数据(十六进制转十进制)
uchar BCD_to_Ten(uchar dat)
{
	uchar dat1, dat2;
	dat1 = dat/16;
	dat2 = dat%16;
	dat2 = dat2+dat1*10;
	return dat2;
}

//十进制数据转BCD码
uchar Ten_to_BCD(uchar dat)
{
	uchar dat1, dat2;
	dat1 = dat/10;
	dat2 = dat%10;
	dat2 = dat2+dat1*16;
	return dat2;
}


/*
DS1302中
先扫描S16和S17判断是否要暂停数码管走时
如果是S17的话，则DS1302正常走时，并显示在数码管上
如果是S16的话，则停止DS1302的走时，并扫描S6-S15的十个按键，分别为0-9，按六次确定六个数字
				按六次直到key等于10才退出人工按键。
接着继续扫描，继续走时
*/

void main()
{
	uchar hour, min, sec, i;
	//uchar index;
	
	//initInterrupt();   //初始化中断

	I2C_init();		//初始化DS1302
	

   	hour=I2C_receData(0);
	delay(500);		 
	min=I2C_receData(1);	
	delay(500);	 
	sec=I2C_receData(2);
	delay(500);

	/*往时钟写入初始时间*/
	DS1302WriteDat(0x8e, 0);	//清除写保护
	DS1302WriteDat(0x80, Ten_to_BCD(sec));	//秒	 
	DS1302WriteDat(0x82, Ten_to_BCD(min));   //分
	DS1302WriteDat(0x84, Ten_to_BCD(hour));	//时
	DS1302WriteDat(0x8e, 0x80);	//开写保护，防止乱写入


	while(1)
	{	
		
		keyscan0();	  //先扫描S16和S17
		
		if(ok==1)	//S17被按下，正常走时
		{	
			wei=0;
			
			/*读取DS1302数据显示在数码管上*/
			DS1302WriteDat(0x8e, 0);	//清除写保护
			sec=BCD_to_Ten(DS1302ReadDat(0x81));	//秒
			min=BCD_to_Ten(DS1302ReadDat(0x83));	//分
			hour=BCD_to_Ten(DS1302ReadDat(0x85));	//时
			DS1302WriteDat(0x8e, 0x80);	//开写保护，防止乱写入

			a1=hour/10;
			a0=hour%10;
			b1=min/10;
			b0=min%10;
			c1=sec/10;
			c0=sec%10;
	
			for(i=0;i<20;i++)
				display(a1,a0,b1,b0,c1,c0);	 //数码管显示时间

			/*掉电将时间保存在E2PROM中*/		// 12:08:36
			I2C_transData(0, hour);		//向E2PROM中存储数据
			delay(3);
			I2C_transData(1, min);		//向E2PROM中存储数据
			delay(3);
			I2C_transData(2, sec);		//向E2PROM中存储数据
			delay(3);
			
			/*准点提醒*/
			//if(hour==18)
			if(min==0 && sec==0)  //逻辑与
			{
				P1 = 0;
				delay(200);	  //流水灯亮1秒
				P1 = 0xff;
			}
		}

		else		//ok=0,表示S16被按下，也就是停止走时
		{	
			keyscan();			 //按键扫描是哪一个按键被按下
			if(key!=10)			 //有效范围是1-9
			{
			
				switch(wei)	   //确定是 00：00：00 中的哪一位
				{
					case 1: if(key<3)	//小时最高位为2	 也就是24：00：00中的2
							a1=key;		 //确定某一位的数字
							else
							wei--;
							break;
					case 2: if(a1==1|a1==0)
							a0=key;
							else
							if(key<5)
							a0=key;		   //当小时最高位为2时，低位最高为4
							break;
					case 3: if(key<7)		//分钟最高位为6
							b1=key;
							else
							wei--;
							break;
					case 4: b0=key; break;
					case 5: if(key<7)		//秒最高位为6
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

			/*把新的数据写入DS1302中*/
			DS1302WriteDat(0x8e, 0);	//清除写保护
			DS1302WriteDat(0x80, Ten_to_BCD(m));	//秒	 
			DS1302WriteDat(0x82, Ten_to_BCD(f));   //分
			DS1302WriteDat(0x84, Ten_to_BCD(s));	//时
			DS1302WriteDat(0x8e, 0x80);	//开写保护，防止乱写入
		}	
	}
}

/*按键中断 S4按下改变小时*/
void KeyH() interrupt 0
{
	 uchar hour;
	 if(S4==0)
	 {
	 	delay(50);
		if(!S4)
		{
			DS1302WriteDat(0x8e, 0);	//清除写保护
			hour=BCD_to_Ten(DS1302ReadDat(0x85)); //暂停时钟芯片DS1302走时
			DS1302WriteDat(0x8e, 0x80);	//开写保护，防止乱写入
			hour++;

			if(hour==24)
				hour=0;

			DS1302WriteDat(0x8e, 0);	//清除写保护
			DS1302WriteDat(0x84, Ten_to_BCD(hour));		  
			DS1302WriteDat(0x8e, 0x80);	//开写保护，防止乱写入
		}
	 }
}

/*按键中断 S5按下改变分钟*/
void KeyM() interrupt 2
{
	uchar min;
	if(S5==0)
	 {
	 	delay(50);
		if(!S5)
		{
			DS1302WriteDat(0x8e, 0);	//清除写保护
			min=BCD_to_Ten(DS1302ReadDat(0x83));
			DS1302WriteDat(0x8e, 0x80);	//开写保护，防止乱写入
			min++;

			if(min==60)
				min=0;

			DS1302WriteDat(0x8e, 0);	//清除写保护
			DS1302WriteDat(0x82, Ten_to_BCD(min));		  
			DS1302WriteDat(0x8e, 0x80);	//开写保护，防止乱写入
		}
	 }
}