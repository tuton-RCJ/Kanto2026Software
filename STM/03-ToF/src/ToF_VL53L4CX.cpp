#include "ToF_VL53L4CX.h"   

ToF_VL53L4CX::ToF_VL53L4CX(TwoWire *i2c)
{

    for (int i = 0; i < tof_num; i++)
    {
        tof_values[i] = 0;
        tof_sensors[i].setI2cDevice(i2c);
        tof_sensors[i].setXShutPin(tof_pins[i]);
        pinMode(tof_pins[i], OUTPUT);
    }


}
void ToF_VL53L4CX::init()
{
    for (int i = 0; i < tof_num; i++)
    {
        tof_sensors[i].begin();
        tof_sensors[i].VL53L4CX_Off();
    }
    // XshutLow();
    delay(50);
    for (int i = 0; i < tof_num; i++)
    {
        tof_sensors[i].InitSensor(0x30 + i);
    }
}

void ToF_VL53L4CX::print(HardwareSerial *serial)
{
    serial->print("ToF: ");
    for (int i = 0; i < tof_num; i++)
    {
        serial->print(tof_values[i]);
        serial->print(" ");
    }
    serial->println();
}

// void ToF_VL53L4CX::XshutLow()
// {
//     for (int i = 0; i < tof_num; i++)
//     {
//         digitalWrite(tof_pins[i], LOW);
//     }
// }

void ToF_VL53L4CX::update(HardwareSerial *SerialPort)
{
    for (int i = 0; i < tof_num; i++)
    {
        VL53L4CX_MultiRangingData_t MultiRangingData;
        VL53L4CX_MultiRangingData_t *pMultiRangingData = &MultiRangingData;
        uint8_t NewDataReady = 0;
        int no_of_object_found = 0, j;
        char report[64];
        int status;

        status = tof_sensors[i].VL53L4CX_GetMeasurementDataReady(&NewDataReady);

        if (!NewDataReady)
        {
            continue;
        }

        if ((!status) && (NewDataReady != 0))
        {
            status = tof_sensors[i].VL53L4CX_GetMultiRangingData(pMultiRangingData);
            no_of_object_found = pMultiRangingData->NumberOfObjectsFound;
            snprintf(report, sizeof(report), "VL53L4CX Satellite: Count=%d, #Objs=%1d ", pMultiRangingData->StreamCount, no_of_object_found);
            SerialPort->print("Sensor Number: ");
            SerialPort->print(i);
            SerialPort->print(" - ");
            SerialPort->print(report);
            for (j = 0; j < no_of_object_found; j++)
            {
                if (j != 0)
                {
                    SerialPort->print("\r\n                               ");
                }
                SerialPort->print("status=");
                SerialPort->print(pMultiRangingData->RangeData[j].RangeStatus);
                SerialPort->print(", D=");
                SerialPort->print(pMultiRangingData->RangeData[j].RangeMilliMeter);
                SerialPort->print("mm");
                SerialPort->print(", Signal=");
                SerialPort->print((float)pMultiRangingData->RangeData[j].SignalRateRtnMegaCps / 65536.0);
                SerialPort->print(" Mcps, Ambient=");
                SerialPort->print((float)pMultiRangingData->RangeData[j].AmbientRateRtnMegaCps / 65536.0);
                SerialPort->print(" Mcps");
            }
            SerialPort->println("");
            if (status == 0)
            {
                status = tof_sensors[i].VL53L4CX_ClearInterruptAndStartMeasurement();
            }
        }
    }
}