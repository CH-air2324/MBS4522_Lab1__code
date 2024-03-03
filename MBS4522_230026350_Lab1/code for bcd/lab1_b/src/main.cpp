// 引入必要的库
#include <Arduino.h>

// 定义光敏电阻器（LDR）和LED的引脚
int ldrPin = 25;
int ledPin = 26;

int brightnessThreshold = 50;  // 亮度阈值

void setup() {
  // 初始化串行通信
  Serial.begin(9600);

  // 设置LED引脚为输出模式
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // 读取LDR的亮度值
  int sensorValue = analogRead(ldrPin);

  // 将亮度值转换为百分比
  int brightness = map(sensorValue, 0, 4095, 0, 100);

  // 打印亮度值到串行监视器
  Serial.print("Brightness: ");
  Serial.print(brightness);
  Serial.println("%");

  // 根据亮度值控制LED
  if (brightness < brightnessThreshold) {
    digitalWrite(ledPin, HIGH);  // 打开LED
  } else {
    digitalWrite(ledPin, LOW);   // 关闭LED
  }

  delay(1000);  // 延迟一秒钟
}