#include "HomeSpan.h"
#include <Preferences.h>
#include <EasyButton.h>

#define SWITCH_CONTROL_PIN 0    // 总开关实体按钮（同时也是ESP32 BOOT按键）
#define LED_INDICATOR      2    // 状态LED，同时兼作总开关指示灯（低电平点亮）
#define DEFAULT_SETUP_CODE "46637726"  // HomeKit默认配对码
#define DEFAULT_QR_ID      "SWCH"      // HomeKit QR码ID

#define RX2_PIN  26
#define TX2_PIN   25
#define BAUD_RATE 1200

#define OUTLET1_BIT 0x01
#define OUTLET2_BIT 0x02
#define OUTLET3_BIT 0x04
#define OUTLET4_BIT 0x08

const int LONG_PRESS_MS = 5000;      // 长按触发恢复出厂设置的时长
EasyButton masterButton(SWITCH_CONTROL_PIN);

Preferences prefs;
uint8_t currentStatus = 0; // 维护本地状态位（bit0~3 对应 插座1~4）

void sendStatusToSTM8(uint8_t status) {
  Serial2.write(0xBB);   // 控制包头
  Serial2.write(status); // 目标状态位
}

void sendToggleToSTM8() {
  Serial2.write('T'); // 总开关切换指令
}

// ---------- 子插座服务 (Service::Outlet) ----------
struct DEV_Outlet : Service::Outlet {
  Characteristic::On outletOn{0};
  Characteristic::OutletInUse inUse{true};
  Characteristic::ConfiguredName *configuredName; // 支持在 HomeKit 中自定义名称
  uint8_t bitMask;

  DEV_Outlet(uint8_t mask, const char *defaultName) : Service::Outlet() {
    bitMask = mask;
    configuredName = new Characteristic::ConfiguredName(defaultName);
  }

  boolean update() override {
    if (outletOn.updated()) {
      uint8_t newStatus = outletOn.getNewVal() ? (currentStatus | bitMask) : (uint8_t)(currentStatus & ~bitMask);
      currentStatus = newStatus;
      sendStatusToSTM8(currentStatus); // 最终状态以 STM8 的 0xAA 回执为准
    }
    return true;
  }
};

// ---------- 总开关服务 (Service::Outlet) ----------
struct DEV_MasterOutlet : Service::Outlet {
  Characteristic::On outletOn{0};
  Characteristic::OutletInUse inUse{true};
  Characteristic::ConfiguredName *configuredName;

  DEV_MasterOutlet(const char *defaultName) : Service::Outlet() {
    configuredName = new Characteristic::ConfiguredName(defaultName);
  }

  boolean update() override {
    if (outletOn.updated()) {
      boolean wantOn = outletOn.getNewVal();
      if (wantOn != (currentStatus != 0)) sendToggleToSTM8(); // 目标状态与当前不一致才切换
    }
    return true;
  }

  void updateIndicatorLED() {
    digitalWrite(LED_INDICATOR, (currentStatus != 0) ? LOW : HIGH);
  }
};

DEV_MasterOutlet *masterOutlet;
DEV_Outlet       *subOutlet[4];

// 把 currentStatus 同步到全部 HomeKit 插座服务和指示灯（setVal 不会触发 update()）
void syncHomeKitSwitches() {
  bool isAnyOn = (currentStatus != 0);
  masterOutlet->outletOn.setVal(isAnyOn ? 1 : 0);
  masterOutlet->inUse.setVal(isAnyOn);
  masterOutlet->updateIndicatorLED();

  for (int i = 0; i < 4; i++) {
    bool isOn = (currentStatus & (1 << i)) != 0;
    subOutlet[i]->outletOn.setVal(isOn ? 1 : 0);
    subOutlet[i]->inUse.setVal(isOn);
  }
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
  Serial.println("======>初始化串口2");
  Serial2.begin(BAUD_RATE, SERIAL_8N1, RX2_PIN, TX2_PIN);

  Serial.println("======>prefs.begin");
  prefs.begin("led_mem", false);
  currentStatus = prefs.getUChar("bits", 0);

  Serial.println("======>masterButton.begin");
  masterButton.begin();
  masterButton.onPressed(onMasterButtonPressed);
  masterButton.onPressedFor(LONG_PRESS_MS, onMasterButtonLongPress);

  Serial.println("======>homeSpan.begin");
  homeSpan.setStatusPin(LED_INDICATOR);
  homeSpan.setQRID(DEFAULT_QR_ID);
  homeSpan.setPairingCode(DEFAULT_SETUP_CODE);
  
  // 设置为排插分类
  homeSpan.begin(Category::Outlets, "HONYAR智能排插");
  homeSpan.enableAutoStartAP();

  // ==================== 单个 Accessory，内部包含 5 个 Service ====================
  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Name("HONYAR 智能排插");
      new Characteristic::Manufacturer("XcuiTech Inc.");
      new Characteristic::Model("HONYAR-PowerStrip");
      new Characteristic::FirmwareRevision("1.0.0");
      new Characteristic::Identify();

    // 1. 服务一：总开关
    masterOutlet = new DEV_MasterOutlet("总开关");

    // 2. 服务二~五：4个独立分控插座
    const char *outletNames[4] = {"插座 1", "插座 2", "插座 3", "插座 4"};
    const uint8_t outletBits[4] = {OUTLET1_BIT, OUTLET2_BIT, OUTLET3_BIT, OUTLET4_BIT};
    
    for (int i = 0; i < 4; i++) {
      subOutlet[i] = new DEV_Outlet(outletBits[i], outletNames[i]);
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

    if      (cmd == "on1")    currentStatus |= OUTLET1_BIT;
    else if (cmd == "off1")   currentStatus &= ~OUTLET1_BIT;
    else if (cmd == "on2")    currentStatus |= OUTLET2_BIT;
    else if (cmd == "off2")   currentStatus &= ~OUTLET2_BIT;
    else if (cmd == "on3")    currentStatus |= OUTLET3_BIT;
    else if (cmd == "off3")   currentStatus &= ~OUTLET3_BIT;
    else if (cmd == "on4")    currentStatus |= OUTLET4_BIT;
    else if (cmd == "off4")   currentStatus &= ~OUTLET4_BIT;
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
        Serial.printf("插座%d:%s ", i, (currentStatus & (1<<(i-1))) ? "●" : "○");
      }
      Serial.println();
    }
  }
}