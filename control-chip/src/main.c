#include "iostm8s103f3.h"

// 寄存器地址映射
#define REG_FLASH_DUKR    (*(volatile unsigned char *)0x5064)
#define REG_FLASH_IAPSR   (*(volatile unsigned char *)0x505F)
#define EEPROM_ADDR       ((volatile unsigned char *)0x4000)

// 引脚定义
#define BTN_PIN     3   // PC3
#define LED1_PIN    6   // PC6
#define LED2_PIN    5   // PC5
#define LED3_PIN    4   // PC4
#define LED4_PIN    7   // PC7

// 函数声明
void GPIO_Init(void);
void UART_Setup_2M_1200(void);
void UART_SendByte(unsigned char data);
void ApplyStatus(unsigned char status);
void SendCurrentStatus(void);
void EEPROM_Write(unsigned char data);
void delay_ms(unsigned int ms);

void main(void) {
    unsigned char btnCurrent, btnLast = 1;
    unsigned char savedStatus;

    GPIO_Init();
    UART_Setup_2M_1200();

    // 1. 上电恢复：从 EEPROM 读取位状态
    savedStatus = *EEPROM_ADDR;
    if(savedStatus > 0x0F) savedStatus = 0; // 初始检查
    ApplyStatus(savedStatus);
    
    delay_ms(500);
    SendCurrentStatus(); // 上电同步给 ESP32

    while(1) {
        // 2. 本地按键逻辑 (按键依然保留循环切换功能作为备用)
        btnCurrent = (PC_IDR & (1 << BTN_PIN)) ? 1 : 0;
        if(btnLast == 1 && btnCurrent == 0) {
            delay_ms(20);
            if(((PC_IDR & (1 << BTN_PIN)) ? 1 : 0) == 0) {
                // 简单的循环切换：获取当前状态并 +1 模拟切换
                unsigned char next = (*EEPROM_ADDR + 1) & 0x0F;
                ApplyStatus(next);
                EEPROM_Write(next);
                SendCurrentStatus();
                while(((PC_IDR & (1 << BTN_PIN)) ? 1 : 0) == 0);
            }
        }
        btnLast = btnCurrent;

        // 3. 接收 ESP32 二进制指令 [0xBB][Status]
        if(UART1_SR & 0x20) {
            static unsigned char rx_step = 0;
            unsigned char rx_data = UART1_DR;

            if(rx_step == 0 && rx_data == 0xBB) {
                rx_step = 1;
            } else if(rx_step == 1) {
                ApplyStatus(rx_data); // 直接应用位状态
                EEPROM_Write(rx_data); // 记忆状态
                SendCurrentStatus();  // 回传确认
                rx_step = 0;
            } else if(rx_data == 'T') { // 兼容旧的单字节触发
                unsigned char next = (*EEPROM_ADDR + 1) & 0x0F;
                ApplyStatus(next);
                EEPROM_Write(next);
                SendCurrentStatus();
            }
        }
        delay_ms(5);
    }
}

void ApplyStatus(unsigned char status) {
    if(status & 0x01) PC_ODR |= (1 << LED1_PIN); else PC_ODR &= ~(1 << LED1_PIN);
    if(status & 0x02) PC_ODR |= (1 << LED2_PIN); else PC_ODR &= ~(1 << LED2_PIN);
    if(status & 0x04) PC_ODR |= (1 << LED3_PIN); else PC_ODR &= ~(1 << LED3_PIN);
    if(status & 0x08) PC_ODR |= (1 << LED4_PIN); else PC_ODR &= ~(1 << LED4_PIN);
}

void SendCurrentStatus(void) {
    unsigned char s = 0;
    if(PC_ODR & (1 << LED1_PIN)) s |= 0x01;
    if(PC_ODR & (1 << LED2_PIN)) s |= 0x02;
    if(PC_ODR & (1 << LED3_PIN)) s |= 0x04;
    if(PC_ODR & (1 << LED4_PIN)) s |= 0x08;
    UART_SendByte(0xAA);
    UART_SendByte(s);
}

void EEPROM_Write(unsigned char data) {
    REG_FLASH_DUKR = 0xAE; REG_FLASH_DUKR = 0x56;
    while (!(REG_FLASH_IAPSR & 0x08));
    *EEPROM_ADDR = data;
    while (!(REG_FLASH_IAPSR & 0x04));
    REG_FLASH_IAPSR &= ~0x08;
}

void UART_Setup_2M_1200(void) {
    UART1_BRR2 = 0x02; UART1_BRR1 = 0x68; UART1_CR2 = 0x2C;
}

void UART_SendByte(unsigned char data) {
    while(!(UART1_SR & 0x80));
    UART1_DR = data;
}

void GPIO_Init(void) {
    PC_DDR &= ~(1 << BTN_PIN); PC_CR1 |= (1 << BTN_PIN);
    PC_DDR |= (1 << LED1_PIN)|(1 << LED2_PIN)|(1 << LED3_PIN)|(1 << LED4_PIN);
    PC_CR1 |= (1 << LED1_PIN)|(1 << LED2_PIN)|(1 << LED3_PIN)|(1 << LED4_PIN);
}

void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for(i = 0; i < ms; i++) for(j = 0; j < 250; j++) __asm__("nop");
}