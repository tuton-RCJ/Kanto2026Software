#include <Arduino.h>

// put function declarations here:
HardwareSerial uart1(PA10, PA9);
HardwareSerial uart2(PA3, PA2);
#include "tof.h"

ToF tof;

void setup()
{
  uart1.begin(115200);
  Wire.setSCL(PB6);
  Wire.setSDA(PB7);
  Wire.begin();
  uart2.begin(115200);
  tof.init();
}

void loop()
{
  tof.getTofValues();
  // tof.print(&uart1);
  if (uart2.available())
  {
    byte data = uart2.read();
    if (data == 0)
    {
      byte checksum = 0; //すべてのXORを送信
      // 4つのセンサの値をuart2に送信
      for (int i = 0; i < tof_num; i++)
      {
        uint16_t value = tof.tof_values[i];
        uart2.write((value >> 8) & 0xFF); // 上位バイト
        uart2.write(value & 0xFF);        // 下位バイト
        checksum ^= (value >> 8) & 0xFF;
        checksum ^= value & 0xFF;
      }
      uart2.write(checksum);
    }
  }
  if (uart1.available())
  {
    byte data = uart1.read();
    if (data == 0)
    {
      byte checksum = 0; //すべてのXORを送信
      // 4つのセンサの値をuart1に送信
      for (int i = 0; i < tof_num; i++)
      {
        uint16_t value = tof.tof_values[i];
        uart1.write((value >> 8) & 0xFF); // 上位バイト
        uart1.write(value & 0xFF);        // 下位バイト
        checksum ^= (value >> 8) & 0xFF;
        checksum ^= value & 0xFF;
      }
      uart1.write(checksum);
    }
  }
}