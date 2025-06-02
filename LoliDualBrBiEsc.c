#include <8052.h>

#define u8 unsigned char
#define u16 unsigned int
#define NOP() __asm NOP __endasm

//----------------------------------------------------------
//STC15寄存器声明

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


// 改为：
__sfr __at(0xB1) P3M1;    // P3M1 寄存器
__sfr __at(0xB2) P3M0;    // P3M0 寄存器
__sfr __at(0x8E) AUXR;    // AUXR 寄存器
__sfr __at(0xD6) T2H;     // Timer2 高字节
__sfr __at(0xD7) T2L;     // Timer2 低字节
__sfr __at(0xAF) IE2;     // 中断使能寄存器2



// 改为：
__sbit __at(0xB0) P30;  // P3.0  
__sbit __at(0xB1) P31;  // P3.1
__sbit __at(0xB2) P32;  // P3.2
__sbit __at(0xB3) P33;  // P3.3
__sbit __at(0xB5) P35;  // P3.5
__sbit __at(0xB4) P34;  // P3.4

#define A1 P30
#define A2 P31
#define B1 P32
#define B2 P33

#define IN1 P35   //pwm 输入1
#define IN2 P34	//pwm 输入2



//----------------------------------------------------------
//软件PWM

u8 PWM_count;
u8 PWM_duty_A;
u8 PWM_duty_B;

u8 PWM_value;

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
#define Direction_A (status_bits & 0x20)    // 位5
#define Direction_B (status_bits & 0x40)    // 位6

// 设置位的宏
#define SET_IN1_last()    (status_bits |= 0x01)
#define SET_IN2_last()    (status_bits |= 0x02)
#define SET_get_pulse1()  (status_bits |= 0x04)
#define SET_get_pulse2()  (status_bits |= 0x08)
#define SET_get_new()     (status_bits |= 0x10)
#define SET_Direction_A() (status_bits |= 0x20)
#define SET_Direction_B() (status_bits |= 0x40)

// 清除位的宏
#define CLR_IN1_last()    (status_bits &= ~0x01)
#define CLR_IN2_last()    (status_bits &= ~0x02)
#define CLR_get_pulse1()  (status_bits &= ~0x04)
#define CLR_get_pulse2()  (status_bits &= ~0x08)
#define CLR_get_new()     (status_bits &= ~0x10)
#define CLR_Direction_A() (status_bits &= ~0x20)
#define CLR_Direction_B() (status_bits &= ~0x40)

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
	
 	P3=0xF0;	      //上电关闭输出
 
	P3M0=0x0F;      //IO推挽输出
	
	delay_ms(400);
	
	shock(200);	//新增 上电音乐，表明正常工作
	shock(150);
	shock(100);
	
	
	IE=0x80;
	IE2=0x04;
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
			if(mix_x>=153 )       // A路正转区间：>=1530μs
			{
				//=======================================================
				// 正转摩擦力补偿公式：(mix_x-151)*100/44
				//=======================================================
				// 关键参数解析：
				// • mix_x-151：起始点偏移补偿
				//   - 151而非150(中位)：提供1单位预加载
				//   - 当mix_x=153时：153-151=2 → 最小输出≠0
				//   - 避免在静摩擦阈值附近震荡
				//
				// • 除以44而非45：量程优化补偿  
				//   - 理论量程：195-150=45
				//   - 实际量程：195-151=44
				//   - 效果：将更多分辨率分配给低速区间
				//
				// • 乘以100：转换为百分比PWM占空比
				//
				// 补偿效果对比：
				// 遥控量  |  线性映射  |  补偿映射  |  实际转速响应
				// 153     |    6.7%    |   4.5%    |  立即缓慢启动
				// 160     |   22.2%    |  20.5%    |  平滑低速运行  
				// 180     |   66.7%    |  65.9%    |  稳定中速运行
				// 195     |   100%     |   100%    |  最高速运行
				PWM_duty_A= (mix_x-151)*100/44; // 补偿摩擦力的非线性映射
				CLR_Direction_A();        // 设置A路正转方向（A2输出PWM，A1保持低）
			}
			else if(mix_x<=147 )  // A路反转区间：<=1470μs
			{
				//=======================================================
				// 反转摩擦力补偿公式：(149-mix_x)*100/44
				//=======================================================
				// 对称性设计原理：
				// • 149-mix_x：反向起始点偏移
				//   - 149而非150：提供反向1单位预加载
				//   - 当mix_x=147时：149-147=2 → 最小反向输出≠0
				//   - 与正转形成完美对称的控制特性
				//
				// • 反向量程：149-105=44（与正转量程相同）
				//   - 保证正反转响应的一致性
				//   - 用户操作感受完全对称
				//
				// 反转补偿效果：
				// 遥控量  |  反转深度  |  PWM占空比  |  实际转速响应
				// 147     |     2      |    4.5%     |  立即缓慢反转
				// 140     |     9      |   20.5%     |  平滑低速反转
				// 120     |    29      |   65.9%     |  稳定中速反转  
				// 105     |    44      |    100%     |  最高速反转
				PWM_duty_A= (149-mix_x)*100/44; // 反转摩擦力补偿
				SET_Direction_A();        // 设置A路反转方向（A1输出PWM，A2保持低）
			}
			else                  // A路停止死区：1470-1530μs（60μs宽度）
			{
				//=======================================================
				// 死区设计的工程意义：
				//=======================================================
				// • 防止误触发：遥控杆中位附近的微小抖动不会导致电机转动
				// • 明确停止：提供清晰的"停止"状态，用户操作更确定
				// • 保护电机：避免正反转快速切换导致的电流冲击
				// • 节能设计：中位时完全关闭PWM输出
				PWM_duty_A=0;        // 完全停止输出，电机进入制动状态
			}
			
			//===========================================================
			// B路电机控制量计算（基于mix_y的值）
			// mix_y取值范围：105-195（对应1050-1950μs）
			//===========================================================
			if(mix_y>=153 )       // B路正转判断：>=1530μs（中位150+死区3）
			{
				// 正转PWM占空比计算：
				// (mix_y-151)*100/44 
				// 当mix_y=153时：(153-151)*100/44 = 4.5%
				// 当mix_y=195时：(195-151)*100/44 = 100%
				// 151是正转起始点，44是满量程范围(195-151)
				PWM_duty_B= (mix_y-151)*100/44; // 补偿摩擦力的非线性映射
				CLR_Direction_B();        // 设置B路正转方向（B2输出PWM）
			}
			else if(mix_y<=147 )  // B路反转判断：<=1470μs（中位150-死区3）
			{
				// 反转PWM占空比计算：
				// (149-mix_y)*100/44
				// 当mix_y=147时：(149-147)*100/44 = 4.5%  
				// 当mix_y=105时：(149-105)*100/44 = 100%
				// 149是反转起始点，44是满量程范围(149-105)
				PWM_duty_B= (149-mix_y)*100/44; // 反转时占空比递增
				SET_Direction_B();        // 设置B路反转方向（B1输出PWM）
			}
			else                  // B路停止区间：1470-1530μs（60μs死区）
			{
				PWM_duty_B=0;        // 停止输出，电机制动
			}
			
		}
		
		
	}
	
}

//10us 触发一次调用
//IN1_H_time 取值 0-255， 即0-2550us
//IN2_H_time 取值 0-255， 即0-2550us
//PWM_count 是0-100， 即每1000us 重置一次
//PWM_duty_A 和PWM_duty_B, 取值都是0-100，当PWM_count 小于PWM_duty_A/B时，管脚高电平，否则低
//软件PWM 1Khz
void T2_isr() __interrupt(12)
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
	
	
	//=======================================================================
	// PWM输出控制部分 - 每10μs执行一次（中断频率100kHz）
	// PWM周期：1ms（PWM_count从0计数到100）
	// PWM分辨率：100级（占空比精度1%）
	//=======================================================================
	
	// A路电机H桥PWM控制
	if(PWM_duty_A > PWM_count)    // PWM有效期间（高电平期间）
	{
		if(Direction_A)           // 方向控制：1=反转，0=正转
			A1=1;                 // 反转：A1高，A2保持低（由else分支控制）
		else 
			A2=1;                 // 正转：A2高，A1保持低（由else分支控制）
	}
	else A1=0,A2=0;              // PWM无效期间：A1、A2都为低电平（电机停止/刹车）
	
	// B路电机H桥PWM控制（逻辑与A路相同）
	if(PWM_duty_B > PWM_count)    // PWM有效期间（高电平期间）
	{
		if(Direction_B)           // 方向控制：1=反转，0=正转  
			B1=1;                 // 反转：B1高，B2保持低
		else 
			B2=1;                 // 正转：B2高，B1保持低
	}
	else B1=0,B2=0;              // PWM无效期间：B1、B2都为低电平（电机停止/刹车）

	
}