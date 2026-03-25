#include "TopLED.h"

TopLED::TopLED(int pin) : strip(20, pin, NEO_GRB + NEO_KHZ800)
{
    strip.begin();
    // Initialize all pixels to 'off'
    for (int i = 0; i < strip.numPixels(); i++)
    {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.show();
}

void TopLED::setCameraColor(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < 5; i++)
    {
        strip.setPixelColor(i, strip.Color(r, g, b));
    }
    for (int i = 15; i < 20; i++)
    {
        strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.show();
}

void TopLED::setVicimColor(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 5; i < 15; i++)
    {
        strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.show();
}

void TopLED::turnOff()
{
    for (int i = 0; i < strip.numPixels(); i++)
    {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    strip.show();
}

void TopLED::setBrightness(uint8_t brightness)
{
    strip.setBrightness(brightness);
    strip.show();
}