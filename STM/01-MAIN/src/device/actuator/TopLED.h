#ifndef Top_LED_H
#define Top_LED_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
class TopLED
{
public:
    TopLED(int pin);
    void setCameraColor(uint8_t r, uint8_t g, uint8_t b);
    void setVicimColor(uint8_t r, uint8_t g, uint8_t b);
    void turnOff();
    void setBrightness(uint8_t brightness);

private:
    Adafruit_NeoPixel strip;
};
#endif
