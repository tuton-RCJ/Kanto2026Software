#include "LED.h"
LED::LED(int pin, int numPixels) : strip(numPixels, pin, NEO_GRB + NEO_KHZ800)
{
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
}

void LED::setColor(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < strip.numPixels(); i++)
    {
        strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.show();
}

void LED::turnOff()
{
    for (int i = 0; i < strip.numPixels(); i++)
    {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.show();
}

void LED::setBrightness(uint8_t brightness)
{
    strip.setBrightness(brightness);
    strip.show();
}