#include  <8052.h>

#define u8 unsigned char
#define u16 unsigned int
#define NOP() __asm NOP __endasm


//----------------------------------------------------------
//STC15寄存器声明, 原来是使用STC15的，如stc15w104
//把STC8G 的PWM 输出控制改成硬件方式的,输出脚为 p3.2 p3.3和p5.4, p5.5  PWM 输入改为p3.0和p3.1,
// 把STC8G 的PWM 输出控制改成硬件方式的,输出脚为 p3.2 p3.3和p5.4, p5.5  PWM 输入改为p3.0和p3.1,
// A正B正转, CCP_S 设置为 10  p3.2 拉低,    CCP1 在p3.3输出pwm, p5.4 拉低,          CCP2在p5.5输出 
// A正B反转, CCP_S 设置为 00  p3.2 拉低,     CCP1 在p3.3输出pwm, CCP2在 p5.4 输出pwm,   p5.5拉低
// A反B正转,CCP_S 设置为 10  p3.2 输出pwm, p3.3 拉低,   p5.4 拉低,  CCP2在 p5.5输出
// A反B反转,CCP_S 设置为 00  p3.2 输出pwm,  p3.3 拉低,  CCP2在 p5.4 输出pwm,   p5.5拉低


// CCMOD,  b0 是ecf, 是否允许PCA溢出中断
//          b3 B2 B1， 是CPS[2:0], 计数脉冲源选择， 000 系统时钟/12，  001 系统时钟/2 ，

//          b6 b5 b4 没有使用
//          b7 CIDL 空闲模式是否停止PCA计数
// CCON,  b6是 CR， 是否开始PCA计数

// 将这些行：
// sfr P3M1=0xb1;
// sfr P3M0=0xb2;
// sfr AUXR=0x8e;
// sfr T2H=0xd6;
// sfr T2L=0xd7;
// sfr IE2=0xaf;

//----------------------------------------------------------
//引脚定义
// sbit A1=P3^0;
// sbit A2=P3^1;
// sbit B1=P3^2;
// sbit B2=P3^3;
// sbit IN1=P3^5;
// sbit IN2=P3^4;


__sfr __at(0xB1) P3M1;    // P3M1 寄存器
__sfr __at(0xB2) P3M0;    // P3M0 寄存器
__sfr __at(0xC9) P5M1;    // P5M1 寄存器
__sfr __at(0xCA) P5M0;    // P5M0 寄存器
__sfr __at(0x8E) AUXR;    // AUXR 寄存器
__sfr __at(0x8A) T0L;     // Timer0 
__sfr __at(0x8C) T0H;     // Timer0 
__sfr __at(0x8B) T1L;     // Timer1 
__sfr __at(0x8D) T1H;     // Timer1 


__sfr __at(0xD6) T2H;     // Timer2 高字节
__sfr __at(0xD7) T2L;     // Timer2 低字节
__sfr __at(0xAF) IE2;     // 中断使能寄存器2
__sfr __at(0xA2) PSW_1;     // 程序状态字寄存器

__sbit __at(0xB0) P30;  // P3.0  
__sbit __at(0xB1) P31;  // P3.1
__sbit __at(0xB2) P32;  // P3.2
__sbit __at(0xB3) P33;  // P3.3
__sbit __at(0xB4) P34;  // P3.4
__sbit __at(0xB5) P35;  // P3.5

__sbit __at(0xC8) P50;  // P5.0  
__sbit __at(0xC9) P51;  // P5.1
__sbit __at(0xCA) P52;  // P5.2
__sbit __at(0xCB) P53;  // P5.3
__sbit __at(0xCC) P54;  // P5.4
__sbit __at(0xCD) P55;  // P5.5
__sbit __at(0xCE) P56;  // P5.6
__sbit __at(0xCF) P57;  // P5.7


#define A1 P54
#define A2 P55
#define B1 P32
#define B2 P33
#define IN1 P30   // pwm 输入1改为P3.0
#define IN2 P31   // pwm 输入2改为P3.1



//----------------------------------------------------------
//软件PWM

// u8 PWM_count;
// u8 PWM_duty_A;
// u8 PWM_duty_B;

//u8 PWM_value;

//----------------------------------------------------------
//控制

u16 timer1,timer2;

u16 pulse;

u16 IN1_H_time,IN2_H_time;
// 将这些行：
// bit IN1_last,IN2_last;
// bit get_pulse1,get_pulse2;
// bit get_new;
// bit Direction_A,Direction_B;

// 改为：
unsigned char status_bits;
#define IN1_last    (status_bits & 0x01)    // 位0
#define IN2_last    (status_bits & 0x02)    // 位1
#define get_pulse1  (status_bits & 0x04)    // 位2
#define get_pulse2  (status_bits & 0x08)    // 位3
#define get_new     (status_bits & 0x10)    // 位4

// 设置位的宏
#define SET_IN1_last()    (status_bits |= 0x01)
#define SET_IN2_last()    (status_bits |= 0x02)
#define SET_get_pulse1()  (status_bits |= 0x04)
#define SET_get_pulse2()  (status_bits |= 0x08)
#define SET_get_new()     (status_bits |= 0x10)

// 清除位的宏
#define CLR_IN1_last()    (status_bits &= ~0x01)
#define CLR_IN2_last()    (status_bits &= ~0x02)
#define CLR_get_pulse1()  (status_bits &= ~0x04)
#define CLR_get_pulse2()  (status_bits &= ~0x08)
#define CLR_get_new()     (status_bits &= ~0x10)

int mix_x,mix_y;

//bit Direction_A,Direction_B;


int pulse1=150,pulse2=150;
u8 lose_A=20,lose_B=20;  //更新：上电默认处于失控状态，防止第一次信号检测错误


//----------------------------------------------------------
//混控设置
#define  mix_en 0  //关闭混控，两路独立
//bit mix_en=1;  //开启混控






void delay_ms(u16 ms)		//STC15 @12.000MHz
{
	u16 i;

	do{
	    i = 12000000 / 13022;
		
		NOP();NOP();NOP();
		
		while(--i)	;   
     }while(--ms);	
}

void delay_us(u8 us)		//@12.000MHz
{

	while(--us)
	{
		NOP();NOP();
	}
}

void shock( u8 n)    //震动发声
{
	u8 i;
	for(i=0;i<250;i++)
	{
		A1=0,A2=0;
		B1=0,B2=0;		
		delay_us(n);
		
		A1=1;B1=1;
		delay_us(n);
		
		A1=0;B1=0;		
		delay_us(n);
				
		A2=1;B2=1;
		delay_us(n);	
	}
	A1=0,A2=0;
	B1=0,B2=0;
	delay_us(n);
}


void main()
{
	
 	P3=0xF0;	      // 上电时将P3端口设置为0xF0 (二进制 1111 0000)。
						// 	这通常用于设置端口的初始电平状态。
						// 具体哪些引脚被设置为高电平或低电平取决于微控制器的硬件连接。
	
	P3M1=0x00;      // 将P3M1寄存器设置为0x00 (二进制 0000 0000)。这通常用于设置P3端口的模式。根据您提供的模式表 (P3M1位, P3M0位)：
                    // (0, 0): 准双向口 (弱上拉)
	P3M0=0x0F;      // 将P3M0寄存器设置为0x0F (二进制 0000 1111)。结合P3M1寄存器（如果未显式设置，通常默认为0x00），根据您提供的模式表 (P3M1位, P3M0位)：
                    // (0, 0): 准双向口 (弱上拉)
                    // (0, 1): 推挽输出 (强上拉)
                    // (1, 0): 高阻输入
                    // (1, 1): 开漏 (内部电阻断开)
                    // P3M0的低4位 (P3.0-P3.3) 设置为1，高4位 (P3.4-P3.7) 设置为0。
					// 如果P3M1默认为0，则P3.0-P3.3的模式为 (0,1) 推挽输出，P3.4-P3.7的模式为 (0,0) 准双向口。
					// 这与代码注释“IO推挽输出”部分一致，表明P3的低4位被配置为推挽输出, 然后p34, p35是准双向口 (弱上拉)
	
#if STC8G
	P5M1=0x00;      // 将P5M1寄存器设置为0x00 (二进制 0000 0000)。这通常用于设置P5端口的模式。根据您提供的模式表 (P5M1位, P5M0位)：//(0, 0): 准双向口 (弱上拉)	
	P5M0=0x00;      // 将P5M1寄存器设置为0x00 (二进制 0000 0000)。这通常用于设置P5端口的模式。根据您提供的模式表 (P5M1位, P5M0位)：//(0, 0): 准双向口 (弱上拉)	
	 // 硬件PWM初始化（替换原软件PWM的定时器初始化）
	 CCON = 0x00;    // 清空中断，停止PCA计数
	 CMOD = 0x08;    // 0000 1000：CPS=000（系统时钟/12），禁止PCA溢出中断
	 CL = 0x00;      // PCA计数器低字节清零
	 CH = 0x00;      // PCA计数器高字节清零
 
	 // 配置CCP模块为PWM模式
	 CCAPPM0 = 0x42; // PCA模块0：PWM模式（8位或7位）
	 CCAPPM1 = 0x42; // PCA模块1：PWM模式
	 CCAPPM2 = 0x42; // PCA模块2：PWM模式
	 PCA_PWM0 = 0x40;// 7位PWM（CL溢出值0x7F）
	 PCA_PWM1 = 0x40;
	 PCA_PWM2 = 0x40;
 
	 CCON |= 0x80;   // CR=1，启动PCA计数

#endif

	delay_ms(400);  // 调用延时函数，暂停程序执行约400毫秒。
	
	shock(200);     // 调用 shock 函数，参数为200。
	shock(150);     // 调用 shock 函数，参数为150。
	shock(100);     // 调用 shock 函数，参数为100。
                    // 这三行代码及其注释“上电音乐，表明正常工作”表明，通过调用 shock 函数并传入不同的参数，可能是在控制蜂鸣器或其他音频设备发出不同音调的声音，形成一段“上电音乐”，以此作为微控制器正常启动的指示。
	
#if STC8G
	IE=0x88;        // 将IE寄存器设置为0x80 (二进制 1000 1000)。 ET1 = 1
// 在许多8051系列的微控制器中，IE寄存器的最高位 (EA, Enable All)
//  用于全局中断使能。设置为1 (0x80) 表示使能总中断，允许各个独立中断源产生中断请求。
	T1L=0xF6;T1H=0xFF;  //开启软件PWM

	AUXR = 0x00; //T1x12 = 0
	TCON = 0x40; //0100 0000  TR1=1
#else
	IE=0x80;        // 将IE寄存器设置为0x80 (二进制 1000 0000)。
					// 在许多8051系列的微控制器中，IE寄存器的最高位 (EA, Enable All)
					//  用于全局中断使能。设置为1 (0x80) 表示使能总中断，允许各个独立中断源产生中断请求。
	IE2=0x04;       // 将IE2寄存器设置为0x04 (二进制 0000 0100)。
						// IE2是中断使能寄存器2，用于控制其他中断源的使能。
						// 0x04表示设置了第2位（从0开始计数）。
						// 根据手册是 第2位 ET2 , 定时器允许中断
	T2L=0xF6;T2H=0xFF;  //开启软件PWM
	AUXR=0x10;
// 定时器初值：
// T2H = 0xFF, T2L = 0xF6
// 16位初值 = 0xFFF6 = 65526
// 计数次数 = 65536 - 65526 = 10
// 在12T模式下的中断周期：
// 系统时钟：12MHz
// 在12T模式下，定时器时钟 = 12MHz ÷ 12 = 1MHz
// 中断周期 = 10 × (1/1MHz) = 10μs
// AUXR = 0x10 = 0001 0000 (二进制)
// 第3位(T2x12) = 0  12分频
// 第4位(T2C/T) = 0   ，T2C/T=0，内部时钟
// 第5位(T2R)=1 ， 允许T2 运行
#endif


	
	while(1)
	{
		
		if(get_pulse1)
		{
			CLR_get_pulse1();
			
			pulse =IN1_H_time;			
			IN1_H_time=0;
			
			if(pulse >85 && pulse <215) //只受理合理舵量范围
			{
				timer1=0;
				
				if(pulse <105)pulse =105;
				if(pulse >195)pulse =195; 
				
				if(lose_A)lose_A--;  //丢信号重连保护
				else { SET_get_new(); pulse1=pulse; }
				
			}
			
		}
		
		if(get_pulse2)
		{
			CLR_get_pulse2();
			
			pulse =IN2_H_time;			
			IN2_H_time=0;
			
			if(pulse >85 && pulse <215)
			{
				timer2=0;
				
				if(pulse <105)pulse =105;
				if(pulse >195)pulse =195;		//舵量限幅 1050~1950	

				if(lose_B)lose_B--;  //丢信号重连保护
				else { SET_get_new(); pulse2=pulse; }
			}			
			
		}
		
		
		if(get_new)
		{
			CLR_get_new();
			
			if(mix_en)  //引脚高电平 使能混控
			{
				if(pulse2>152 || pulse2<148) //通道2为差向输入，中位死区
				{
					mix_x=pulse1 + pulse2 -150;  
					mix_y=pulse1 - pulse2 +150;	
				}
				else
				{
					mix_x=pulse1;   //通道1为同向输入，
					mix_y=pulse1;
				}
							
			}
			else        //引脚低电平 两通道独立
			{
				mix_x=pulse1;
				mix_y=pulse2;				
			}
			
			
			if(mix_x<105)mix_x=105;
			if(mix_x>195)mix_x=195;
			
			if(mix_y<105)mix_y=105;
			if(mix_y>195)mix_y=195;
			
// 理想电机特性：
// PWM占空比    转速
// 0%    →     0 RPM
// 20%   →     20% 最大转速  
// 50%   →     50% 最大转速
// 100%  →     100% 最大转速			
// 实际电机特性：
// PWM占空比    转速
// 0%    →     0 RPM
// 20%   →     0 RPM (静摩擦力未克服)
// 30%   →     5% 最大转速 (刚好克服静摩擦)
// 50%   →     45% 最大转速
// 100%  →     100% 最大转速
//
			//===========================================================
			// A路电机控制量计算 - 摩擦力补偿算法
			// mix_x取值范围：105-195（对应1050-1950μs）
			// 核心思想：通过非线性映射克服电机静摩擦力，实现平滑可控
			//===========================================================
			// 四组合电机控制
			if(mix_x >= 153 && mix_y >= 153) { // 组合1: A正B正 (CCP_S=10)
				PSW_1 = 0x20;  // b5=1, b4=0 → 10
				CCAP0L = 0; CCAP0H = 0;    // B1(P3.2) PWM固定输出0
				B1 = 0;                     // B1强制GPIO模式拉低
				CCAP1L = (u8)(((mix_x - 151) * 127)/44); // B2(P3.3) PWM输出
				A1 = 0;                     // A1(P5.4) GPIO拉低
				CCAP2L = (u8)(((mix_y - 151) * 127)/44); // A2(P5.5) PWM输出
			} 
			else if(mix_x >= 153 && mix_y <= 147) { // 组合2: A正B反 (CCP_S=00)
				PSW_1 = 0x00;  
				CCAP0L = 0; CCAP0H = 0;    // B1(P3.2) PWM固定输出0
				B1 = 0;                     // B1强制GPIO模式拉低
				CCAP1L = (u8)(((mix_x - 151) * 127)/44); // B2(P3.3) PWM输出
				CCAP2L = (u8)(((149 - mix_y) * 127)/44); // A1(P5.4) PWM输出
				A2 = 0;                     // A2(P5.5) GPIO拉低
			}
			else if(mix_x <= 147 && mix_y >= 153) { // 组合3: A反B正 (CCP_S=10)
				PSW_1 = 0x20;
				CCAP0L = (u8)(((149 - mix_x) * 127)/44); // B1(P3.2) PWM输出
				CCAP1L = 0; CCAP1H = 0;    // B2(P3.3) PWM固定输出0
				B2 = 0;                     // B2强制GPIO模式拉低
				A1 = 0;                     // A1(P5.4) GPIO拉低
				CCAP2L = (u8)(((mix_y - 151) * 127)/44); // A2(P5.5) PWM输出
			}
			else if(mix_x <= 147 && mix_y <= 147) { // 组合4: A反B反 (CCP_S=00)
				PSW_1 = 0x00;
				CCAP0L = (u8)(((149 - mix_x) * 127)/44); // B1(P3.2) PWM输出
				CCAP1L = 0; CCAP1H = 0;    // B2(P3.3) PWM固定输出0
				B2 = 0;                     // B2强制GPIO模式拉低
				CCAP2L = (u8)(((149 - mix_y) * 127)/44); // A1(P5.4) PWM输出
				A2 = 0;                     // A2(P5.5) GPIO拉低
			}
			else { // 停止状态
				PSW_1 = 0x00;
				CCAP0L = 0; CCAP0H = 0;
				CCAP1L = 0; CCAP1H = 0;
				CCAP2L = 0; CCAP2H = 0;
				A1 = 0;                     // A1(P5.4) GPIO拉低
			}


		
	}
	
}

//10us 触发一次调用
//IN1_H_time 取值 0-255， 即0-2550us
//IN2_H_time 取值 0-255， 即0-2550us
//PWM_count 是0-100， 即每1000us 重置一次
//PWM_duty_A 和PWM_duty_B, 取值都是0-100，当PWM_count 小于PWM_duty_A/B时，管脚高电平，否则低
//软件PWM 1Khz
void T1_isr() __interrupt(3)
{

	
	if(IN1)					//通道1脉宽检测
	{
		SET_IN1_last();
		IN1_H_time++;
	}
	else 
	{
		if(IN1_last)  SET_get_pulse1();
		CLR_IN1_last();
	}
	
	if(IN2)					//通道2脉宽检测
	{
		SET_IN2_last();
		IN2_H_time++;
	}
	else 
	{
		if(IN2_last) SET_get_pulse2();
		CLR_IN2_last();
	}	
	
	
	PWM_count++;
	if(PWM_count==100) //100级分辨率
	{
		PWM_count=0;
		
		timer1++;
		if(timer1>500)//0.5S无信号保护
		{
			timer1=0; lose_A=30;
			
			pulse1=150; SET_get_new();		
		}
		
		timer2++;
		if(timer2>500)//0.5S无信号保护
		{
			timer2=0; lose_B=30;
			
			pulse2=150; SET_get_new();		
		}
	}
	

	
}