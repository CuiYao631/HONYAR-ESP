// STM8 串口发送数据程序 - 寄存器直接操作版本
#include "iostm8s103f3.h"

// 通信参数定义
#define BAUD_RATE 9600
#define CPU_FREQ 16000000

// 函数声明
void SystemClockInit(void);
void UART_Setup(void);
void UART_SendByte(unsigned char data);
void UART_SendString(char* str);
void UART_SendHexData(unsigned char* data, unsigned char len);
void delay_ms(unsigned int ms);
void sendTestData(void);

// 定时发送控制
unsigned long lastSendTime = 0;
unsigned int sendInterval = 1000; // 1秒

// "test"的十六进制数据：t=0x74, e=0x65, s=0x73, t=0x74
unsigned char testData[] = {0x74, 0x65, 0x73, 0x74};
unsigned char dataLength = 4;

int main(void) {
    // 系统时钟初始化
    SystemClockInit();
    
    // 初始化UART1
    UART_Setup();
    
    // 等待1秒后开始发送
    delay_ms(1000);
    
    while(1) {
        // 定时发送测试数据
        sendTestData();
        delay_ms(10);
    }
}

// 系统时钟初始化
void SystemClockInit(void) {
    // 使用内部16MHz RC振荡器
    // 默认情况下STM8已经使用内部16MHz时钟
    // 这里可以保持为空或者进行额外配置
}

// UART1初始化函数 - 直接寄存器操作
void UART_Setup(void) {
    // 配置GPIO PD5(TX) PD6(RX)
    // PD5设为推挽输出，PD6设为浮空输入
    PD_DDR |= (1 << 5);  // PD5 输出
    PD_CR1 |= (1 << 5);  // PD5 推挽
    PD_CR2 |= (1 << 5);  // PD5 快速
    
    PD_DDR &= ~(1 << 6); // PD6 输入
    PD_CR1 &= ~(1 << 6); // PD6 浮空
    PD_CR2 &= ~(1 << 6); // PD6 无中断
    
    // UART1波特率配置：9600 @ 16MHz
    // BRR = 16000000 / 9600 = 1667 (0x0683)
    UART1_BRR2 = 0x03;
    UART1_BRR1 = 0x68;
    
    // UART1控制寄存器配置
    UART1_CR1 = 0x00;  // 8位数据，无奇偶校验
    UART1_CR2 = 0x0C;  // 使能发送和接收 (TEN=1, REN=1)
    UART1_CR3 = 0x00;  // 1个停止位
}

// UART1发送单个字节
void UART_SendByte(unsigned char data) {
    // 等待发送数据寄存器空 (TXE = 1)
    while(!(UART1_SR & 0x80));
    
    // 发送数据
    UART1_DR = data;
    
    // 等待发送完成 (TC = 1)
    while(!(UART1_SR & 0x40));
}

// UART1发送字符串
void UART_SendString(char* str) {
    while(*str) {
        UART_SendByte(*str);
        str++;
    }
}

// UART1发送十六进制数据
void UART_SendHexData(unsigned char* data, unsigned char len) {
    unsigned char i;
    for(i = 0; i < len; i++) {
        UART_SendByte(data[i]);
    }
}

// 简单的毫秒延时函数
void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for(i = 0; i < ms; i++) {
        for(j = 0; j < 1000; j++) {
            __asm__("nop");  // 空操作
        }
    }
}

// 发送测试数据
void sendTestData(void) {
    static unsigned long counter = 0;
    
    counter++;
    if(counter >= 100000) { // 约1秒间隔
        // 发送十六进制数据：0x74 0x65 0x73 0x74 ("test")
        UART_SendHexData(testData, dataLength);
        counter = 0;
    }
}

