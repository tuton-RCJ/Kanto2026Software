#include <Arduino.h>

// put function declarations here:
HardwareSerial uart1(PA10, PA9);

void setup()
{
  // put your setup code here, to run once:
  uart1.begin(115200);
}

void loop()
{
  // put your main code here, to run repeatedly:
  uart1.println("HelloWorld");
}

