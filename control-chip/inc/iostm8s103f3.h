// STM8S103F3 寄存器定义
#ifndef IOSTM8S103F3_H
#define IOSTM8S103F3_H

// GPIO 寄存器定义 (Port D)
#define PD_ODR   (*(volatile unsigned char*)0x500F)  // 输出数据寄存器
#define PD_IDR   (*(volatile unsigned char*)0x5010)  // 输入数据寄存器  
#define PD_DDR   (*(volatile unsigned char*)0x5011)  // 数据方向寄存器
#define PD_CR1   (*(volatile unsigned char*)0x5012)  // 控制寄存器1
#define PD_CR2   (*(volatile unsigned char*)0x5013)  // 控制寄存器2

// UART1 寄存器定义
#define UART1_SR    (*(volatile unsigned char*)0x5230)  // 状态寄存器
#define UART1_DR    (*(volatile unsigned char*)0x5231)  // 数据寄存器
#define UART1_BRR1  (*(volatile unsigned char*)0x5232)  // 波特率寄存器1
#define UART1_BRR2  (*(volatile unsigned char*)0x5233)  // 波特率寄存器2
#define UART1_CR1   (*(volatile unsigned char*)0x5234)  // 控制寄存器1
#define UART1_CR2   (*(volatile unsigned char*)0x5235)  // 控制寄存器2
#define UART1_CR3   (*(volatile unsigned char*)0x5236)  // 控制寄存器3
#define UART1_CR4   (*(volatile unsigned char*)0x5237)  // 控制寄存器4
#define UART1_CR5   (*(volatile unsigned char*)0x5238)  // 控制寄存器5
#define UART1_GTR   (*(volatile unsigned char*)0x5239)  // 保护时间寄存器
#define UART1_PSCR  (*(volatile unsigned char*)0x523A)  // 预分频寄存器

// UART1 状态寄存器位定义
#define UART1_SR_TXE    0x80  // 发送数据寄存器空
#define UART1_SR_TC     0x40  // 传输完成
#define UART1_SR_RXNE   0x20  // 接收数据寄存器非空

// UART1 控制寄存器位定义
#define UART1_CR2_TEN   0x08  // 发送器使能
#define UART1_CR2_REN   0x04  // 接收器使能

#endif // IOSTM8S103F3_H