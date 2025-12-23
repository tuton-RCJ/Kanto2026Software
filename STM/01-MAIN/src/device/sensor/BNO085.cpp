#include "BNO085.h"

#ifndef PI
#define PI 3.14159265358979323846
#endif

#ifndef RAD_TO_DEG
#define RAD_TO_DEG (180.0 / PI)
#endif

#ifndef DEG_TO_RAD
#define DEG_TO_RAD (PI / 180.0)
#endif

// コンストラクタの実装
BNO085::BNO085( TwoWire *wire, HardwareSerial *debugSerial)
{
    _bno = Adafruit_BNO08x(-1);
    _debugSerial = debugSerial;
}

// BNO085の初期化
bool BNO085::begin()
{
    direction_offset = 0.0;
    _calibration_status = 0;

    // I2C初期化 (begin_I2Cの呼び出しは、コンストラクタで設定したパラメータを使用)
    if (!_bno.begin_I2C())
    {
        return false;
    }

    // レポートの有効化
    setReports(REPORT_TYPE, REPORT_INTERVAL_US);
    _debugSerial->println("BNO085 initialized");
    return true;
}

// キャリブレーション状態の確認
// BNO085のキャリブレーションは 0 (unreliable) から 3 (fully calibrated)
bool BNO085::isCalibrated()
{
    // キャリブレーションステータスが3（完全キャリブレーション）であればtrue
    return (_calibration_status == 3);
}

// センサー値を読み取り、オイラー角を更新
bool BNO085::read()
{
    sh2_SensorValue_t sensorValue;

    if (_bno.wasReset())
    {
        _debugSerial->print("sensor was reset ");
        setReports(REPORT_TYPE, REPORT_INTERVAL_US);
    }

    // センサーイベントの取得
    if (!_bno.getSensorEvent(&sensorValue))
    {
        return false;
    }

    // ARVR Stabilized Rotation Vector レポートであるかを確認
    if (sensorValue.sensorId != REPORT_TYPE)
    {
        return false;
    }

    // キャリブレーションステータスを更新
    _calibration_status = sensorValue.status;

    // クォータニオンデータへのポインタを取得
    sh2_RotationVectorWAcc_t *rotational_vector = &sensorValue.un.arvrStabilizedRV;

    // クォータニオンからオイラー角に変換
    quaternionToEuler(rotational_vector->real,
                      rotational_vector->i,
                      rotational_vector->j,
                      rotational_vector->k,
                      &heading, &pitch, &roll, true); // trueで度(degrees)に変換

    // directionの計算 (0-360度の範囲に収める)
    direction = heading - direction_offset;
    if (direction < 0.0)
    {
        direction += 360.0;
    }
    else if (direction >= 360.0)
    {
        direction -= 360.0;
    }

    return true;
}

// directionを現在のheadingにリセット
void BNO085::setZero()
{
    read(); // 最新のheadingを取得
    direction_offset = heading;
}

// センサー値のシリアル出力
void BNO085::print()
{
    _debugSerial->print(F("Calib: "));
    _debugSerial->print(_calibration_status);
    _debugSerial->print(F(" Heading: "));
    _debugSerial->print(heading, 2);
    _debugSerial->print(F(" Pitch: "));
    _debugSerial->print(pitch, 2);
    _debugSerial->print(F(" Roll: "));
    _debugSerial->print(roll, 2);
    _debugSerial->print(F(" Direction: "));
    _debugSerial->println(direction, 2);
}

// --- プライベート関数 ---

/**
 * @brief クォータニオンからオイラー角 (Yaw, Pitch, Roll) への変換
 * @param qr クォータニオンの実部 (w)
 * @param qi クォータニオンの i 成分 (x)
 * @param qj クォータニオンの j 成分 (y)
 * @param qk クォータニオンの k 成分 (z)
 * @param yaw 計算されたヨー角 (Heading) の格納先ポインタ
 * @param pitch 計算されたピッチ角 の格納先ポインタ
 * @param roll 計算されたロール角 の格納先ポインタ
 * @param degrees 結果を度数 (true) またはラジアン (false) で返すか
 */
void BNO085::quaternionToEuler(float qr, float qi, float qj, float qk, float *yaw, float *pitch, float *roll, bool degrees)
{

    float sqr = sq(qr);
    float sqi = sq(qi);
    float sqj = sq(qj);
    float sqk = sq(qk);
    float t;

    // Yaw (Z軸回転)
    // atan2(2.0 * (qi * qj + qk * qr), (sqi - sqj - sqk + sqr));
    t = 2.0 * (qi * qj + qk * qr);
    t /= (sqi - sqj - sqk + sqr);
    *yaw = atan2(2.0 * (qi * qj + qk * qr), (sqi - sqj - sqk + sqr));

    // Pitch (Y軸回転)
    // asin(-2.0 * (qi * qk - qj * qr) / (sqi + sqj + sqk + sqr));
    *pitch = asin(-2.0 * (qi * qk - qj * qr) / (sqi + sqj + sqk + sqr));

    // Roll (X軸回転)
    // atan2(2.0 * (qj * qk + qi * qr), (-sqi - sqj + sqk + sqr));
    *roll = atan2(2.0 * (qj * qk + qi * qr), (-sqi - sqj + sqk + sqr));

    if (degrees)
    {
        *yaw *= RAD_TO_DEG;
        *pitch *= RAD_TO_DEG;
        *roll *= RAD_TO_DEG;

        // Yaw (Heading) を 0 から 360 度の範囲に調整
        if (*yaw < 0.0)
        {
            *yaw += 360.0;
        }
    }
}

void BNO085::setReports(sh2_SensorId_t reportType, long report_interval) {
  Serial.println("Setting desired reports");
  if (! _bno.enableReport(reportType, report_interval)) {
    Serial.println("Could not enable stabilized remote vector");
  }
}