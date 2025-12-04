# STM8项目Makefile

# 编译器设置
CC = sdcc
CFLAGS = -mstm8 -lstm8 --opt-code-size

# 目录设置
SRC_DIR = src
INC_DIR = inc
BUILD_DIR = build

# 源文件
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.rel)

# 目标文件
TARGET = stm8_uart

# 包含路径
INCLUDES = -I$(INC_DIR) -I/usr/local/share/sdcc/include/stm8

# 默认目标
all: $(BUILD_DIR) $(TARGET).ihx

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 编译规则
$(BUILD_DIR)/%.rel: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 链接规则
$(TARGET).ihx: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

# 清理
clean:
	rm -rf $(BUILD_DIR) *.ihx *.lk *.map *.mem *.rst *.sym

# 烧录（需要根据具体的烧录器调整）
flash: $(TARGET).ihx
	@echo "Please use your STM8 programmer to flash $(TARGET).ihx"

.PHONY: all clean flash