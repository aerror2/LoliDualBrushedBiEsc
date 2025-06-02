# 萝丽双路双向电调 - B版源码

把原来的keil的语法改成了sdcc的写法，可以使用sdcc编译。
## 编译方法
1. 安装sdcc
2. 安装make和srecord工具
3. 运行make命令
4. 如果不安装make 工具, 也可以使用命令行编译:
   sdcc  LoliDualBrBiEsc.c -o LoliDualBrusedBiEsc.ihx
使用的mcu是STC15 系列的应该都可以, 只有需要6个IO就可以了， 两个IO输出，4个IO输出.
如STC15W104系列
