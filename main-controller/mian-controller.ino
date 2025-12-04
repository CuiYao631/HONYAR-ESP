// ESP32与STM8串口通信程序
// 使用Serial2 (RX2: GPIO16, TX2: GPIO17)

// 定义串口引脚 (ESP32默认Serial2引脚)
#define RX2_PIN 16  // GPIO16
#define TX2_PIN 17  // GPIO17

// 定义通信参数
#define BAUD_RATE 9600
#define BUFFER_SIZE 256

// 定义按钮引脚
#define BUTTON_PIN 0  // GPIO0 (BOOT按钮)

// LED状态管理
bool ledState = false;  // false=关灯, true=开灯
bool buttonPressed = false;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// 数据缓冲区
String receivedData = "";
bool dataReceived = false;

// 通信状态监控
unsigned long lastReceiveTime = 0;
unsigned long lastHeartbeatTime = 0;
unsigned long heartbeatInterval = 5000; // 5秒心跳
unsigned long receiveTimeout = 3000; // 3秒接收超时
bool stm8Connected = false;
int totalMessagesReceived = 0;
int corruptedMessages = 0;
int validMessages = 0;

void setup() {
  // 初始化USB串口监视器 (调试用)
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32-STM8串口通信初始化...");
  
  // 初始化Serial2与STM8通信
  // 参数: 波特率, 数据位8, 无奇偶校验, 停止位1
  Serial2.begin(BAUD_RATE, SERIAL_8N1, RX2_PIN, TX2_PIN);
  Serial.println("Serial2已初始化");
  Serial.printf("RX2引脚: GPIO%d\n", RX2_PIN);
  Serial.printf("TX2引脚: GPIO%d\n", TX2_PIN);
  Serial.printf("波特率: %d\n", BAUD_RATE);
  
  // 初始化GPIO0按钮
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("GPIO0按钮已初始化");
  
  // 初始化时间记录
  lastHeartbeatTime = millis();
  lastReceiveTime = millis();
  
  Serial.println("===== 系统初始化完成 =====");
  Serial.println("等待STM8连接...");
  
  // 发送初始化完成信号给STM8
  delay(100);
  sendToSTM8("ESP32_READY");
}

void loop() {
  // 检测通信状态
  checkCommunicationStatus();
  
  // 检测GPIO0按钮按下
  checkButtonPress();
  
  // 接收来自STM8的数据
  receiveFromSTM8();
  
  // 检查是否有来自串口监视器的命令 (用于测试)
  if (Serial.available()) {
    String command = Serial.readString();
    command.trim();
    
    if (command == "stats") {
      printDetailedStats();
    } else if (command == "clear") {
      // 清空统计
      totalMessagesReceived = 0;
      validMessages = 0;
      corruptedMessages = 0;
      Serial.println("🗑️ 统计已清空");
    } else {
      Serial.println("📝 手动发送给STM8: " + command);
      sendToSTM8(command);
    }
  }
  
  delay(10); // 短暂延时
}

// 接收来自STM8的数据
void receiveFromSTM8() {
  if (Serial2.available()) {
    Serial.print("📡 接收数据: ");
    
    // 读取所有可用数据
    while (Serial2.available()) {
      char c = Serial2.read();
      
      // 显示原始字符
      Serial.print("[");
      Serial.print(c);
      Serial.print("/0x");
      if (c < 16) Serial.print("0");
      Serial.print(c, HEX);
      Serial.print("] ");
      
      // 添加到接收缓冲区
      receivedData += c;
      
      // 更新接收时间
      lastReceiveTime = millis();
    }
    
    Serial.println();
    
    // 检查是否收到"test"
    if (receivedData.endsWith("test")) {
      Serial.println("✓ 收到STM8的test消息！");
      totalMessagesReceived++;
      validMessages++;
      stm8Connected = true;
      
      // 清空缓冲区
      receivedData = "";
    }
    
    // 防止缓冲区过长
    if (receivedData.length() > 100) {
      Serial.println("⚠️ 清空缓冲区（过长）");
      receivedData = "";
    }
  }
}

// 处理接收到的数据
void processReceivedData() {
  // 简化处理，不再需要，因为在receiveFromSTM8中已经处理
}

// 发送数据给STM8
void sendToSTM8(String data) {
  Serial2.println(data);
  Serial.println("发送给STM8: " + data);
}

// 处理传感器数据
void processSensorData(String sensorValue) {
  // 示例：处理温度数据
  float temperature = sensorValue.toFloat();
  
  if (temperature > 30.0) {
    Serial.println("温度过高，发送警告给STM8");
    sendToSTM8("TEMP_HIGH");
  } else if (temperature < 10.0) {
    Serial.println("温度过低，发送警告给STM8");
    sendToSTM8("TEMP_LOW");
  }
}

// 处理状态数据
void processStatusData(String status) {
  if (status == "OK") {
    Serial.println("STM8状态正常");
  } else if (status == "ERROR") {
    Serial.println("STM8报告错误，请求复位");
    sendToSTM8("RESET");
  } else if (status == "BUSY") {
    Serial.println("STM8忙碌中");
  }
}

// 发送控制命令给STM8的便捷函数
void sendCommand(String command, String parameter = "") {
  String fullCommand = command;
  if (parameter != "") {
    fullCommand += ":" + parameter;
  }
  sendToSTM8(fullCommand);
}

// 检测GPIO0按钮按下 (带防抖)
void checkButtonPress() {
  int reading = digitalRead(BUTTON_PIN);
  
  // 检测状态变化
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonPressed) {
      buttonPressed = reading;
      
      // 按钮按下 (LOW，因为使用了上拉电阻)
      if (buttonPressed == LOW) {
        toggleLED();
      }
    }
  }
  
  lastButtonState = reading;
}

// 切换LED状态
void toggleLED() {
  ledState = !ledState;
  
  if (ledState) {
    Serial.println("🔘 按钮按下: 请求开灯");
    sendToSTM8("LED_ON");
  } else {
    Serial.println("🔘 按钮按下: 请求关灯");
    sendToSTM8("LED_OFF");
  }
}

// LED控制函数
void controlLED(bool state) {
  if (state) {
    sendToSTM8("LED_ON");
  } else {
    sendToSTM8("LED_OFF");
  }
}

// 获取当前LED状态
bool getLEDState() {
  return ledState;
}

// 检查通信状态
void checkCommunicationStatus() {
  unsigned long currentTime = millis();
  
  // 检查是否超时未接收数据
  if (stm8Connected && (currentTime - lastReceiveTime) > receiveTimeout) {
    if (stm8Connected) {
      Serial.println("⚠️  STM8通信超时！尝试重新连接...");
      stm8Connected = false;
      sendToSTM8("ESP32_READY");
    }
  }
  
  // 定期显示统计信息
  static unsigned long lastStatsTime = 0;
  if ((currentTime - lastStatsTime) > 30000) { // 每30秒
    printDetailedStats();
    lastStatsTime = currentTime;
  }
}

void printStats() {
  Serial.println("\n===== 通信状态 =====");
  Serial.println("STM8连接: " + String(stm8Connected ? "✓ 已连接" : "✗ 断开"));
  Serial.println("已接收消息数: " + String(totalMessagesReceived));
  Serial.println("LED状态: " + String(ledState ? "🔆 开" : "🔅 关"));
  Serial.println("===================\n");
}

void printDetailedStats() {
  Serial.println("\n📊 详细通信统计:");
  Serial.println("  接收test消息: " + String(validMessages));
  Serial.println("  波特率: 9600 (固定)");
  Serial.println("  STM8状态: " + String(stm8Connected ? "在线" : "离线"));
  Serial.println("  当前缓冲区: [" + receivedData + "]");
  Serial.println();
}

