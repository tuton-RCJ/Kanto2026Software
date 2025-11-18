#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
class LED
{
public:
    LED(int pin, int numPixels);
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void turnOff();
    void setBrightness(uint8_t brightness);

private:
    Adafruit_NeoPixel strip;
};
#endif
