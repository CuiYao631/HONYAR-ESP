# ESP32-STM8串口通信项目

## 项目结构

```
HONYAR-ESP/
├── main-controller/          # ESP32主控代码
│   └── main-controller.ino   # ESP32 Arduino代码
├── control-chip/             # STM8控制芯片代码
│   ├── src/
│   │   └── main.c           # STM8主程序
│   ├── inc/
│   │   └── stm8s_conf.h     # STM8配置文件
│   └── Makefile             # STM8编译文件
└── README.md
```

## 硬件连接

### ESP32端
- RX2: GPIO16 (连接STM8的TX)
- TX2: GPIO17 (连接STM8的RX)
- GPIO0: 按钮输入 (用于测试LED控制)

### STM8端
- PD5: UART1_TX (连接ESP32的RX2)
- PD6: UART1_RX (连接ESP32的TX2)

### 连接方式
```
ESP32 GPIO16 (RX2) ←→ STM8 PD5 (TX)
ESP32 GPIO17 (TX2) ←→ STM8 PD6 (RX)
GND               ←→ GND
```

## 代码修复说明

### 主要修复内容

1. **STM8端修复**:
   - 替换Arduino框架为标准STM8库函数
   - 使用硬件UART1而非软件串口
   - 正确配置时钟和GPIO
   - 稳定的串口发送实现

2. **ESP32端修复**:
   - 改进数据接收处理逻辑
   - 添加数据验证和确认机制
   - 更好的错误处理和状态监控

### 通信协议

- **波特率**: 9600
- **数据位**: 8
- **停止位**: 1
- **奇偶校验**: 无

### 测试数据

STM8每秒发送"test"字符串，ESP32接收并验证数据正确性。

## 编译和上传

### ESP32
1. 使用Arduino IDE打开`main-controller/main-controller.ino`
2. 选择ESP32开发板
3. 编译并上传

### STM8
1. 确保安装了SDCC编译器
2. 在`control-chip`目录下运行:
   ```bash
   make
   ```
3. 使用STM8烧录器烧录生成的.ihx文件

## 调试说明

1. 打开ESP32的串口监视器（波特率115200）
2. 观察通信状态和接收数据
3. 输入命令进行测试：
   - `stats`: 显示详细统计信息
   - `clear`: 清空统计数据

## 预期输出

ESP32串口监视器应显示类似内容：
```
📡 接收数据: "test" (HEX: 0x74 0x65 0x73 0x74 )
✓ 收到STM8的test消息！数据正确
```

## 故障排除

1. **无数据接收**: 检查硬件连接和波特率设置
2. **数据乱码**: 检查时钟配置和UART参数
3. **间歇性通信**: 检查电源稳定性和接地
