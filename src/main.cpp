#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <INA226.h>

// ===== 屏幕 =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===== 引脚（按文档）=====
#define OLED_SDA  PB7   // OLED SDA
#define OLED_SCL  PB6   // OLED SCL（INA226 同 I2C1）
#define BTN_NEXT  PA1   // 下一页按键（低电平触发）
#define BTN_PREV  PA2   // 上一页按键（低电平触发）
#define ONE_WIRE  PB0   // DS18B20 DQ

// ===== BMS 阈值 =====
#define V_FULL 4.2f     // 满电电压
#define V_LOW  3.0f     // 欠压截止电压

OneWire oneWire(ONE_WIRE);
DallasTemperature sensors(&oneWire);
INA226 ina226(0x40);

int page = 0;
const int NUM_PAGES = 3;

// ---- 读电池电压：用 INA226 直接测（量程 0~36V，无需分压电阻）----
float batteryVoltage() {
  return ina226.getBusVoltage();
}

// ---- 估算电量 %（3.0V=0%，4.2V=100%）----
int socPercent(float v) {
  int p = (int)((v - V_LOW) / (V_FULL - V_LOW) * 100.0f);
  if (p < 0) p = 0;
  if (p > 100) p = 100;
  return p;
}

// ---- 按键（低电平触发，带消抖）----
bool buttonPressed(int pin) {
  if (digitalRead(pin) == LOW) {
    delay(20);
    if (digitalRead(pin) == LOW) {
      while (digitalRead(pin) == LOW) delay(10);   // 等松开
      return true;
    }
  }
  return false;
}

void setup() {
  pinMode(PC13, OUTPUT);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);

  Wire.setSDA(OLED_SDA);
  Wire.setSCL(OLED_SCL);
  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1) { digitalWrite(PC13, !digitalRead(PC13)); delay(150); }
  }
  digitalWrite(PC13, HIGH);          // 灭板载 LED

  sensors.begin();
  sensors.setResolution(11);         // 温度 11 位

  ina226.begin();                    // INA226 默认 0x40
  ina226.setMaxCurrentShunt(2.0f, 0.1f);   // 2A 量程，0.1Ω 采样电阻

  Serial.begin(115200);              // USART1：PA9(TX) / PA10(RX)，接 CH340

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.display();
}

// 右上角页码
void pageIndicator(int p) {
  display.setTextSize(1);
  display.setCursor(104, 0);
  display.print(p + 1);
  display.print(F("/"));
  display.print(NUM_PAGES);
}

// ---- 第 1 页：电压 / 电流 ----
void drawPowerPage(float v, float i) {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Battery Monitor"));
  display.drawFastHLine(0, 11, 128, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print(v, 2);
  display.setTextSize(1);
  display.setCursor(76, 20);
  display.print(F("V"));

  display.setCursor(0, 36);
  display.print(F("I: "));
  display.print(i, 0);
  display.print(F(" mA"));

  display.setCursor(0, 46);
  if (i > 20.0f)       display.print(F("Charging"));
  else if (i < -20.0f) display.print(F("Discharging"));
  else                 display.print(F("Standby"));
}

// ---- 第 2 页：温度 ----
void drawTempPage(float t, int soc) {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Temperature"));
  display.drawFastHLine(0, 11, 128, SSD1306_WHITE);

  display.setTextSize(3);
  display.setCursor(0, 18);
  if (t > -100.0f) display.print(t, 1);
  else             display.print(F("--"));
  display.setTextSize(1);
  display.setCursor(80, 26);
  display.print((char)247);
  display.print(F("C"));

  display.setCursor(0, 50);
  display.print(F("SOC "));
  display.print(soc);
  display.print(F("%"));
}

// ---- 第 3 页：BMS 状态 ----
void drawBmsPage(float v, int soc) {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("BMS Status"));
  display.drawFastHLine(0, 11, 128, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 16);
  if (v >= V_FULL)     display.print(F("FULL"));
  else if (v <= V_LOW) display.print(F("LOW!"));
  else                 display.print(F("OK"));

  display.setTextSize(1);
  display.setCursor(0, 36);
  display.print(F("SOC "));
  display.print(soc);
  display.print(F("%"));

  int bar = map(soc, 0, 100, 0, 124);
  display.drawRect(0, 46, 128, 10, SSD1306_WHITE);
  display.fillRect(2, 48, bar, 6, SSD1306_WHITE);
}

void loop() {
  // 采集
  float v = batteryVoltage();
  float i = ina226.getCurrent() * 1000.0f;  // 安培 → 毫安
  sensors.requestTemperatures();
  float t = sensors.getTempCByIndex(0);
  int soc = socPercent(v);

  // 按键翻页
  if (buttonPressed(BTN_NEXT)) page = (page + 1) % NUM_PAGES;
  if (buttonPressed(BTN_PREV)) page = (page - 1 + NUM_PAGES) % NUM_PAGES;

  // 串口导出（电压,电流,温度）
  Serial.print(v, 2);
  Serial.print(F(","));
  Serial.print(i, 0);
  Serial.print(F(","));
  Serial.println(t, 1);

  // BMS 指示：满电常亮、欠压快闪、正常灭
  if (v >= V_FULL) {
    digitalWrite(PC13, LOW);
  } else if (v <= V_LOW) {
    digitalWrite(PC13, !digitalRead(PC13));
  } else {
    digitalWrite(PC13, HIGH);
  }

  // 显示
  display.clearDisplay();
  if (page == 0)      drawPowerPage(v, i);
  else if (page == 1) drawTempPage(t, soc);
  else                drawBmsPage(v, soc);
  pageIndicator(page);
  display.display();

  delay(200);
}
