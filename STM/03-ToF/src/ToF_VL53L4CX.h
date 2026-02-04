#ifndef tof_VL53L4CX_H
#define tof_VL53L4CX_H

#include <Arduino.h>
#include <Wire.h>
#include <vl53l4cx_class.h>

#define tof_num 4

class ToF_VL53L4CX
{
public:
    ToF_VL53L4CX(TwoWire *i2c = &Wire);
    void init();

    volatile uint16_t tof_values[tof_num];

    void print(HardwareSerial *serial);
    // bool i2c_scanner();
    // void XshutLow();
    byte update(HardwareSerial *SerialPort); // アンブロッキングな読み取り

private:
    int tof_pins[4] = {PC0, PC2, PC4, PC6};
    VL53L4CX tof_sensors[tof_num];
};

#endif