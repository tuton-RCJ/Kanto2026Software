#include "UnitV.h"

UnitV::UnitV(HardwareSerial *serial, unsigned long baudrate)
    : _serial(serial), _baudrate(baudrate)
{
    _serial->begin(_baudrate);
    status = 0;
}

void UnitV::read()
{
    while (_serial->available())
    {
        status = _serial->read();
    }
}
