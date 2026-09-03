#include "HomeSpan.h"
#include <Preferences.h>
#include <EasyButton.h>

#define SWITCH_CONTROL_PIN 0    // 总开关实体按钮（同时也是ESP32 BOOT按键）
#define LED_INDICATOR      2    // 状态LED，同时兼作总开关指示灯（低电平点亮）
#define DEFAULT_SETUP_CODE "46637726"  // HomeKit默认配对码
#define DEFAULT_QR_ID      "SWCH"      // HomeKit QR码ID

#define RX2_PIN   16
#define TX2_PIN   17
#define BAUD_RATE 1200

#define LED1_BIT 0x01
#define LED2_BIT 0x02
#define LED3_BIT 0x04
#define LED4_BIT 0x08

const int LONG_PRESS_MS = 5000;      // 长按触发恢复出厂设置的时长
EasyButton masterButton(SWITCH_CONTROL_PIN);

Preferences prefs;
uint8_t currentStatus = 0; // 维护本地状态位（bit0~3 对应 灯1~4）

void sendStatusToSTM8(uint8_t status) {
  Serial2.write(0xBB);   // 控制包头
  Serial2.write(status); // 目标状态位
}

void sendToggleToSTM8() {
  Serial2.write('T'); // 总开关切换指令
}

// ---------- 灯光子开关（无实体按钮） ----------
struct DEV_LightSwitch : Service::Switch {
  Characteristic::On switchOn{0};
  uint8_t bitMask;

  DEV_LightSwitch(uint8_t mask) : Service::Switch() {
    bitMask = mask;
  }

  boolean update() override {
    if (switchOn.updated()) {
      uint8_t newStatus = switchOn.getNewVal() ? (currentStatus | bitMask) : (uint8_t)(currentStatus & ~bitMask);
      currentStatus = newStatus;
      sendStatusToSTM8(currentStatus); // 最终状态以 STM8 的 0xAA 回执为准
    }
    return true;
  }
};

// ---------- 总开关（实体按钮 GPIO0，兼管指示灯） ----------
struct DEV_MasterSwitch : Service::Switch {
  Characteristic::On switchOn{0};

  boolean update() override {
    if (switchOn.updated()) {
      boolean wantOn = switchOn.getNewVal();
      if (wantOn != (currentStatus != 0)) sendToggleToSTM8(); // 目标状态与当前不一致才切换
    }
    return true;
  }

  void updateIndicatorLED() {
    digitalWrite(LED_INDICATOR, (currentStatus != 0) ? LOW : HIGH);
  }
};

DEV_MasterSwitch *masterSwitch;
DEV_LightSwitch  *lightSwitch[4];

// 把 currentStatus 同步到全部 HomeKit 开关和指示灯（setVal 不会触发 update()）
void syncHomeKitSwitches() {
  masterSwitch->switchOn.setVal(currentStatus != 0 ? 1 : 0);
  masterSwitch->updateIndicatorLED();
  lightSwitch[0]->switchOn.setVal((currentStatus & LED1_BIT) ? 1 : 0);
  lightSwitch[1]->switchOn.setVal((currentStatus & LED2_BIT) ? 1 : 0);
  lightSwitch[2]->switchOn.setVal((currentStatus & LED3_BIT) ? 1 : 0);
  lightSwitch[3]->switchOn.setVal((currentStatus & LED4_BIT) ? 1 : 0);
}

// 实体按钮短按：交由 STM8 处理开关/记忆逻辑，真实结果通过 0xAA 回执同步
void onMasterButtonPressed() {
  sendToggleToSTM8();
}

// 实体按钮长按：恢复出厂设置
void onMasterButtonLongPress() {
  Serial.println("🔄 长按检测到 - 恢复出厂设置");
  homeSpan.processSerialCommand("F");
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(BAUD_RATE, SERIAL_8N1, RX2_PIN, TX2_PIN);

  prefs.begin("led_mem", false);
  currentStatus = prefs.getUChar("bits", 0);

  masterButton.begin();
  masterButton.onPressed(onMasterButtonPressed);
  masterButton.onPressedFor(LONG_PRESS_MS, onMasterButtonLongPress);

  homeSpan.setStatusPin(LED_INDICATOR);
  homeSpan.setQRID(DEFAULT_QR_ID);
  homeSpan.setPairingCode(DEFAULT_SETUP_CODE);
  homeSpan.begin(Category::Bridges, "HONYAR灯光控制");

  // Accessory 1：桥接器本体（多配件模式下必须，本身不含任何功能服务）
  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Name("HONYAR灯光控制器");
      new Characteristic::Manufacturer("XcuiTech Inc.");
      new Characteristic::Model("HONYAR-Bridge");
      new Characteristic::FirmwareRevision("1.0.0");
      new Characteristic::Identify();

  // Accessory 2：总开关
  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Name("总开关");
      new Characteristic::Identify();
    masterSwitch = new DEV_MasterSwitch();

  // Accessory 3~6：灯1~灯4
  const char *lightNames[4] = {"灯1", "灯2", "灯3", "灯4"};
  const uint8_t lightBits[4] = {LED1_BIT, LED2_BIT, LED3_BIT, LED4_BIT};
  for (int i = 0; i < 4; i++) {
    new SpanAccessory();
      new Service::AccessoryInformation();
        new Characteristic::Name(lightNames[i]);
        new Characteristic::Identify();
      lightSwitch[i] = new DEV_LightSwitch(lightBits[i]);
  }

  syncHomeKitSwitches(); // 用已保存的状态初始化 HomeKit 显示和指示灯

  Serial.println("\n[系统就绪] 输入指令如: on1, off3, allon, alloff");
}

void loop() {
  homeSpan.poll();
  masterButton.read(); // EasyButton处理总开关按键

  // 1. 处理电脑串口输入的字符串指令
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim(); cmd.toLowerCase();

    uint8_t oldStatus = currentStatus;

    if      (cmd == "on1")    currentStatus |= LED1_BIT;
    else if (cmd == "off1")   currentStatus &= ~LED1_BIT;
    else if (cmd == "on2")    currentStatus |= LED2_BIT;
    else if (cmd == "off2")   currentStatus &= ~LED2_BIT;
    else if (cmd == "on3")    currentStatus |= LED3_BIT;
    else if (cmd == "off3")   currentStatus &= ~LED3_BIT;
    else if (cmd == "on4")    currentStatus |= LED4_BIT;
    else if (cmd == "off4")   currentStatus &= ~LED4_BIT;
    else if (cmd == "allon")  currentStatus = 0x0F;
    else if (cmd == "alloff") currentStatus = 0x00;

    if (currentStatus != oldStatus || cmd == "sync") {
      sendStatusToSTM8(currentStatus);
      Serial.printf("[发送] 目标状态更新: 0x%02X\n", currentStatus);
    }
  }

  // 2. 解析 STM8 回传的 [0xAA][Status]
  if (Serial2.available() >= 2) {
    if (Serial2.read() == 0xAA) {
      currentStatus = Serial2.read();
      prefs.putUChar("bits", currentStatus); // 存入 NVS
      syncHomeKitSwitches();                 // 同步到 HomeKit 显示状态和指示灯

      Serial.print("[STM8同步] ");
      for(int i=1; i<=4; i++) {
        Serial.printf("L%d:%s ", i, (currentStatus & (1<<(i-1))) ? "●" : "○");
      }
      Serial.println();
    }
  }
}
