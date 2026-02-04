#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "ToF_VL53L4CX.h"

// put function declarations here:
HardwareSerial uart1(PA10, PA9);
HardwareSerial uart2(PA3, PA2);

#define LED_PIN PA14
#define LED_NUM 8
Adafruit_NeoPixel pixels(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

ToF_VL53L4CX tof_vl53l4cx;
#define uart uart2

byte seq;

byte value_changed;

void setup()
{
  uart1.begin(115200);
  pinMode(PC2, OUTPUT);
  digitalWrite(PC2, LOW); // Power off VL53L4CX
  pinMode(PC4, OUTPUT);
  digitalWrite(PC4, LOW); // Power off VL53L4CX
  pinMode(PC6, OUTPUT);
  digitalWrite(PC6, LOW); // Power off VL53L4CX

  Wire.setSCL(PB6);
  Wire.setSDA(PB7);
  Wire.setClock(800000);
  Wire.begin();
  Wire.setClock(800000);
  uart2.begin(115200);
  // tof.init();
  delay(50);
  tof_vl53l4cx.init();

  pixels.begin();
  pixels.show(); // Initialize all pixels to 'off'
  pixels.setBrightness(160);
  // for (int i = 0; i < LED_NUM; i++)
  // {
  //   pixels.setPixelColor(i, pixels.Color(255, 0, 0));
  // }
  // pixels.show();
  // delay(1000);
  // for (int i = 0; i < LED_NUM; i++)
  // {
  //   pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  // }
  // pixels.show();
  seq = 0;
  value_changed = 0x00;
}

void loop()
{
  value_changed |= tof_vl53l4cx.update(&uart1);
  if (value_changed == 0x0F)
  {
    uart1.println(millis());
    // for(int i=0;i<4;i++){
    //   uart1.print("ToF ");
    //   uart1.print(i);
    //   uart1.print(": ");
    //   uart1.print(tof_vl53l4cx.tof_values[i]);
    //   uart1.println(" mm");
    // }

    // データ送信
    // VL53L4CXはデータ受信に時間がかかるので、測定終了後に垂れ流す
    //---------------------------------------------------
    byte checksum = 0;
    // ヘッダー：0xFF 0xFF
    uart.write(0xFF);
    uart.write(0xFF);
    // シーケンス番号
    if (seq == 0xFE)
    {
      seq = 0;
    }
    else
    {
      seq++;
    }
    uart.write(seq);

    checksum ^= 0xFF;
    checksum ^= 0xFF;
    checksum ^= seq;
    for (int i = 0; i < 4; i++)
    {
      uint16_t value = tof_vl53l4cx.tof_values[i];
      uart.write((value >> 8) & 0xFF); // 上位バイト
      uart.write(value & 0xFE);        // 下位バイト
      checksum ^= (value >> 8) & 0xFF;
      checksum ^= value & 0xFE;
    }
    uart.write(checksum);
    //---------------------------------------------------
    value_changed = 0x00;
  }

  // コマンド受信
  //---------------------------------------------------
  if (uart.available())
  {
    byte data = uart.read();
    if (data == 0)
    {
      // byte checksum = 0; // すべてのXORを送信
      // // 4つのセンサの値をuart1に送信
      // for (int i = 0; i < tof_num; i++)
      // {
      //   uint16_t value = tof.tof_values[i];
      //   uart.write((value >> 8) & 0xFF); // 上位バイト
      //   uart.write(value & 0xFF);        // 下位バイト
      //   checksum ^= (value >> 8) & 0xFF;
      //   checksum ^= value & 0xFF;
      // }
      // uart.write(checksum);
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
  //---------------------------------------------------
}
