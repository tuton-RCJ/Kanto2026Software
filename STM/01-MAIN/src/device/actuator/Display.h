#ifndef Display_H
#define Display_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>

class Display
{
public:
    Display(int width, int height, int resetPin);
    void begin();
    void clear();
    void display();
    Adafruit_SSD1306 &getDisplay();

    // Run the full demo (blocking). This executes a sequence of drawing
    // routines and then starts the bitmap animation (infinite loop).
    void runDemo();

private:
    // Individual demo steps (moved from example sketch)
    void testdrawline();
    void testdrawrect();
    void testfillrect();
    void testdrawcircle();
    void testfillcircle();
    void testdrawroundrect();
    void testfillroundrect();
    void testdrawtriangle();
    void testfilltriangle();
    void testdrawchar();
    void testdrawstyles();
    void testscrolltext();
    void testdrawbitmap();
    void testanimate(const uint8_t *bitmap, uint8_t w, uint8_t h);

    Adafruit_SSD1306 _display;
};
#endif
