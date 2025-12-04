// STM8 串口发送十六进制数据程序
#include "Arduino.h"

// 通信参数定义
#define BAUD_RATE 9600

// 函数声明
void serialInit(void);
void serialWrite(char c);
void sendHexData(void);

// 定时发送控制
unsigned long lastSendTime = 0;
unsigned long sendInterval = 1000; // 1秒

// "test"的十六进制数据：t=0x74, e=0x65, s=0x73, t=0x74
char testHexData[] = {0x74, 0x65, 0x73, 0x74};
int dataLength = 4;

void setup() {
    // 初始化串口通信
    serialInit();
    
    // 等待1秒后开始发送
    delay(1000);
}

void loop() {
    // 定时发送十六进制数据
    sendHexData();
    
    delay(10);
}

// STM8串口初始化函数
void serialInit() {
    // 配置PD5为TX引脚，PD6为RX引脚
    pinMode(PD5, OUTPUT);  // TX
    pinMode(PD6, INPUT);   // RX
    
    // 初始化TX引脚为高电平（空闲状态）
    digitalWrite(PD5, HIGH);
}

// 串口发送单个字节
void serialWrite(char c) {
    // 简单的软件串口发送实现
    // 使用位操作模拟串口发送
    digitalWrite(PD5, LOW);  // 起始位
    delayMicroseconds(104);  // 9600波特率延时 (1000000/9600)
    
    // 发送8个数据位
    for (int i = 0; i < 8; i++) {
        digitalWrite(PD5, (c >> i) & 0x01);
        delayMicroseconds(104);
    }
    
    digitalWrite(PD5, HIGH); // 停止位
    delayMicroseconds(104);
}

// 发送"test"的十六进制数据
void sendHexData() {
    unsigned long currentTime = millis();
    if (currentTime - lastSendTime >= sendInterval) {
        // 发送"test"的十六进制数据：0x74 0x65 0x73 0x74
        for (int i = 0; i < dataLength; i++) {
            serialWrite(testHexData[i]);
        }
        lastSendTime = currentTime;
    }
}

