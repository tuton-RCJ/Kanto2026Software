#ifndef BNO085_H
#define BNO085_H

#include <Arduino.h>
#include <Adafruit_BNO08x.h>
#include <Wire.h>




// レポートタイプと間隔の定義
#define FAST_MODE

#ifdef FAST_MODE
  // Top frequency is reported to be 1000Hz (but freq is somewhat variable)
    #define REPORT_TYPE SH2_GYRO_INTEGRATED_RV
    #define REPORT_INTERVAL_US  2000
#else
    // Top frequency is about 250Hz but this report is more accurate
    #define REPORT_TYPE SH2_ARVR_STABILIZED_RV // AR/VR Stabilization Rotation Vector (より正確)
    #define REPORT_INTERVAL_US 5000 // 5000 us = 200 Hz

#endif


class BNO085
{
public:
    // コンストラクタ: I2C通信を想定
    BNO085( TwoWire *wire = &Wire,HardwareSerial *debugSerial= nullptr);

    bool begin();
    // キャリブレーション状態を確認 (0:unreliable to 3:fully calibrated)
    bool isCalibrated();
    void setZero();

    // センサー値を読み取り、成功すればtrueを返す
    bool read();
    
    // 読み取り値（オイラー角）
    float direction; // headingからオフセットを引いた値
    float heading, pitch, roll;

    void print();

private:
    Adafruit_BNO08x _bno;
    float direction_offset;
    uint8_t _calibration_status;
    HardwareSerial *_debugSerial;

    // クォータニオンからオイラー角への変換
    void quaternionToEuler(float qr, float qi, float qj, float qk, float* yaw, float* pitch, float* roll, bool degrees = true);
    void setReports(sh2_SensorId_t reportType, long report_interval);
};

#endif