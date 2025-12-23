#include "Display.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>

// Constants and logo bitmap from the Adafruit SSD1306 example
#define NUMFLAKES 10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT 16
#define LOGO_WIDTH 16
static const unsigned char PROGMEM logo_bmp[] =
    {B00000000, B11000000,
     B00000001, B11000000,
     B00000001, B11000000,
     B00000011, B11100000,
     B11110011, B11100000,
     B11111110, B11111000,
     B01111110, B11111111,
     B00110011, B10011111,
     B00011111, B11111100,
     B00001101, B01110000,
     B00011011, B10100000,
     B00111111, B11100000,
     B00111111, B11110000,
     B01111100, B11110000,
     B01110000, B01110000,
     B00000000, B00110000};

Display::Display(int width, int height, int resetPin) : _display(width, height, &Wire, resetPin)
{
}

void Display::begin()
{
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    _display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    _display.clearDisplay();
    _display.display();
}

void Display::clear()
{
    _display.clearDisplay();
}

void Display::display()
{
    _display.display();
}

Adafruit_SSD1306 &Display::getDisplay()
{
    return _display;
}

// Public demo runner: executes the sequence from the Adafruit example.
// Note: this is blocking and at the end enters an infinite animation loop
// similar to the example sketch.
void Display::runDemo()
{
    Serial.begin(115200);

    // show initial buffer (Adafruit splash may be present)
    _display.display();
    delay(2000);

    _display.clearDisplay();

    // Draw a single pixel
    _display.drawPixel(10, 10, WHITE);
    _display.display();
    delay(2000);

    testdrawline();
    testdrawrect();
    testfillrect();
    testdrawcircle();
    testfillcircle();
    testdrawroundrect();
    testfillroundrect();
    testdrawtriangle();
    testfilltriangle();
    testdrawchar();
    testdrawstyles();
    testscrolltext();
    testdrawbitmap();

    // invert and restore
    _display.invertDisplay(true);
    delay(1000);
    _display.invertDisplay(false);
    delay(1000);

    testanimate(logo_bmp, LOGO_WIDTH, LOGO_HEIGHT);
}

// ----- Demo helper implementations (mostly copied/adapted from example) -----
void Display::testdrawline()
{
    int16_t i;

    _display.clearDisplay(); // Clear display buffer

    for (i = 0; i < _display.width(); i += 4)
    {
        _display.drawLine(0, 0, i, _display.height() - 1, WHITE);
        _display.display(); // Update screen with each newly-drawn line
        delay(1);
    }
    for (i = 0; i < _display.height(); i += 4)
    {
        _display.drawLine(0, 0, _display.width() - 1, i, WHITE);
        _display.display();
        delay(1);
    }
    delay(250);

    _display.clearDisplay();

    for (i = 0; i < _display.width(); i += 4)
    {
        _display.drawLine(0, _display.height() - 1, i, 0, WHITE);
        _display.display();
        delay(1);
    }
    for (i = _display.height() - 1; i >= 0; i -= 4)
    {
        _display.drawLine(0, _display.height() - 1, _display.width() - 1, i, WHITE);
        _display.display();
        delay(1);
    }
    delay(250);

    _display.clearDisplay();

    for (i = _display.width() - 1; i >= 0; i -= 4)
    {
        _display.drawLine(_display.width() - 1, _display.height() - 1, i, 0, WHITE);
        _display.display();
        delay(1);
    }
    for (i = _display.height() - 1; i >= 0; i -= 4)
    {
        _display.drawLine(_display.width() - 1, _display.height() - 1, 0, i, WHITE);
        _display.display();
        delay(1);
    }
    delay(250);

    _display.clearDisplay();

    for (i = 0; i < _display.height(); i += 4)
    {
        _display.drawLine(_display.width() - 1, 0, 0, i, WHITE);
        _display.display();
        delay(1);
    }
    for (i = 0; i < _display.width(); i += 4)
    {
        _display.drawLine(_display.width() - 1, 0, i, _display.height() - 1, WHITE);
        _display.display();
        delay(1);
    }

    delay(2000); // Pause for 2 seconds
}

void Display::testdrawrect(void)
{
    _display.clearDisplay();

    for (int16_t i = 0; i < _display.height() / 2; i += 2)
    {
        _display.drawRect(i, i, _display.width() - 2 * i, _display.height() - 2 * i, WHITE);
        _display.display(); // Update screen with each newly-drawn rectangle
        delay(1);
    }

    delay(2000);
}

void Display::testfillrect(void)
{
    _display.clearDisplay();

    for (int16_t i = 0; i < _display.height() / 2; i += 3)
    {
        // The INVERSE color is used so rectangles alternate white/black
        _display.fillRect(i, i, _display.width() - i * 2, _display.height() - i * 2, INVERSE);
        _display.display(); // Update screen with each newly-drawn rectangle
        delay(1);
    }

    delay(2000);
}

void Display::testdrawcircle(void)
{
    _display.clearDisplay();

    for (int16_t i = 0; i < max(_display.width(), _display.height()) / 2; i += 2)
    {
        _display.drawCircle(_display.width() / 2, _display.height() / 2, i, WHITE);
        _display.display();
        delay(1);
    }

    delay(2000);
}

void Display::testfillcircle(void)
{
    _display.clearDisplay();

    for (int16_t i = max(_display.width(), _display.height()) / 2; i > 0; i -= 3)
    {
        // The INVERSE color is used so circles alternate white/black
        _display.fillCircle(_display.width() / 2, _display.height() / 2, i, INVERSE);
        _display.display(); // Update screen with each newly-drawn circle
        delay(1);
    }

    delay(2000);
}

void Display::testdrawroundrect(void)
{
    _display.clearDisplay();

    for (int16_t i = 0; i < _display.height() / 2 - 2; i += 2)
    {
        _display.drawRoundRect(i, i, _display.width() - 2 * i, _display.height() - 2 * i,
                               _display.height() / 4, WHITE);
        _display.display();
        delay(1);
    }

    delay(2000);
}

void Display::testfillroundrect(void)
{
    _display.clearDisplay();

    for (int16_t i = 0; i < _display.height() / 2 - 2; i += 2)
    {
        // The INVERSE color is used so round-rects alternate white/black
        _display.fillRoundRect(i, i, _display.width() - 2 * i, _display.height() - 2 * i,
                               _display.height() / 4, INVERSE);
        _display.display();
        delay(1);
    }

    delay(2000);
}

void Display::testdrawtriangle(void)
{
    _display.clearDisplay();

    for (int16_t i = 0; i < max(_display.width(), _display.height()) / 2; i += 5)
    {
        _display.drawTriangle(
            _display.width() / 2, _display.height() / 2 - i,
            _display.width() / 2 - i, _display.height() / 2 + i,
            _display.width() / 2 + i, _display.height() / 2 + i, WHITE);
        _display.display();
        delay(1);
    }

    delay(2000);
}

void Display::testfilltriangle(void)
{
    _display.clearDisplay();

    for (int16_t i = max(_display.width(), _display.height()) / 2; i > 0; i -= 5)
    {
        // The INVERSE color is used so triangles alternate white/black
        _display.fillTriangle(
            _display.width() / 2, _display.height() / 2 - i,
            _display.width() / 2 - i, _display.height() / 2 + i,
            _display.width() / 2 + i, _display.height() / 2 + i, INVERSE);
        _display.display();
        delay(1);
    }

    delay(2000);
}

void Display::testdrawchar(void)
{
    _display.clearDisplay();

    _display.setTextSize(1);      // Normal 1:1 pixel scale
    _display.setTextColor(WHITE); // Draw white text
    _display.setCursor(0, 0);     // Start at top-left corner
    _display.cp437(true);         // Use full 256 char 'Code Page 437' font

    // Not all the characters will fit on the display. This is normal.
    for (int16_t i = 0; i < 256; i++)
    {
        if (i == '\n')
            _display.write(' ');
        else
            _display.write(i);
    }

    _display.display();
    delay(2000);
}

void Display::testdrawstyles(void)
{
    _display.clearDisplay();

    _display.setTextSize(1);      // Normal 1:1 pixel scale
    _display.setTextColor(WHITE); // Draw white text
    _display.setCursor(0, 0);     // Start at top-left corner
    _display.println(F("Hello, world!"));

    _display.setTextColor(BLACK, WHITE); // Draw 'inverse' text
    _display.println(3.141592);

    _display.setTextSize(2); // Draw 2X-scale text
    _display.setTextColor(WHITE);
    _display.print(F("0x"));
    _display.println(0xDEADBEEF, HEX);

    _display.display();
    delay(2000);
}

void Display::testscrolltext(void)
{
    _display.clearDisplay();

    _display.setTextSize(2); // Draw 2X-scale text
    _display.setTextColor(WHITE);
    _display.setCursor(10, 0);
    _display.println(F("scroll"));
    _display.display(); // Show initial text
    delay(100);

    // Scroll in various directions, pausing in-between:
    _display.startscrollright(0x00, 0x0F);
    delay(2000);
    _display.stopscroll();
    delay(1000);
    _display.startscrollleft(0x00, 0x0F);
    delay(2000);
    _display.stopscroll();
    delay(1000);
    _display.startscrolldiagright(0x00, 0x07);
    delay(2000);
    _display.startscrolldiagleft(0x00, 0x07);
    delay(2000);
    _display.stopscroll();
    delay(1000);
}

void Display::testdrawbitmap(void)
{
    _display.clearDisplay();

    _display.drawBitmap(
        (_display.width() - LOGO_WIDTH) / 2,
        (_display.height() - LOGO_HEIGHT) / 2,
        logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
    _display.display();
    delay(1000);
}

#define XPOS 0 // Indexes into the 'icons' array in function below
#define YPOS 1
#define DELTAY 2

void Display::testanimate(const uint8_t *bitmap, uint8_t w, uint8_t h)
{
    int8_t f, icons[NUMFLAKES][3];

    // Initialize 'snowflake' positions
    for (f = 0; f < NUMFLAKES; f++)
    {
        icons[f][XPOS] = random(1 - LOGO_WIDTH, _display.width());
        icons[f][YPOS] = -LOGO_HEIGHT;
        icons[f][DELTAY] = random(1, 6);
        Serial.print(F("x: "));
        Serial.print(icons[f][XPOS], DEC);
        Serial.print(F(" y: "));
        Serial.print(icons[f][YPOS], DEC);
        Serial.print(F(" dy: "));
        Serial.println(icons[f][DELTAY], DEC);
    }

    for (;;)
    {                            // Loop forever...
        _display.clearDisplay(); // Clear the display buffer

        // Draw each snowflake:
        for (f = 0; f < NUMFLAKES; f++)
        {
            _display.drawBitmap(icons[f][XPOS], icons[f][YPOS], bitmap, w, h, WHITE);
        }

        _display.display(); // Show the display buffer on the screen
        delay(200);         // Pause for 1/10 second

        // Then update coordinates of each flake...
        for (f = 0; f < NUMFLAKES; f++)
        {
            icons[f][YPOS] += icons[f][DELTAY];
            // If snowflake is off the bottom of the screen...
            if (icons[f][YPOS] >= _display.height())
            {
                // Reinitialize to a random position, just off the top
                icons[f][XPOS] = random(1 - LOGO_WIDTH, _display.width());
                icons[f][YPOS] = -LOGO_HEIGHT;
                icons[f][DELTAY] = random(1, 6);
            }
        }
    }
}
