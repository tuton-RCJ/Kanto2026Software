#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
const int ledPin = PD0;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(2, ledPin, NEO_GRB + NEO_KHZ800);

HardwareSerial uart1(PA10, PA9);
void setup()
{
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  uart1.begin(115200);
}
void loop()
{
  uart1.println("Hello, World!");
  // 赤と青の交互点灯
  strip.setPixelColor(0, strip.Color(255, 0, 0)); // 赤
  strip.setPixelColor(1, strip.Color(0, 0, 255)); // 青
  strip.show();
  delay(500);
  strip.setPixelColor(0, strip.Color(0, 0, 255)); // 青
  strip.setPixelColor(1, strip.Color(255, 0, 0)); // 赤
  strip.show();
  delay(500);
}
