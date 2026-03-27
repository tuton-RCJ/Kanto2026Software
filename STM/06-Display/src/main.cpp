#include <Arduino.h>
#include "Adafruit_VL53L0X.h"
#include <SPI.h>
#include <Wire.h>
#include "Adafruit_NeoPixel.h"

#include "buzzer.h"
#include "TFT_eSPI.h"
#include "./img/logo.h"

HardwareSerial uart1(PA10, PA9);
Buzzer buzzer(PB8);
#define RE_LED_RED PB4
#define RE_LED_BLUE PB5

#define SW1 PB10
#define SW2 PB12
#define SW3 PB13
#define SW4 PB14

TFT_eSPI tft = TFT_eSPI();
#define TFT_BL PA8
void UpdateDisplayCoordinates(byte x, byte y, byte direction);
void DrawMessage(String message);
void DrawErrorMessage(String message);
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
uint16_t tof_distance;

bool verifyCheckDigit(byte data[], int length, byte checkDigit);

Adafruit_NeoPixel pixels(20, PB5, NEO_GRB + NEO_KHZ800);

void setup()
{
  uart1.begin(115200);
  buzzer.boot();
  pinMode(RE_LED_RED, OUTPUT);
  pinMode(RE_LED_BLUE, OUTPUT);
  digitalWrite(RE_LED_RED, LOW);
  digitalWrite(RE_LED_BLUE, LOW);
  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);
  pinMode(SW3, INPUT);
  pinMode(SW4, INPUT);

  // Display setup
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.init();
  tft.setRotation(1);
  // tft.fillScreen(TFT_BLACK);
  // tft.setSwapBytes(true);
  tft.pushImage(0, 0, 240, 240, image_data);

  Wire.setSDA(PB7);
  Wire.setSCL(PB6);
  Wire.begin();
  uart1.println("Adafruit VL53L0X test.");
  if (!lox.begin())
  {
    uart1.println(F("Failed to boot VL53L0X"));
    while (1)
      ;
  }
  lox.startRangeContinuous();
  delay(500);
  tft.fillScreen(TFT_WHITE);
  for (int i = 0; i < 3; i++)
  {
    tft.drawFastHLine(0, 40 + i, 240, TFT_BLACK);
  }

  UpdateDisplayCoordinates(0, 0, 0);
  DrawMessage("System Initialized");
  pixels.begin();
  pixels.setBrightness(50);
  for (int i = 0; i < 20; i++)
  {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0));      // 　オフ
  }
  // pixels.setBrightness(20);
  // for (int i = 5; i < 15; i++)
  // {
  //   pixels.setPixelColor(i, pixels.Color(255, 255, 255)); // 　オフ
  // }
  pixels.show();
}

void loop()
{

  if (lox.isRangeComplete())
  {
    // uart1.print("Distance in mm: ");
    tof_distance = lox.readRange();
    // uart1.println(lox.readRange());
  }
  if (digitalRead(SW1) == LOW)
  {
    DrawMessage("ToF: " + String(tof_distance) + " mm");
  }

  if (uart1.available() >= 2)
  {
    // 通信のメモ
    // 1バイト目：タイプ（ToF：0、ディスプレイ：1）
    // 2バイト目：シーケンス番号（0-255）
    byte type = uart1.read();
    byte seq = uart1.read();
    if (type == 0) // ToFセンサーのデータ要求
    {
      unsigned long startMillis = millis();
      while (uart1.available() < 1)
      {
        if (millis() - startMillis > 3000)
        { // タイムアウト3000ms
          DrawErrorMessage("Timeout - check digit");
          return;
        }
      } // チェックディジット受信まで待機

      byte checkdigit = uart1.read();
      byte data[2];
      data[0] = type;
      data[1] = seq;
      if (!verifyCheckDigit(data, 2, checkdigit))
      {
        // チェックディジットエラー
        DrawErrorMessage("Check digit error");
        while (uart1.available())
          uart1.read(); // バッファクリア
        return;
      }

      uart1.write(type);                                                                 // タイプ
      uart1.write(seq);                                                                  // シーケンス番号
      uart1.write((byte)(tof_distance >> 8));                                            // 上位バイト
      uart1.write((byte)(tof_distance & 0xFF));                                          // 下位バイト
      uart1.write(type ^ seq ^ (byte)(tof_distance >> 8) ^ (byte)(tof_distance & 0xFF)); // チェックディジット
      while (uart1.available())
        uart1.read(); // バッファクリア
    }
    else if (type == 1)
    {
      unsigned long startMillis = millis();
      while (uart1.available() < 4)
      {
        if (millis() - startMillis > 3000)
        { // タイムアウト3000ms
          DrawErrorMessage("Timeout - check digit");
          return;
        }
      } // チェックディジット受信まで待機
      byte x_coord = uart1.read();
      byte y_coord = uart1.read();
      byte direction = uart1.read();
      byte checkdigit = uart1.read();
      byte data[5];
      data[0] = type;
      data[1] = seq;
      data[2] = x_coord;
      data[3] = y_coord;
      data[4] = direction;
      if (!verifyCheckDigit(data, 5, checkdigit))
      {
        // チェックディジットエラー
        DrawErrorMessage("Check digit error");
        while (uart1.available())
          uart1.read(); // バッファクリア
        return;
      }

      UpdateDisplayCoordinates(x_coord, y_coord, direction);

      while (uart1.available())
        uart1.read();          // バッファクリア
      uart1.write(type);       // タイプ
      uart1.write(seq);        // シーケンス番号
      uart1.write(type ^ seq); // X座標
    }
    else if (type == 2)
    {
      unsigned long startMillis = millis();
      while (uart1.available() < 1)
      {
        if (millis() - startMillis > 3000)
        { // タイムアウト3000ms
          DrawErrorMessage("Timeout - data length");
          return;
        }
      } // データ長受信まで待機
      byte data_length = uart1.read();

      String message = "";
      for (int i = 0; i < data_length; i++)
      {
        startMillis = millis();
        while (uart1.available() < 1)
        {
          if (millis() - startMillis > 3000)
          { // タイムアウト3000ms
            DrawErrorMessage("Timeout - data");
            return;
          }
        } // データ受信まで待機
        message += (char)uart1.read();
      }

      startMillis = millis();
      while (uart1.available() < 1)
      {
        if (millis() - startMillis > 3000)
        { // タイムアウト3000ms
          DrawErrorMessage("Timeout - check digit");
          return;
        }
      } // チェックディジット受信まで待機
      byte checkdigit = uart1.read();

      byte data[3 + data_length];
      data[0] = type;
      data[1] = seq;
      data[2] = data_length;
      for (int i = 0; i < data_length; i++)
      {
        data[3 + i] = message[i];
      }
      if (!verifyCheckDigit(data, 3 + data_length, checkdigit))
      {
        // チェックディジットエラー
        DrawErrorMessage("Check digit error");
        while (uart1.available())
          uart1.read(); // バッファクリア
        return;
      }
      DrawMessage(message);
      while (uart1.available())
        uart1.read();          // バッファクリア
      uart1.write(type);       // タイプ
      uart1.write(seq);        // シーケンス番号
      uart1.write(type ^ seq); // チェックディジット
    }
    else if (type == 3)
    {
      // Buzzer Control Command
      unsigned long startTime = millis();
      while (uart1.available() < 1)
      {
        if (millis() - startTime > 100)
        {
          DrawErrorMessage("Timeout - music length");
          while (uart1.available())
          {
            uart1.read();
          }
          return;
        }
      }
      byte musicLength = uart1.read();
      constexpr int MAX_MUSIC_LEN = 300;
      NoteMillis notes[MAX_MUSIC_LEN];
      int effectiveLength = (int)musicLength;
      if (effectiveLength > MAX_MUSIC_LEN)
      {
        effectiveLength = MAX_MUSIC_LEN;
      }

      byte CD = 0x04 ^ seq ^ musicLength;
      for (int i = 0; i < musicLength; i++)
      {
        unsigned long noteStart = millis();
        while (uart1.available() < 4)
        {
          if (millis() - noteStart > 200)
          {
            DrawErrorMessage("Timeout - note data");
            while (uart1.available())
            {
              uart1.read();
            }
            return;
          }
        }
        byte note_h = uart1.read();
        byte note_l = uart1.read();
        byte dur_h = uart1.read();
        byte dur_l = uart1.read();
        if (i < effectiveLength)
        {
          notes[i].note = ((int)note_h << 8) | (int)note_l;
          notes[i].duration = ((int)dur_h << 8) | (int)dur_l;
        }
        CD ^= note_h ^ note_l ^ dur_h ^ dur_l;
      }
      unsigned long cdStart = millis();
      while (uart1.available() < 1)
      {
        if (millis() - cdStart > 200)
        {
          DrawErrorMessage("Timeout - check digit");
          while (uart1.available())
          {
            uart1.read();
          }
          return;
        }
      }
      byte receivedCD = uart1.read();
      if (CD == receivedCD)
      {
        DrawMessage("CheckDigit OK!");
        if (effectiveLength <= 0)
        {
          buzzer.mute();
        }
        else
        {
          buzzer.RegisterMusic(notes, effectiveLength);
        }
        // 返答
        uart1.write(type);       // type
        uart1.write(seq);        // seq
        uart1.write(type ^ seq); // CD
      }
      else
      {
        DrawErrorMessage("CheckDigit Error!");
      }
    }
    else if (type == 4)
    {
      // Cam LED Control Command
      unsigned long startMillis = millis();
      while (uart1.available() < 4)
      {
        if (millis() - startMillis > 3000)
        { // タイムアウト3000ms
          DrawErrorMessage("Timeout - check digit");
          return;
        }
      }
      byte red = uart1.read();
      byte green = uart1.read();
      byte blue = uart1.read();
      byte checkdigit = uart1.read();
      byte data[5];
      data[0] = type;
      data[1] = seq;
      data[2] = red;
      data[3] = green;
      data[4] = blue;
      if (!verifyCheckDigit(data, 5, checkdigit))
      {
        // チェックディジットエラー
        DrawErrorMessage("Check digit error");
        while (uart1.available())
          uart1.read(); // バッファクリア
        return;
      }
      for (int i = 0; i < 5; i++)
      {
        pixels.setPixelColor(i, pixels.Color(red, green, blue));
      }
      for (int i = 15; i < 20; i++)
      {
        pixels.setPixelColor(i, pixels.Color(red, green, blue));
      }
      pixels.show();

      // 返答
      uart1.write(type);       // type
      uart1.write(seq);        // seq
      uart1.write(type ^ seq); // CD
    }
    else if (type == 5)
    {
      // Victim LED Control Command
      // Cam LED Control Command
      unsigned long startMillis = millis();
      while (uart1.available() < 4)
      {
        if (millis() - startMillis > 3000)
        { // タイムアウト3000ms
          DrawErrorMessage("Timeout - check digit");
          return;
        }
      }
      byte red = uart1.read();
      byte green = uart1.read();
      byte blue = uart1.read();
      byte checkdigit = uart1.read();
      byte data[5];
      data[0] = type;
      data[1] = seq;
      data[2] = red;
      data[3] = green;
      data[4] = blue;
      if (!verifyCheckDigit(data, 5, checkdigit))
      {
        // チェックディジットエラー
        DrawErrorMessage("Check digit error");
        while (uart1.available())
          uart1.read(); // バッファクリア
        return;
      }
      for (int i = 5; i < 15; i++)
      {
        pixels.setPixelColor(i, pixels.Color(red, green, blue));
      }
      pixels.show();

      // 返答
      uart1.write(type);       // type
      uart1.write(seq);        // seq
      uart1.write(type ^ seq); // CD
    }
    else
    {
      // 未知のタイプ
      DrawErrorMessage("Unknown type");
      while (uart1.available())
        uart1.read(); // バッファクリア
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
  // DrawErrorMessage("Check digit: " + String(cd, HEX));
  return (cd == checkDigit);
}

void UpdateDisplayCoordinates(byte x, byte y, byte direction)
{
  tft.fillRect(0, 50, 240, 200, TFT_WHITE);

  tft.setTextColor(TFT_BLACK);
  tft.setTextFont(7);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(String(x), 60, 100);
  tft.drawString(String(y), 180, 100);

  // 方角の配列
  String directions[] = {"NORTH", "EAST", "SOUTH", "WEST"};
  tft.setTextFont(4);
  // tft.setTextSize(2);
  tft.drawString(directions[direction], 120, 180);
}

void DrawMessage(String message)
{
  tft.fillRect(0, 0, 240, 40, TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  tft.setTextFont(4);
  tft.setCursor(10, 10);
  tft.print(message);
}

void DrawErrorMessage(String message)
{
  tft.fillRect(0, 200, 240, 240, TFT_WHITE);
  tft.setTextColor(TFT_RED);
  tft.setTextFont(2);
  tft.setCursor(10, 220);
  tft.print(message);
}