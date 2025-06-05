 对于stg8它有硬件3 路pwm，完全可以使用硬件PWM,  控制方式如下： 


CCPAM0（0xDA), 的bit1是控制pwm0是不是输出，bit6是允许模块进行上升沿捕获
CCPAM1（0xDB), 的bit1是控制pwm1是不是输出，  bit6是允许模块进行上升沿捕获
CCPAM2（0xDC), 的bit1是控制pwm2是不是输出，  bit6是允许模块进行上升沿捕获

PSW_1 寄存器 在0xA2这个地址，它的bit4 和 bit5是CCP_S， 是用来控制CCP的输出管脚的，详细如下：
PSW_1 CCP_S[1:0](b5-b4) 
00  CCP0->p3.2 CCP1->p3.3 CCP2->p5.4
01  CCP0->p3.1 CCP1->p3.3 CCP2->p5.4
10  CCP0->p3.2 CCP1->p3.3 CCP2->p5.5


对于， 双向双路电调控制的逻辑如下：

 场景      L1    H1    L2    H2
A正B正转:   0     P     0     P
CCP_S 00  p3.1  p3.2  p3.3   p5.4
A正B反转:   0     P     P     0
CCP_S 00  p3.1  p3.2  p3.3   p5.4      
A反B正转:   P     0     0     P   
CCP_S 01  p3.1  p3.2  p3.3   p5.4
A反B反转:   P     0     P      0 
CCP_S 01  p3.1  p3.2  p3.3   p5.4

使用7位PWM的一个全例子如下：
CCON= 0x00; //清空中断，停止pca计数
CMOD= 0x08; //0000 100 0 设置PCA的时钟为系统时钟
CL=0x0;
CH=0x0;   //是PCA的时钟计数，


CCAPPM0 = 0x42 ; //PCA模块0设为pwm工作模式
PCA_PWM0 = 0x40; //PCA模块0设为 7位PWM
CCAP0L=0x20;      //比较值
CCAP0H=0x20;     //重载值 ， 7位时CL 在7F后就溢出，归零， 所以占空比是75% （0x80-0x20)/0x80 ， 0x80表示CL的溢出计数。



把STC8G 的PWM 输出控制改成硬件方式的,输出脚为 p3.2 p3.3和p5.4, p5.5  PWM 输入改为p3.0和p3.1

对于， 双向双路电调控制的逻辑如下：

 场景      L1     H1     L2    H2
A正B正转:   0     P      0     P
CCP_S 10   p3.2  p3.3   p5.4 p5.5
             
A正B反转:   0     P     P     0
CCP_S 00  p3.2  p3.3   p5.4  p5.5


A反B正转:   P     0     0     P   
CCP_S 10  p3.2  p3.3   p5.4  p5.5

A反B反转:   P     0     P      0 
CCP_S 00  p3.2  p3.3   p5.4 p5.5

把STC8G 的PWM 输出控制改成硬件方式的,输出脚为 p3.2 p3.3和p5.4, p5.5  PWM 输入改为p3.0和p3.1,
A正B正转, CCP_S 设置为 10  p3.2 CCP0 输出固定pwm ==0,     p3.3 CCP1  输出pwm,      p5.4 gpio=0         p5.5 CCP2 输出 pwm
A正B反转, CCP_S 设置为 00  p3.2 CCP0 输出固定pwm ==0,     p3.3 CCP1  输出pwm,      p5.4 CCP2输出pwm,    p5.5 gpio=0
A反B正转,CCP_S  设置为 10  p3.2 CCP0 输出pwm,             p3.3 CCP1 输出pwm=0,    p5.4 gpio=0  ,       p5.5 CCP2 输出pwm 
A反B反转,CCP_S 设置为 00   p3.2 CCP0 输出pwm,              p3.3 CCP1 输出pwm=0,    p5.4  CCP2 输出pwm,   p5.5 gpio=0


CCMOD,  b0 是ecf, 是否允许PCA溢出中断
         b3 B2 B1 是CPS[2:0], 计数脉冲源选择, 000 系统时钟/12, 001 系统时钟/2 

         b6 b5 b4 没有使用
         b7 CIDL 空闲模式是否停止PCA计数
CCON,  b6是 CR 是否开始PCA计数

陶瓷低通

LFL152G45TC1A219

DEA162700LT

ACX1608
