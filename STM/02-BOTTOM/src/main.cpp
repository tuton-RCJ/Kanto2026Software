#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "colorsensor.h"

#define CS_RANGE PB5
#define CS_GATE PB3
#define CS_CK PB4
#define CS_DOUT PB8

// S9706 Color Sensor-------------------------------------------
Colorsensor colorsensor(CS_RANGE, CS_GATE, CS_CK, CS_DOUT);
int startTime;
int waitTime = 30;

// NEO PIXEL-----------------------------------------------------
const int ledPin = PD0;
const int numPixels = 2;
Adafruit_NeoPixel strip(numPixels, ledPin, NEO_GRB + NEO_KHZ800);
// const Adafruit_NeoPixel::Color led_color = Adafruit_NeoPixel::Color(0, 255, 0); // 光らせる色

// UART----------------------------------------------------------
HardwareSerial uart1(PA10, PA9);
HardwareSerial uart2(PA3, PA2);

void setup()
{
  AFIO->MAPR |= AFIO_MAPR_PD01_REMAP;
  strip.begin();
  strip.setBrightness(100);
  strip.fill(Adafruit_NeoPixel::Color(255, 255, 255)); // 全部指定した色にする
  strip.show();

  uart1.begin(115200);
  uart2.begin(115200);
}

void loop()
{
  colorsensor.start();
  startTime = millis();
  while (millis() - startTime < waitTime)
  {
    // 待機
  }
  colorsensor.end();

  uart1.print("R:");
  uart1.print(colorsensor.color[0]);
  uart1.print(" G:");
  uart1.print(colorsensor.color[1]);
  uart1.print(" B:");
  uart1.println(colorsensor.color[2]);
  uart2.print(colorsensor.color[0]);
  uart2.print(",");
  uart2.print(colorsensor.color[1]);
  uart2.print(",");
  uart2.println(colorsensor.color[2]);
}
