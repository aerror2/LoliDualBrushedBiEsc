 对于stg8它有硬件3 路pwm，完全可以使用硬件PWM,  控制方式如下： 


CCPAM0（0xDA), 的bit1是控制pwm0是不是输出，bit6是允许模块进行上升沿捕获
CCPAM1（0xDB), 的bit1是控制pwm1是不是输出，  bit6是允许模块进行上升沿捕获
CCPAM2（0xDC), 的bit1是控制pwm2是不是输出，  bit6是允许模块进行上升沿捕获

PSW_1 寄存器 在0xA2这个地址，它的bit4 和 bit5是CCP_S， 是用来控制CCP的输出管脚的，详细如下：
PSW_1 CCP_S[1:0](b5-b4) 
    CCP0 CCP1 CCP2
00  p3.2 p3.3 p5.4
01  p3.1 p3.3 p5.4
10  p3.2 p3.3 p5.5


对于， 双向双路电调控制的逻辑如下：

          L1    H1    L2    H2
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
CCAP0H=0x20;     //重载值 ， 7位时CL 在7F后就溢出，归零， 所以占空比是75% （0x80-0x20)/0x80


把STC8G 的PWM 输出控制改成硬件方式的,输出脚为p3.1 p3.2 p3.3和p5.4,  PWM 输入改为p3.0和p5.5，
PSW_1 寄存器 在0xA2这个地址，它的bit4 和 bit5是CCP_S， 是用来控制CCP的输出管脚的，
电调控制的逻辑如下：
1. A正B正转,   CCP_S 设置为 00 ,  p3.1 拉低， CCP0 在p3.2输出pwm, p3.3 拉低，  CCP2在p5.4输出
2 .A正B反转，  CCP_S 设置为 00 ,  p3.1 拉低， CCP0 在p3.2输出pwm, CCP1在 p3.3 输出pwm，   p5.4拉低
3. A反B正转 ，CCP_S 设置为 01 ,   CCP0 在 p3.1 输出pwm,  p3.2 拉低,  p3.3 拉低 ,  CCP2在 p5.4输出
4. A反B反转,  CCP_S 设置为 01 ,   CCP0 在 p3.1 输出pwm,  p3.2 拉低, CCP1在 p3.3 输出pwm,   p5.4拉低