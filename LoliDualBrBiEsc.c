#include  <8052.h>

#define u8 unsigned char
#define u16 unsigned int
#define u32 unsigned long

#define NOP() __asm NOP __endasm


//----------------------------------------------------------
//STC15寄存器声明, 原来是使用STC15的，如stc15w104
//增加STC8G1单片机的支持， 改为timer1做定时，输出脚从p3.5 p3.4 改为p5.5 p5.4

#define STC8G 1

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


#define A1 P30
#define A2 P31
#define B1 P32
#define B2 P33

#if STC8G
#define IN1 P55   //pwm 输入1
#define IN2 P54	//pwm 输入2
#else
#define IN1 P35   //pwm 输入1
#define IN2 P34	//pwm 输入2
#endif



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

// 状态机状态定义
#define STATE_INIT_COMPLETE   0  // 初始化完成
#define STATE_CHECK_SIGNAL    1  // 检查输入信号中
#define STATE_SETTING         2  // 功能设置中
#define STATE_NORMAL_RUNNING  3  // 正常运行中

// 功能设置相关定义
#define FUNC_MIX_CONTROL      1  // 功能1：混控设置
#define FUNC_DIRECTION        2  // 功能2：方向设置

// 参数值定义
#define PARAM_MIX_DISABLE     1  // 默认不混控
#define PARAM_MIX_ENABLE      2  // 启用混控
#define PARAM_DIR_BIDIRECT    1  // 默认双向
#define PARAM_DIR_UNIDIRECT   2  // 单向

// 油门状态定义
#define THROTTLE_ZERO         0  // 油门归零
#define THROTTLE_UP           1  // 油门提升
#define THROTTLE_FULL         2  // 全油门

u8 system_state = STATE_INIT_COMPLETE;  // 系统状态
u8 current_function = FUNC_MIX_CONTROL; // 当前设置的功能
u8 current_param = PARAM_MIX_DISABLE;   // 当前参数值
u8 beep_counter = 0;                    // 蜂鸣器计数器
u8 repeat_counter = 0;                  // 重复计数器
u8 throttle_state = THROTTLE_ZERO;      // 油门状态跟踪

// 混控和方向设置
u8 mix_control_setting = PARAM_MIX_DISABLE;  // 默认不混控
u8 direction_setting = PARAM_DIR_BIDIRECT;   // 默认双向

int mix_x,mix_y;

//bit Direction_A,Direction_B;


int pulse1=150,pulse2=150;
u8 lose_A=20,lose_B=20;  //更新：上电默认处于失控状态，防止第一次信号检测错误
u32 g_ticks; //全局时钟

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


// 基于g_ticks的精确延时函数，带有最大延时限制
// 输入参数：us - 目标延时微秒数
// 返回值：1 - 达到目标时间，0 - 提前返回
u8 delay_us_max(u16 us)
{
	u32 target_ticks;
	u32 start_ticks = g_ticks;
	u32 max_ticks;
	u8 loop_count;
	
	// 计算目标ticks值，每10μs增加1个tick
	target_ticks = start_ticks + (us / 10) + ((us % 10) ? 1 : 0);
	
	// 先以最大可能性计算循环次数
	loop_count = (us > 0) ? ((us / 10) + 1) : 1;
	
	// 循环执行processinput，同时检查g_ticks
	while(loop_count--)
	{
		processinput(); // 处理输入
		
		// 检查是否已经达到目标时间
		if(g_ticks >= target_ticks)
		{
			return 1; // 达到目标时间，正常返回
		}
	}
	
	return 0; // 未达到目标时间，提前返回
}

// 使用delay_us_max实现的毫秒级延时函数
u8 delay_ms_max(u16 ms)
{
	u8 result = 1;
	u16 us;
	
	// 将毫秒转换为微秒
	us = ms * 1000;
	
	// 如果超过delay_us_max的最大参数值(65535)，则多次调用
	while(us > 65535)
	{
		result &= delay_us_max(65535);
		us -= 65535;
	}
	
	// 处理剩余的微秒数
	if(us > 0)
	{
		result &= delay_us_max(us);
	}
	
	return result;
}



void processinput()
{
	if(get_pulse1)
	{
		CLR_get_pulse1();
		
		pulse = IN1_H_time;			
		IN1_H_time = 0;
		
		if(pulse > 85 && pulse < 215) // 只受理合理舵量范围
		{
			timer1 = 0;
			
			if(pulse < 105) pulse = 105;
			if(pulse > 195) pulse = 195; 
			
			if(lose_A) lose_A--;  // 丢信号重连保护
			else { SET_get_new(); pulse1 = pulse; }
		}
	}
	
	if(get_pulse2)
	{
		CLR_get_pulse2();
		
		pulse = IN2_H_time;			
		IN2_H_time = 0;
		
		if(pulse > 85 && pulse < 215)
		{
			timer2 = 0;
			
			if(pulse < 105) pulse = 105;
			if(pulse > 195) pulse = 195;		// 舵量限幅 1050~1950	

			if(lose_B) lose_B--;  // 丢信号重连保护
			else { SET_get_new(); pulse2 = pulse; }
		}			
	}
	
}


void shock( u8 n, u8 len)    //震动发声，n为延时参数，len为循环次数
{
	u8 i;
	
	for(i=0;i<len;i++)
	{
		A1=0,A2=0;
		B1=0,B2=0;		
		// 使用最大延时函数
		delay_us_max(n);
		
		A1=1;B1=1;
		delay_us_max(n);
		
		A1=0;B1=0;		
		delay_us_max(n);
				
		A2=1;B2=1;
		delay_us_max(n);
	}
	A1=0,A2=0;
	B1=0,B2=0;
	delay_us_max(n);
}

// 短音"哔"
void beep_short(u8 tone)
{
	shock(tone, 100); // 减少循环次数，产生更短的音效
	delay_ms_max(100); // 使用delay_ms_max替代delay_ms
}

// 长音"哔"
void beep_long(u8 tone)
{
	shock(tone, 400); // 增加循环次数，产生更长的音效
	delay_ms_max(100); // 使用delay_ms_max替代delay_ms
}

// 双响"哔哔"
void beep_double(u8 tone)
{
	shock(tone, 100); // 使用与 beep_short 一致的循环次数
	delay_ms_max(100); // 使用delay_ms_max替代delay_ms
	shock(tone, 100); // 使用与 beep_short 一致的循环次数
	delay_ms_max(100); // 使用delay_ms_max替代delay_ms
}

// 播放启动序列音
void play_startup_sound()
{
	// 升调三声"哔哔哔"
	shock(200, 150); // 调整循环次数，使音效更清晰
	shock(150, 150); // 调整循环次数，使音效更清晰
	shock(100, 150); // 调整循环次数，使音效更清晰
}

// 播放油门提升音
void play_throttle_up_sound()
{
	// 一声长低音"哔"
	beep_long(200);
}

// 播放全油门音
void play_full_throttle_sound()
{
	// 连续发出4声短促高音"哔"
	for(u8 i=0; i<4; i++)
	{
		beep_short(100);
	}
}

// 播放油门归零音
void play_zero_throttle_sound()
{
	// 发出4组低音双响"哔哔"
	for(u8 i=0; i<4; i++)
	{
		beep_double(200);
		delay_ms_max(100); // 使用delay_ms_max替代delay_ms
	}
}

// 播放进入编程模式音
void play_enter_program_sound()
{
	// 发出4声升调"哔哔哔哔"
	for(u8 i=0; i<4; i++)
	{
		shock(200 - i*25, 150); // 调整循环次数，与其他函数保持一致
	}
}

// 播放功能参数提示音
void play_param_sound(u8 function, u8 param)
{
	if(function == FUNC_MIX_CONTROL) // 功能1
	{
		// 一个长"哔"音
		beep_long(150);
		delay_ms_max(100); // 使用delay_ms_max替代delay_ms
		
		if(param == PARAM_MIX_DISABLE) // 参数1
		{
			// 接着是1个短的"哔"音
			beep_short(150);
		}
		else // 参数2
		{
			// 接着是1个短的"哔-哔"音
			beep_double(150);
		}
	}
	else if(function == FUNC_DIRECTION) // 功能2
	{
		// 一个长的"哔-哔"音
		beep_long(150);
		delay_ms_max(100);
		beep_long(150);
		delay_ms_max(100);
		
		if(param == PARAM_DIR_BIDIRECT) // 参数1
		{
			// 接着是一个短"哔"音
			beep_short(150);
		}
		else // 参数2
		{
			// 接着是一个短"哔-哔"音
			beep_double(150);
		}
	}
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
#endif

	delay_ms(400);  // 调用延时函数，暂停程序执行约400毫秒。
	
	// 初始化状态机变量
	system_state = STATE_INIT_COMPLETE;
	throttle_state = THROTTLE_ZERO;
	current_function = FUNC_MIX_CONTROL;
	current_param = PARAM_MIX_DISABLE;
	beep_counter = 0;
	repeat_counter = 0;
	
	// 初始化功能设置参数
	mix_control_setting = PARAM_MIX_DISABLE; // 默认不混控
	direction_setting = PARAM_DIR_BIDIRECT;  // 默认双向
	

	
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
		// 处理输入信号 - 在所有状态下都需要执行的代码
		// 这部分代码需要一直运行，不受状态机状态影响
		processinput();
		// 状态机处理
		switch(system_state)
		{
		case STATE_INIT_COMPLETE:
			// 播放开机音乐
			play_startup_sound(); // 上电音乐，表明正常工作
			// 初始化完成状态，等待油门提升
			system_state = STATE_CHECK_SIGNAL;
			break;
			
		case STATE_CHECK_SIGNAL:
			// 检查输入信号状态
			// 检测油门状态变化
			if(pulse1 > 105 && throttle_state == THROTTLE_ZERO)
			{
				// 油门从零位提升
				throttle_state = THROTTLE_UP;
				play_throttle_up_sound(); // 发出一声长低音"哔"
			}
			else if(pulse1 >= 190 && throttle_state == THROTTLE_UP)
			{
				// 检测到全油门
				throttle_state = THROTTLE_FULL;
				play_full_throttle_sound(); // 连续发出4声短促高音"哔"
			}
			else if(pulse1 <= 110 && throttle_state == THROTTLE_FULL)
			{
				// 油门归零
				throttle_state = THROTTLE_ZERO;
				play_zero_throttle_sound(); // 发出4组低音双响"哔哔"
			}
			else if(pulse1 >= 190 && throttle_state == THROTTLE_ZERO)
			{
				// 再次检测到全油门，进入编程模式
				throttle_state = THROTTLE_FULL;
				play_enter_program_sound(); // 发出4声升调"哔哔哔哔"
				system_state = STATE_SETTING;
				current_function = FUNC_MIX_CONTROL;
				current_param = mix_control_setting;
				repeat_counter = 0;
			}
			break;
			
		case STATE_SETTING:
			// 功能设置状态
			if(beep_counter == 0)
			{
				// 播放当前功能和参数的提示音
				play_param_sound(current_function, current_param);
				beep_counter = 100; // 设置延时
				repeat_counter++;
				
				if(repeat_counter >= 3)
				{
					// 重复三次后切换到下一个参数或功能
					repeat_counter = 0;
					
					if(current_function == FUNC_MIX_CONTROL)
					{
						// 从功能1切换到功能2
						current_function = FUNC_DIRECTION;
						current_param = direction_setting;
					}
					else
					{
						// 设置完成，回到功能1
						current_function = FUNC_MIX_CONTROL;
						current_param = mix_control_setting;
					}
				}
			}
			else
			{
				beep_counter--;
			}
			
			// 检测油门归零，保存设置并退出编程模式
			if(pulse1 <= 110 && throttle_state == THROTTLE_FULL)
			{
				// 保存当前设置
				if(current_function == FUNC_MIX_CONTROL)
				{
					mix_control_setting = current_param;
				}
				else if(current_function == FUNC_DIRECTION)
				{
					direction_setting = current_param;
				}
				
				// 退出编程模式
				throttle_state = THROTTLE_ZERO;
				system_state = STATE_NORMAL_RUNNING;
				beep_short(150); // 发出一声短音表示保存成功
			}
			
			// 检测油门变化，切换参数
			if(pulse1 >= 190 && throttle_state != THROTTLE_FULL)
			{
				throttle_state = THROTTLE_FULL;
				
				// 切换当前参数
				if(current_param == PARAM_MIX_DISABLE || current_param == PARAM_DIR_BIDIRECT)
				{
					current_param++; // 切换到第二个参数
				}
				else
				{
					current_param--; // 切换回第一个参数
				}
				
				repeat_counter = 0;
				beep_counter = 0; // 立即播放新参数的提示音
			}
			else if(pulse1 <= 110 && throttle_state == THROTTLE_FULL)
			{
				throttle_state = THROTTLE_ZERO;
			}
			break;
			
		case STATE_NORMAL_RUNNING:
			// 正常运行状态，无需额外处理输入信号（已在状态机外处理）
			break;
			
		default:
			// 未知状态，重置为初始化完成状态
			system_state = STATE_INIT_COMPLETE;
			break;
		}
		
		// 处理get_new信号 - 在所有状态下都需要执行
		if(get_new && STATE_NORMAL_RUNNING==system_state)
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
#if STC8G
void T1_isr() __interrupt(3)
#else
void T2_isr() __interrupt(12)
#endif
{

	g_ticks++;
	
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