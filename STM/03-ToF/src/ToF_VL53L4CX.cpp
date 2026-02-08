#include "ToF_VL53L4CX.h"

ToF_VL53L4CX::ToF_VL53L4CX(TwoWire *i2c)
{
    for (int i = 0; i < tof_num; i++)
    {
        tof_values[i] = 0;
        tof_sensors[i].setI2cDevice(i2c);
        tof_sensors[i].setXShutPin(tof_pins[i]);
        pinMode(tof_pins[i], OUTPUT);
        digitalWrite(tof_pins[i], LOW);
    }
}

void ToF_VL53L4CX::init()
{
    for (int i = 0; i < tof_num; i++)
    {
        tof_sensors[i].begin();
        tof_sensors[i].VL53L4CX_Off();
        tof_sensors[i].InitSensor(0x12 + i * 2);
        // tof_sensors[i].VL53L4CX_SetDistanceMode(VL53L4CX_DISTANCEMODE_SHORT);
        tof_sensors[i].VL53L4CX_SetMeasurementTimingBudgetMicroSeconds(15000);

        delay(10);
    }
    for (int i = 0; i < tof_num; i++)
    {
        tof_sensors[i].VL53L4CX_StartMeasurement();
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

byte ToF_VL53L4CX::update(HardwareSerial *SerialPort)
{
    byte value_changed = 0x00;
    for (int i = 0; i < 4; i++)
    {
        VL53L4CX_MultiRangingData_t MultiRangingData;
        VL53L4CX_MultiRangingData_t *pMultiRangingData = &MultiRangingData;
        uint8_t NewDataReady = 0;
        int no_of_object_found = 0, j;
        int status;

        // do
        // {
        status = tof_sensors[i].VL53L4CX_GetMeasurementDataReady(&NewDataReady);
        // } while (!NewDataReady);

        if (!NewDataReady)
        {
            continue;
        }

        if ((!status) && (NewDataReady != 0))
        {
            status = tof_sensors[i].VL53L4CX_GetMultiRangingData(pMultiRangingData);

            no_of_object_found = pMultiRangingData->NumberOfObjectsFound;
            uint16_t min_distance = 0xFFFF;
            bool has_valid_distance = false;
            for (j = 0; j < no_of_object_found; j++)
            {
                uint16_t distance = pMultiRangingData->RangeData[j].RangeMilliMeter;
                if (distance > 80 && distance < min_distance)
                {
                    min_distance = distance;
                    has_valid_distance = true;
                }
            }
            if (no_of_object_found == 0 || status != 0 || !has_valid_distance)
            {
                tof_values[i] = 8191;
            }
            else
            {
                tof_values[i] = min_distance;
            }
            if (status == 0)
            {
                status = tof_sensors[i].VL53L4CX_ClearInterruptAndStartMeasurement();
            }
            value_changed |= (1 << i);
        }
    }
    // if (value_changed != 0)
    // {
    //     SerialPort->print("ToF: ");
    //     for (int i = 0; i < tof_num; i++)
    //     {
    //         SerialPort->print(tof_values[i]);
    //         if (i < tof_num - 1)
    //         {
    //             SerialPort->print(" ");
    //         }
    //     }
    //     SerialPort->println();
    // }
    return value_changed;
}