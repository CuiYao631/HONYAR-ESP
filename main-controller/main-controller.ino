#include <Preferences.h>

#define RX2_PIN 16
#define TX2_PIN 17
#define BOOT_BTN 0
#define BAUD_RATE 1200

Preferences prefs;
uint8_t currentStatus = 0; // 维护本地状态位
bool lastBtn = HIGH;

void setup() {
  Serial.begin(115200);
  Serial2.begin(BAUD_RATE, SERIAL_8N1, RX2_PIN, TX2_PIN);
  pinMode(BOOT_BTN, INPUT_PULLUP);
  
  prefs.begin("led_mem", false);
  currentStatus = prefs.getUChar("bits", 0);
  
  Serial.println("\n[系统就绪] 输入指令如: on1, off3, allon, alloff");
}

void loop() {
  // 1. 处理电脑串口输入的字符串指令
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim(); cmd.toLowerCase();
    
    uint8_t oldStatus = currentStatus;
    
    if      (cmd == "on1")    currentStatus |= 0x01;
    else if (cmd == "off1")   currentStatus &= ~0x01;
    else if (cmd == "on2")    currentStatus |= 0x02;
    else if (cmd == "off2")   currentStatus &= ~0x02;
    else if (cmd == "on3")    currentStatus |= 0x04;
    else if (cmd == "off3")   currentStatus &= ~0x04;
    else if (cmd == "on4")    currentStatus |= 0x08;
    else if (cmd == "off4")   currentStatus &= ~0x08;
    else if (cmd == "allon")  currentStatus = 0x0F;
    else if (cmd == "alloff") currentStatus = 0x00;
    
    if (currentStatus != oldStatus || cmd == "sync") {
      Serial2.write(0xBB);          // 发送控制包头
      Serial2.write(currentStatus); // 发送控制目标位
      Serial.printf("[发送] 目标状态更新: 0x%02X\n", currentStatus);
    }
  }

  // 2. 处理本地 Boot 按键 (循环触发)
  bool btn = digitalRead(BOOT_BTN);
  if (lastBtn == HIGH && btn == LOW) {
    delay(50);
    if (digitalRead(BOOT_BTN) == LOW) {
      Serial2.write('T'); // 发送切换指令
      while(digitalRead(BOOT_BTN) == LOW);
    }
  }
  lastBtn = btn;

  // 3. 解析 STM8 回传的 [0xAA][Status]
  if (Serial2.available() >= 2) {
    if (Serial2.read() == 0xAA) {
      currentStatus = Serial2.read();
      prefs.putUChar("bits", currentStatus); // 存入 NVS
      
      Serial.print("[STM8同步] ");
      for(int i=1; i<=4; i++) {
        Serial.printf("L%d:%s ", i, (currentStatus & (1<<(i-1))) ? "●" : "○");
      }
      Serial.println();
    }
  }
}