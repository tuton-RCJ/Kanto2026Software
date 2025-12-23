#ifndef tof_H
#define tof_H

#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>
# define tof_num 4
class ToF
{
public:
    ToF();
    void init();
    
    volatile uint16_t tof_values[tof_num];
    void getTofValues();
    void print(HardwareSerial *serial);
    // bool i2c_scanner();
    void XshutLow();
    void update(); // アンブロッキングな読み取り
    

private:
    int tof_pins[tof_num] = {PC0,PC2,PC4,PC6};
    VL53L0X tof_sensors[tof_num];
    int init_tof_sensors(int i);
};

#endif