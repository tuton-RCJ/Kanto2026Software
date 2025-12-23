#ifndef UnitV_H
#define UnitV_H

#include <Arduino.h>
#include <HardwareSerial.h>

class UnitV
{
public:
    UnitV(HardwareSerial *serial, unsigned long baudrate = 115200);

    void read();
    int status;

private:
    HardwareSerial *_serial;
    unsigned long _baudrate;
};
#endif
