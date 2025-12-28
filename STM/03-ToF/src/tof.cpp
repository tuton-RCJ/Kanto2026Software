#include "tof.h"

ToF::ToF()
{
    for (int i = 0; i < tof_num; i++)
    {
        tof_values[i] = 0;
    }
    for (int i = 0; i < tof_num; i++)
    {
        pinMode(tof_pins[i], OUTPUT);
    }
    XshutLow();
}
void ToF::init()
{
    XshutLow();
    delay(50);
    for (int i = 0; i < tof_num; i++)
    {
        init_tof_sensors(i);
    }
}

void ToF::getTofValues()
{
    for (int i = 0; i < tof_num; i++)
    {
        tof_values[i] = tof_sensors[i].readRangeContinuousMillimeters();
    }
}

int ToF::init_tof_sensors(int i)
{
    digitalWrite(tof_pins[i], HIGH);
    tof_sensors[i] = VL53L0X();
    delay(10);
    // i2c_scanner();
    tof_sensors[i].setTimeout(500);
    if (!tof_sensors[i].init())
    {
        // Serial.println("Failed to detect and initialize sensor!");
        // i2c_scanner();
        // Serial.println(i);
    }
    else
    {
        tof_sensors[i].setAddress(0x30 + i);
    }
    tof_sensors[i].setMeasurementTimingBudget(50000);
    // tof_sensors[i].setSignalRateLimit(0.1);
    // increase laser pulse periods (defaults are 14 and 10 PCLKs)
    // tof_sensors[i].setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18);
    // tof_sensors[i].setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14);
    tof_sensors[i].startContinuous();

    // delay(100);
    // digitalWrite(tof_pins[i], LOW);

    return 0;
}

void ToF::print(HardwareSerial *serial)
{
    serial->print("ToF: ");
    for (int i = 0; i < tof_num; i++)
    {
        serial->print(tof_values[i]);
        serial->print(" ");
    }
    serial->println();
}

// bool ToF::i2c_scanner()
// {
//     // scan for i2c devices
//     byte error, address;
//     int nDevices;

//     Serial.println("Scanning...");

//     nDevices = 0;
//     for (address = 1; address < 127; address++)
//     {
//         // The i2c_scanner uses the return value of
//         // the Write.endTransmisstion to see if
//         // a device did acknowledge to the address.
//         Wire.beginTransmission(address);
//         error = Wire.endTransmission();

//         if (error == 0)
//         {
//             Serial.print("I2C device found at address 0x");
//             if (address < 16)
//                 Serial.print("0");
//             Serial.print(address, HEX);
//             Serial.println("  !");

//             nDevices++;
//         }
//         else if (error == 4)
//         {
//             Serial.print("Unknown error at address 0x");
//             if (address < 16)
//                 Serial.print("0");
//             Serial.println(address, HEX);
//         }
//     }
//     if (nDevices == 0)
//     {
//         Serial.println("No I2C devices found\n");
//         return false;
//     }
//     else
//     {
//         Serial.println("done\n");
//         return true;
//     }
// }

void ToF::XshutLow()
{
    for (int i = 0; i < tof_num; i++)
    {
        digitalWrite(tof_pins[i], LOW);
    }
}

void ToF::update()
{
    for (int i = 0; i < tof_num; i++)
    {
        // センサーのアドレス (initで設定した 0x30 + i)
        int address = 0x30 + i;

        // VL53L0Xのレジスタ 0x13 (RESULT_INTERRUPT_STATUS) をチェック
        Wire.beginTransmission(address);
        Wire.write(0x13);
        Wire.endTransmission();
        Wire.requestFrom(address, 1);

        if (Wire.available())
        {
            byte status = Wire.read();
            // ステータスの下位3ビットが非ゼロなら新しい測定データあり
            if ((status & 0x07) != 0)
            {
                // データがあるので読み出す。
                // ここで readRangeContinuousMillimeters を呼ぶと、
                // すでにフラグが立っているので待機せずに即座に値を返してくれます。
                tof_values[i] = tof_sensors[i].readRangeContinuousMillimeters();
                
                // 異常値(タイムアウト等)の場合は無視するか、前回の値を保持するならここを調整
                if (tof_sensors[i].timeoutOccurred()) { 
                    // エラー処理が必要なら記述
                }
            }
        }
    }
}