#include "Adafruit_VL53L0X.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

uint16_t tof_distance;

bool verifyCheckDigit(byte data[], int length, byte checkDigit);
void setup()
{
  Serial.begin(115200);
  // wait until serial port opens for native USB devices
  // while (!Serial)
  // {
  //   delay(1);
  // }
  // Display setup
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    // Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  display.display();
  delay(1000);
  display.clearDisplay();
  display.display();

  // Serial.println("Adafruit VL53L0X test.");
  if (!lox.begin())
  {
    // Serial.println(F("Failed to boot VL53L0X"));
    while (1)
      ;
  }
  // power
  // Serial.println(F("VL53L0X API Continuous Ranging example\n\n"));

  // start continuous ranging
  lox.startRangeContinuous();
}

void loop()
{
  if (lox.isRangeComplete())
  {
    // Serial.print("Distance in mm: ");
    tof_distance = lox.readRange();
    // Serial.println(lox.readRange());
  }

  if (Serial.available()>=2)
  {
    // 通信のメモ
    // 1バイト目：タイプ（ToF：0、ディスプレイ：1）
    // 2バイト目：シーケンス番号（0-255）
    byte type = Serial.read();
    byte seq = Serial.read();
    if (type == 0) // ToFセンサーからのデータ要求
    {
      unsigned long startMillis = millis();
      while(Serial.available()<1){
        if(millis()-startMillis>3000){ // タイムアウト3000ms
          // Serial.println("Timeout waiting for check digit");
          return;
        }
      } // チェックディジット受信まで待機

      byte checkdigit = Serial.read();
      byte data[2];
      data[0] = type;
      data[1] = seq;
      if (!verifyCheckDigit(data, 2, checkdigit))
      {
        // チェックディジットエラー
        // Serial.println("Check digit error");
        while (Serial.available())
          Serial.read(); // バッファクリア
        return;
      }
      Serial.write(type);                                                                 // タイプ
      Serial.write(seq);                                                                  // シーケンス番号
      Serial.write((byte)(tof_distance >> 8));                                            // 上位バイト
      Serial.write((byte)(tof_distance & 0xFF));                                          // 下位バイト
      Serial.write(type ^ seq ^ (byte)(tof_distance >> 8) ^ (byte)(tof_distance & 0xFF)); // チェックディジット
      while (Serial.available())
        Serial.read(); // バッファクリア
    }
    if (type == 1)
    {
      unsigned long startMillis = millis();
      while(Serial.available()<4){
        if(millis()-startMillis>3000){ // タイムアウト3000ms
          // Serial.println("Timeout waiting for check digit");
          return;
        }
      } // チェックディジット受信まで待機
      byte x_coord = Serial.read();
      byte y_coord = Serial.read();
      byte direction = Serial.read();
      byte checkdigit = Serial.read();
      byte data[5];
      data[0] = type;
      data[1] = seq;
      data[2] = x_coord;
      data[3] = y_coord;
      data[4] = direction;
      if (!verifyCheckDigit(data, 5, checkdigit))
      {
        // チェックディジットエラー
        // Serial.println("Check digit error");
        while (Serial.available())
          Serial.read(); // バッファクリア
        return;
      }
      // OLED表示更新
      display.clearDisplay();
      display.clearDisplay();
      display.setTextSize(2);              // Normal 1:1 pixel scale
      display.setTextColor(SSD1306_WHITE); // Draw white text
      display.setCursor(20, 15);           // Start at top-left corner
      display.print("X:");
      display.print(x_coord);
      display.print(" Y:");
      display.print(y_coord);
      display.setCursor(20, 40);
      // 方角の配列
      String directions[] = {"North", "East", "South", "West"};
      // display.print("Dir:");
      display.print(directions[direction]);
      display.display();
      while (Serial.available())
        Serial.read();          // バッファクリア
      Serial.write(type);       // タイプ
      Serial.write(seq);        // シーケンス番号
      Serial.write(type ^ seq); // X座標
    }
    else{
      // 未知のタイプ
      while (Serial.available())
        Serial.read(); // バッファクリア
      return;
    }
  }
}

bool verifyCheckDigit(byte data[], int length, byte checkDigit)
{
  // XORが一致するか確認
  byte cd = 0x00;
  for (int i = 0; i < length; i++)
  {
    cd ^= data[i];
  }
  return (cd == checkDigit);
}
