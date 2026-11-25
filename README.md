# 光伏储能监测盒（PV Energy Storage Monitor）

以太阳能板为输入，经充电管理模块为 18650 锂电池充电，STM32F103C8T6 实时采集**电池电压、充电电流、温度**，通过 OLED 显示，支持按键切换页面与串口数据导出，并实现 BMS 过充过放保护逻辑。

一个项目、两个方向可用：集成电路（STM32 嵌入式、I2C/ADC/GPIO）、新能源（光伏储能、充放电管理、BMS）。

## 功能

- OLED 实时显示：电池电压 / 充电电流 / 温度（共 3 页，按键切换）
- 按键翻页：PA1 = 下一页，PA2 = 上一页
- BMS 保护逻辑：欠压 3.0V 截止放电、满电 4.2V 提示停止充电（板载 LED 指示）
- 串口导出：每 0.2 秒输出一行 `电压,电流,温度`

## 系统组成

- 子系统一（采集显示）：STM32 + OLED（SSD1306）+ INA226（电流/电压）+ DS18B20（温度）+ 按键
- 子系统二（光伏储能）：太阳能板 → 充电管理模块（CN3791/TP4056）→ 18650 锂电池 → 放电负载（USB 小风扇）

## 硬件接线（引脚定义）

| 模块 | 引脚 | 接到 |
|------|------|------|
| OLED（SSD1306, I2C） | SDA / SCL | PB7 / PB6 |
| INA226（电流/电压, I2C） | SDA / SCL | PB7 / PB6（与 OLED 共用 I2C） |
| INA226 | VIN+ / VIN- | 电池正极 / 负载 |
| DS18B20（温度） | DQ | PB0（DQ↔VCC 加 4.7kΩ 上拉） |
| 按键 1（下一页） | — | PA1 ↔ GND |
| 按键 2（上一页） | — | PA2 ↔ GND |
| 串口（CH340） | TX / RX | PA10 / PA9 |

## 开发环境

- 框架：PlatformIO + Arduino（STM32duino core）
- 芯片：STM32F103C8T6（Blue Pill）
- 依赖库：Adafruit SSD1306、Adafruit GFX、OneWire、DallasTemperature、Rob Tillaart INA226

## 烧录方式

- 方式一：ST-Link 拖拽烧录（复制 firmware.bin 到 ST-Link 的 MBED 盘）
- 方式二：ST-Link SWD（OpenOCD，老固件需用 HLA 模式）

## 实物照片

（待补充）

## 实测数据

（待补充：不同光照下的充电电压/电流）
