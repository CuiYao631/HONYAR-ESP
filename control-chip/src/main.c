#include "iostm8s103f3.h"

// 寄存器地址映射
#define REG_FLASH_DUKR    (*(volatile unsigned char *)0x5064)
#define REG_FLASH_IAPSR   (*(volatile unsigned char *)0x505F)
#define EEPROM_CUR_ADDR   ((volatile unsigned char *)0x4000) // 当前实际状态
#define EEPROM_MEM_ADDR   ((volatile unsigned char *)0x4001) // 关灯前的记忆状态

// 引脚定义
#define BTN_PIN     3   // PC3
#define LED1_PIN    6   // PC6
#define LED2_PIN    5   // PC5
#define LED3_PIN    4   // PC4
#define LED4_PIN    7   // PC7

static unsigned char currentStatus = 0; // 当前灯光位状态

void GPIO_Init(void);
void UART_Setup_2M_1200(void);
void UART_SendByte(unsigned char data);
void ApplyStatus(unsigned char status);
void SendCurrentStatus(void);
void EEPROM_Write(volatile unsigned char* addr, unsigned char data);
void delay_ms(unsigned int ms);

void main(void) {
    unsigned char btnCurrent, btnLast = 1;

    GPIO_Init();
    UART_Setup_2M_1200();

    // 1. 上电恢复：从 EEPROM 读取当前状态
    currentStatus = *EEPROM_CUR_ADDR;
    if(currentStatus > 0x0F) currentStatus = 0; 
    ApplyStatus(currentStatus);
    
    delay_ms(500);
    SendCurrentStatus(); 

    while(1) {
        // 2. 本地按键逻辑：总开关功能
        btnCurrent = (PC_IDR & (1 << BTN_PIN)) ? 1 : 0;
        if(btnLast == 1 && btnCurrent == 0) {
            delay_ms(20); // 消抖
            if(((PC_IDR & (1 << BTN_PIN)) ? 1 : 0) == 0) {
                
                if(currentStatus != 0) {
                    // 当前有灯亮 -> 执行【全关】，并【记忆】当前状态
                    EEPROM_Write(EEPROM_MEM_ADDR, currentStatus); // 记住这一刻的状态
                    currentStatus = 0; 
                } else {
                    // 当前全灭 -> 执行【开启】，恢复【记忆】的状态
                    currentStatus = *EEPROM_MEM_ADDR;
                    if(currentStatus == 0) currentStatus = 0x0F; // 如果记忆也是0，默认全开
                }
                
                ApplyStatus(currentStatus);
                EEPROM_Write(EEPROM_CUR_ADDR, currentStatus); // 保存当前实况
                SendCurrentStatus();
                
                while(((PC_IDR & (1 << BTN_PIN)) ? 1 : 0) == 0); // 等待释放
                delay_ms(20);
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
                currentStatus = rx_data;
                ApplyStatus(currentStatus);
                EEPROM_Write(EEPROM_CUR_ADDR, currentStatus);
                // 如果这次操作是点亮了灯，也更新一下“记忆地址”
                if(currentStatus != 0) EEPROM_Write(EEPROM_MEM_ADDR, currentStatus);
                SendCurrentStatus();
                rx_step = 0;
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
    UART_SendByte(0xAA);
    UART_SendByte(currentStatus);
}

void EEPROM_Write(volatile unsigned char* addr, unsigned char data) {
    REG_FLASH_DUKR = 0xAE; REG_FLASH_DUKR = 0x56;
    while (!(REG_FLASH_IAPSR & 0x08));
    *addr = data;
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