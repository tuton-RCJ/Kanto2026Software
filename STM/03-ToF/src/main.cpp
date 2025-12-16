#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "tof.h"

// put function declarations here:
HardwareSerial uart1(PA10, PA9);
HardwareSerial uart2(PA3, PA2);

#define LED_PIN PA14
#define LED_NUM 8
Adafruit_NeoPixel pixels(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

ToF tof;

#define uart uart2

void setup()
{
  uart1.begin(115200);
  Wire.setSCL(PB6);
  Wire.setSDA(PB7);
  Wire.begin();
  uart2.begin(115200);
  tof.init();
  pixels.begin();
  pixels.show(); // Initialize all pixels to 'off'
  pixels.setBrightness(160);
}

void loop()
{
  // uart1.println(millis());
  tof.update();
  // tof.print(&uart1);
  // return;
  // if (uart2.available())
  // {
  //   byte data = uart2.read();
  //   if (data == 0)
  //   {
  //     byte checksum = 0; // すべてのXORを送信
  //     // 4つのセンサの値をuart2に送信
  //     for (int i = 0; i < tof_num; i++)
  //     {
  //       uint16_t value = tof.tof_values[i];
  //       uart2.write((value >> 8) & 0xFF); // 上位バイト
  //       uart2.write(value & 0xFF);        // 下位バイト
  //       checksum ^= (value >> 8) & 0xFF;
  //       checksum ^= value & 0xFF;
  //     }
  //     uart2.write(checksum);
  //   }
  // }

  if (uart.available())
  {
    byte data = uart.read();
    if (data == 0)
    {
      byte checksum = 0; // すべてのXORを送信
      // 4つのセンサの値をuart1に送信
      for (int i = 0; i < tof_num; i++)
      {
        uint16_t value = tof.tof_values[i];
        uart.write((value >> 8) & 0xFF); // 上位バイト
        uart.write(value & 0xFF);        // 下位バイト
        checksum ^= (value >> 8) & 0xFF;
        checksum ^= value & 0xFF;
      }
      uart.write(checksum);
    }
    if (data == 1)
    {
      // タイムアウト0.5秒
      unsigned long startMillis = millis();
      // LEDを指定されたRGBに設定。チェックディジット（XOR）が一致を確認。
      while (uart.available() < 4)
      {
        if (millis() - startMillis > 500)
        {
          while (uart.available())
            uart.read();
          // タイムアウト
          return;
        }
      }
      byte r = uart.read();
      byte g = uart.read();
      byte b = uart.read();
      byte checksum = uart.read();
      if (checksum == (1 ^ r ^ g ^ b))
      {
        for (int i = 0; i < LED_NUM; i++)
        {
          pixels.setPixelColor(i, pixels.Color(r, g, b));
        }
        pixels.show();
      }
      // ACKを返す
      uart.write(0x01);
    }
    while (uart.available())
    {
      uart.read();
    }
  }
}