#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "colorsensor.h"

// #define PhotoRefPin PA5_ALT0
#define PhotoRefPin1 PA4_ALT0
#define PhotoRefPin2 PA6_ALT0

#define CS_RANGE PB5
#define CS_GATE PB3
#define CS_CK PB4
#define CS_DOUT PB8

// S9706 Color Sensor-------------------------------------------
Colorsensor colorsensor(CS_RANGE, CS_GATE, CS_CK, CS_DOUT);
int startTime;
int waitTime = 30;

byte sensorData[3];
unsigned long measuringStartTime;

// NEO PIXEL-----------------------------------------------------
const int ledPin = PD0;
const int numPixels = 2;
Adafruit_NeoPixel strip(numPixels, ledPin, NEO_GRB + NEO_KHZ800);
// const Adafruit_NeoPixel::Color led_color = Adafruit_NeoPixel::Color(0, 255, 0); // 光らせる色

// UART----------------------------------------------------------
HardwareSerial uart1(PA10, PA9);
HardwareSerial uart2(PA3, PA2);

void checkRPi();
bool verifyCheckDigit(byte data[], int length, byte checkDigit);
void setup()
{
  uart1.begin(115200);
  // pinMode(PhotoRefPin, INPUT);
  // pinMode(PA2, OUTPUT);
  // return;

  pinMode(PhotoRefPin1, INPUT);
  pinMode(PhotoRefPin2, INPUT);

  AFIO->MAPR |= AFIO_MAPR_PD01_REMAP;
  strip.begin();
  strip.setBrightness(70);
  strip.fill(Adafruit_NeoPixel::Color(255, 255, 255)); // 全部指定した色にする
  strip.show();

  uart1.begin(115200);
  uart2.begin(115200);

  // 一回測光してからスタート
  colorsensor.start();
  measuringStartTime = millis();
  while (millis() - measuringStartTime < waitTime)
  {
    // wait
  }
  colorsensor.end();
  sensorData[0] = colorsensor.color[0];
  sensorData[1] = colorsensor.color[1];
  sensorData[2] = colorsensor.color[2];
  colorsensor.start();
}

void loop()
{

  if (millis() - measuringStartTime >= waitTime)
  {
    colorsensor.end();
    sensorData[0] = colorsensor.color[0];
    sensorData[1] = colorsensor.color[1];
    sensorData[2] = colorsensor.color[2];
    colorsensor.start();
    measuringStartTime = millis();
  }
  checkRPi();
  // uart1.print("R:");
  // uart1.print(colorsensor.color[0]);
  // uart1.print(" G:");
  // uart1.print(colorsensor.color[1]);
  // uart1.print(" B:");
  // uart1.println(colorsensor.color[2]);
  // uart2.print(colorsensor.color[0]);
  // uart2.print(",");
  // uart2.print(colorsensor.color[1]);
  // uart2.print(",");
  // uart2.println(colorsensor.color[2]);
  // uart1.print("R: ");
  // uart1.print(sensorData[0]);
  // uart1.print(" G: ");
  // uart1.print(sensorData[1]);
  // uart1.print(" B: ");
  // uart1.println(sensorData[2]);
}

void checkRPi()
{
  if (uart2.available() > 2)
  {
    uart1.println("Command Received");
    // uart1.println("Received Command from RPi");
    byte type = uart2.read();
    byte seq = uart2.read();
    if (type == 0x00)
    {
      while (uart2.available() < 1)
      {
        // wait for check digit
      }
      // センサーデータ送信
      byte CD = uart2.read();
      byte data[2] = {type, seq};
      if (verifyCheckDigit(data, 2, CD))
      {
        byte checkDigit = 0x00;
        uart2.write(0x00); // type
        uart2.write(seq);  // seq
        checkDigit ^= 0x00;
        checkDigit ^= seq;
        for (int i = 0; i < 3; i++)
        {
          uart2.write(sensorData[i]);
          checkDigit ^= sensorData[i];
        }
        byte _rf1 = analogRead(PhotoRefPin1) >> 2;
        byte _rf2 = analogRead(PhotoRefPin2) >> 2;
        uart2.write(_rf1);
        checkDigit ^= _rf1;
        uart2.write(_rf2);
        checkDigit ^= _rf2;
        uart2.write(checkDigit);
      }
      else
      {
        uart1.println("CheckDigit Error!");
      }
    }
    else if (type == 0x01)
    {
      while (uart2.available() < 5)
      {
        // wait for full command
      }
      // RGBをLEDに設定
      byte r = uart2.read();
      byte g = uart2.read();
      byte b = uart2.read();
      byte brightness = uart2.read();
      byte CD = uart2.read();
      byte data[6] = {type, seq, r, g, b, brightness};
      if (verifyCheckDigit(data, 6, CD))
      {
        strip.setBrightness(brightness);
        strip.fill(Adafruit_NeoPixel::Color(r, g, b)); // 全部指定した色にする
        strip.show();
        // 返答
        uart2.write(0x01);       // type
        uart2.write(seq);        // seq
        uart2.write(0x01 ^ seq); // CD
      }
      else
      {
        uart1.println("CheckDigit Error!");
      }
    }
    else
    {
      uart1.println("Unknown Command Type!");
      uart1.println(type);
    }
    while (uart2.available())
    {
      uart2.read();
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