#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

const int numPixels = 1;
const int pixelPin = PA14;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(numPixels, pixelPin, NEO_GRB + NEO_KHZ800);
HardwareSerial uart1(PA10, PA9);
const int BuzzerPin = PB8;

// 音階定義（周波数[Hz]）
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define NOTE_A5 880
#define NOTE_B5 988

void PlayYuuyakeKoyake();
void ColorfulLED();

void setup()
{
  uart1.begin(115200);
  pinMode(BuzzerPin, OUTPUT);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop()
{
  ColorfulLED();
  PlayYuuyakeKoyake();
  delay(5000); // 次の演奏までの休み
}

void ColorfulLED()
{
  for (int i = 0; i < numPixels; i++)
  {
    strip.setPixelColor(i, strip.Color(255, 0, 0)); // 赤
    strip.show();
    delay(200);
    strip.setPixelColor(i, strip.Color(0, 255, 0)); // 緑
    strip.show();
    delay(200);
    strip.setPixelColor(i, strip.Color(0, 0, 255)); // 青
    strip.show();
    delay(200);
    strip.setPixelColor(i, strip.Color(255, 255, 0)); // 黄
    strip.show();
    delay(200);
    strip.setPixelColor(i, strip.Color(0, 255, 255)); // シアン
    strip.show();
    delay(200);
    strip.setPixelColor(i, strip.Color(255, 0, 255)); // マゼンタ
    strip.show();
    delay(200);
    strip.setPixelColor(i, strip.Color(255, 255, 255)); // 白
    strip.show();
    delay(200);
    strip.setPixelColor(i, strip.Color(0, 0, 0)); // 消灯
    strip.show();
    delay(200);
  }
}

// 「夕焼け小焼け」メロディデータ
void PlayYuuyakeKoyake()
{
  // ♪夕焼け小焼けで日が暮れて…
  int melody[] = {
      NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5, NOTE_B4, NOTE_A4, NOTE_G4,
      NOTE_A4, NOTE_B4, NOTE_C5, NOTE_D5, NOTE_C5, NOTE_B4, NOTE_A4,
      NOTE_B4, NOTE_C5, NOTE_D5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_B4,
      NOTE_C5, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_E5, NOTE_D5, NOTE_C5};

  int noteDurations[] = {
      4, 4, 4, 2, 4, 4, 2,
      4, 4, 4, 2, 4, 4, 2,
      4, 4, 4, 2, 4, 4, 2,
      4, 4, 4, 2, 4, 4, 2};

  int notes = sizeof(melody) / sizeof(melody[0]);
  for (int i = 0; i < notes; i++)
  {
    int duration = 500 / noteDurations[i];
    tone(BuzzerPin, melody[i], duration);
    delay(duration * 1.3);
    noTone(BuzzerPin);
  }
}
