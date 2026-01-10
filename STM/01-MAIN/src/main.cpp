#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "device/device.h"
#define I2C_SDA PB7
#define I2C_SCL PB6
#include <queue>
#include <Wire.h>

HardwareSerial uart1(PA10, PA9);              // USB
HardwareSerial uart2(PA3, PA2);               // RPi
HardwareSerial uart3(PC_11_ALT1, PC_10_ALT1); // STS3032
HardwareSerial uart4(PA_1, PA_0);             // UnitV Left
HardwareSerial uart5(PD_2, PC_12);            // UnitV Right
HardwareSerial uart6(PC_7, PC_6);             // ToF module
UnitV unitv_L(&uart4);
UnitV unitv_R(&uart5);
void ReadUnitV();
int unitV_L_lastStatus;
int unitV_R_lastStatus;
unsigned long unitV_L_lastUpdatedTime;
unsigned long unitV_R_lastUpdatedTime;

#define isLoadcell false // loadcellを使う場合true,switchの場合false
LoadCell loadcell(PC0, PC1);
int switch_front[2] = {PB10, PB12};
int switch_rear[2] = {PB13, PB14};
void ReadBumper();

// BNO055 bno(55, &Wire);
BNO085 bno(&Wire, &uart1);
void ReadBNO();

const int SWpin[2] = {PA13, PA12};
void ReadSW();

STS3032 sts3032(&uart3);
void driveSTS3032(int leftSpeed, int rightSpeed);

void ReadToF();

Buzzer buzzer(PB8);
LED led(PA14, 1);

const int Servo_L = PC9;
const int Servo_R = PB9;
Servo servo_L;
Servo servo_R;
void DropRescueKit(bool isLeft);
void MoveServo();

void init_i2c();
void i2c_scan();
void i2c_bus_recovery();
// Display ssd1306(128, 64, -1); // width, height, resetPin

byte sensorData[21]; // UnitV_L,UnitV_R,UnitV_L_Time,UnitV_R_Time, Loadcell_L, Loadcell_R, BNO055_heading[2], BNO055_pitch[2],BNO055_roll[2],SW,ToF0[2],ToF1[2],ToF2[2],ToF3[2]
void checkRPi();
bool verifyCheckDigit(byte data[], int length, byte checkDigit);

// サーボを動かすタイミングのキュー

// サーボを動かすタイミングを管理するための構造体。L・R、Open/Close、時間を持つ
struct ServoCommand
{
    bool isLeft;               // 左右
    bool isOpen;               // 開閉
    unsigned long executeTime; // 実行時間
};
std::queue<ServoCommand> servoCommandQueue;

void setup()
{
    uart1.begin(115200);
    uart1.println("System Start");
    uart6.begin(115200);
    servo_L.attach(Servo_L);
    servo_R.attach(Servo_R);
    servo_L.write(150);
    servo_R.write(60);

    init_i2c();
    // delay(50);

    uart1.println(bno.begin());

    // delay(50);
    // ssd1306.begin();
    // // ssd1306.runDemo();

    led.setBrightness(20);
    led.turnOff();
    sts3032.isDisabled = false;
    sts3032.stop();
    for (int i = 0; i < 2; i++)
    {
        pinMode(switch_front[i], INPUT);
        pinMode(switch_rear[i], INPUT);
        pinMode(SWpin[i], INPUT);
    }
    uart2.begin(115200);
    buzzer.boot();

    unitV_L_lastStatus=-1;
    unitV_R_lastStatus=-1;
    unitV_L_lastUpdatedTime=millis();
    unitV_R_lastUpdatedTime=millis();
}
void checkRPi();

void setToFboardLED(byte r, byte g, byte b)
{
    uart6.write(1); // LED制御コマンド
    uart6.write(r);
    uart6.write(g);
    uart6.write(b);
    byte checksum = 1 ^ r ^ g ^ b;
    uart6.write(checksum);
    // タイムアウト0.1秒で応答待ち
    unsigned long startTime = millis();
    while (uart6.available() < 1)
    {
        if (millis() - startTime > 100)
        {
            uart1.println("ToF LED set timeout");
            return;
        }
    }
    byte response = uart6.read();
    if (response != 0x01)
    {
        uart1.println("ToF LED set error");
    }
    return;
}
unsigned long previousMillis = 0;
void loop()
{
    // buzzer.Shougatu();
    // return;
    // uart1.println("Main Loop Start");
    // servo_R.write(180);
    // delay(500);
    // servo_R.write(60);
    // delay(500);
    // return;
    checkRPi();
    MoveServo();
    ReadUnitV();
    checkRPi();
    MoveServo();
    ReadBumper();
    checkRPi();
    // return;
    MoveServo();
    ReadBNO();
    checkRPi();
    MoveServo();
    ReadSW();
    checkRPi();
    // return;
    MoveServo();
    ReadToF();
    buzzer.update();
}

void checkRPi()
{
    // uart1.println("Checking RPi Command...");
    if (uart2.available() > 2)
    {
        // uart1.println("Received Command from RPi");
        byte type = uart2.read();
        byte seq = uart2.read();
        if (type == 0x00)
        {
            while (uart2.available() < 1)
            {
                // wait for check digit
            }
            // センサーデータ送信
            byte CD = uart2.read();
            byte data[2] = {type, seq};
            if (verifyCheckDigit(data, 2, CD))
            {
                byte checkDigit = 0x00;
                uart2.write(0x00); // type
                uart2.write(seq);  // seq
                checkDigit ^= 0x00;
                checkDigit ^= seq;
                for (int i = 0; i < 21; i++)
                {
                    uart2.write(sensorData[i]);
                    checkDigit ^= sensorData[i];
                }
                uart2.write(checkDigit);
            }
            else
            {
                uart1.println("CheckDigit Error!");
            }
        }
        else if (type == 0x01)
        {
            while (uart2.available() < 3)
            {
                // wait for full command
            }
            // STS3032動作指令
            byte leftSpeed = uart2.read();
            byte rightSpeed = uart2.read();
            byte CD = uart2.read();
            byte data[4] = {type, seq, leftSpeed, rightSpeed};
            if (verifyCheckDigit(data, 4, CD))
            {
                driveSTS3032((int)leftSpeed - 100, (int)rightSpeed - 100);
                // 返答
                uart2.write(0x01);       // type
                uart2.write(seq);        // seq
                uart2.write(0x01 ^ seq); // CD
            }
            else
            {
                uart1.println("CheckDigit Error!");
            }
        }
        else if (type == 2)
        {
            while (uart2.available() < 3)
            {
                // wait for full command
            }
            // 救助キット投下指令
            byte side = uart2.read();
            byte num = uart2.read();
            byte CD = uart2.read();
            byte data[4] = {type, seq, side, num};
            if (verifyCheckDigit(data, 4, CD))
            {
                uart1.println("Drop Rescue Kit Command Received");
                uart1.print("Number: ");
                uart1.println((int)num);
                unsigned long currentTime = millis();
                for (int i = 0; i < (int)num; i++)
                {
                    ServoCommand cmd;
                    cmd.isLeft = (side == 0);
                    cmd.isOpen = true;
                    cmd.executeTime = currentTime + i * 800;
                    servoCommandQueue.push(cmd);
                    cmd.isOpen = false;
                    cmd.executeTime = currentTime + i * 800 + 350;
                    servoCommandQueue.push(cmd);
                }
                // 返答
                uart2.write(0x02);       // type
                uart2.write(seq);        // seq
                uart2.write(0x02 ^ seq); // CD
            }

            else
            {
                uart1.println("CheckDigit Error!");
            }
        }
        else if (type == 3)
        {
            while (uart2.available() < 4)
            {
                // wait for full command
            }
            // LED制御命令
            byte r = uart2.read();
            byte g = uart2.read();
            byte b = uart2.read();
            byte CD = uart2.read();
            byte data[5] = {type, seq, r, g, b};
            if (verifyCheckDigit(data, 5, CD))
            {
                setToFboardLED(r, g, b);
                // 返答
                uart2.write(0x03);       // type
                uart2.write(seq);        // seq
                uart2.write(0x03 ^ seq); // CD
            }
            else
            {

                uart1.println("CheckDigit Error!");
            }
        }
        else if (type == 4)
        {
            // Buzzer Control Command
            while (uart2.available() < 1)
            {
            }
            byte musicLength = uart2.read();
            constexpr int MAX_MUSIC_LEN = 300;
            NoteMillis notes[MAX_MUSIC_LEN];
            int effectiveLength = (int)musicLength;
            if (effectiveLength > MAX_MUSIC_LEN)
            {
                effectiveLength = MAX_MUSIC_LEN;
            }
            uart1.println("Buzzer Music Command Received");
            uart1.print("Length: ");
            uart1.println((int)musicLength);
            // while (uart2.available() < musicLength * 4 + 1)
            // {
            //     uart1.println((int)uart2.available());
            // }
            // for (int i = 0; i < musicLength * 4 + 1; i++)
            // {
            //     // wait for full data
            // }

            byte CD = 0x04 ^ seq ^ musicLength;
            for (int i = 0; i < musicLength; i++)
            {
                while (uart2.available() < 4)
                {
                    // wait for full note data
                }
                byte note_h = uart2.read();
                byte note_l = uart2.read();
                byte dur_h = uart2.read();
                byte dur_l = uart2.read();
                if (i < effectiveLength)
                {
                    notes[i].note = ((int)note_h << 8) | (int)note_l;
                    notes[i].duration = ((int)dur_h << 8) | (int)dur_l;
                }
                CD ^= note_h ^ note_l ^ dur_h ^ dur_l;
            }
            while (uart2.available() < 1)
            {
                // wait for check digit
            }
            byte receivedCD = uart2.read();
            if (CD == receivedCD)
            {
                uart1.println("CheckDigit OK!");
                if (effectiveLength <= 0)
                {
                    buzzer.mute();
                }
                else
                {
                    buzzer.RegisterMusic(notes, effectiveLength);
                }

                // 返答
                uart2.write(0x04);       // type
                uart2.write(seq);        // seq
                uart2.write(0x04 ^ seq); // CD
            }
            else
            {
                uart1.println("CheckDigit Error!");
            }
        }
        else
        {
            uart1.println("Unknown Command Type!");
            uart1.println(type);
        }
        while (uart2.available())
        {
            uart2.read();
        }
    }
}

void init_i2c()
{
    Wire.setSDA(I2C_SDA);
    Wire.setSCL(I2C_SCL);
    Wire.begin();
    Wire.setClock(100000); // 100kHz
}
void i2c_bus_recovery()
{
    pinMode(I2C_SDA, INPUT_PULLUP);
    pinMode(I2C_SCL, OUTPUT_OPEN_DRAIN);

    // 9クロック生成
    for (int i = 0; i < 9; i++)
    {
        digitalWrite(I2C_SCL, HIGH);
        delayMicroseconds(5);
        digitalWrite(I2C_SCL, LOW);
        delayMicroseconds(5);
    }

    // STOPコンディションを生成
    pinMode(I2C_SDA, OUTPUT_OPEN_DRAIN);
    digitalWrite(I2C_SDA, LOW);
    delayMicroseconds(5);

    digitalWrite(I2C_SCL, HIGH);
    delayMicroseconds(5);

    digitalWrite(I2C_SDA, HIGH);
    delayMicroseconds(5);

    // I2Cを再初期化
    Wire.begin();
}

void i2c_scan()
{
    uart1.println("I2C Scan Start");
    for (byte address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
        if (error == 0)
        {
            uart1.print("I2C device found at address 0x");
            if (address < 16)
                uart1.print("0");
            uart1.print(address, HEX);
            uart1.println(" !");
        }
    }
    uart1.println("I2C Scan End");
}

void MoveServo()
{
    while (servoCommandQueue.size())
    {

        unsigned long currentTime = millis();
        ServoCommand cmd = servoCommandQueue.front();
        if (currentTime >= cmd.executeTime)
        {
            if (cmd.isLeft)
            {
                if (cmd.isOpen)
                {
                    servo_L.write(30);
                }
                else
                {
                    servo_L.write(150);
                }
            }
            else
            {
                if (cmd.isOpen)
                {
                    servo_R.write(180);
                }
                else
                {
                    servo_R.write(60);
                }
            }
            servoCommandQueue.pop();
        }
        else
        {
            break;
        }
    }
}

void DropRescueKit(bool isLeft)
{
    if (isLeft)
    {
        servo_L.write(66);
        delay(200);
        servo_L.write(0);
        delay(200);
    }
    else
    {
        servo_R.write(90);
        delay(200);
        servo_R.write(180);
        delay(200);
    }
}

void driveSTS3032(int leftSpeed, int rightSpeed)
{
    for (int i = 0; i < 5; i++)
    {
        sts3032.LeftDrive(leftSpeed, 0);
        sts3032.RightDrive(rightSpeed, 0);
    }
}

void ReadUnitV()
{

    unitv_L.read();
    sensorData[0] = (byte)(unitv_L.status & 0xFF);
    unitv_R.read();
    sensorData[1] = (byte)(unitv_R.status & 0xFF);
    if(unitV_L_lastStatus!=unitv_L.status){
        unitV_L_lastUpdatedTime=millis();
        unitV_L_lastStatus=unitv_L.status;
    }
    if(unitV_R_lastStatus!=unitv_R.status){
        unitV_R_lastUpdatedTime=millis();
        unitV_R_lastStatus=unitv_R.status;
    }
    unsigned long currentTime = millis();
    sensorData[2] = (byte)(((currentTime - unitV_L_lastUpdatedTime)>0xFF ? 0xFF : (currentTime - unitV_L_lastUpdatedTime)) & 0xFF);
    sensorData[3] = (byte)(((currentTime - unitV_R_lastUpdatedTime)>0xFF ? 0xFF : (currentTime - unitV_R_lastUpdatedTime)) & 0xFF);

    // if (unitv_L.status != 0)
    // {
    //     led.setColor(255, 0, 0);
    // }
    // else if (unitv_R.status != 0)
    // {
    //     led.setColor(0, 0, 255);
    // }
    // else
    // {
    //     led.setColor(0, 0, 0);
    // }
}

void ReadBumper()
{
    if (isLoadcell)
    {
        loadcell.read();
        sensorData[4] = (byte)((loadcell.values[0] / 64) & 0xFF);
        sensorData[5] = (byte)((loadcell.values[1] / 64) & 0xFF);
    }
    else
    {
        sensorData[4] = (byte)(digitalRead(switch_front[0]) << 7 | digitalRead(switch_front[1]) << 6);
        sensorData[5] = (byte)(digitalRead(switch_rear[0]) << 7 | digitalRead(switch_rear[1]) << 6);
    }
    // uart1.print("Bumper Front: ");
    // uart1.println(sensorData[2]);
}

void ReadBNO()
{
    bno.read();
    // int16_tに変換して、2バイトずつ格納
    int16_t heading_int = (int16_t)(bno.heading * 100);
    sensorData[6] = (byte)((heading_int >> 8) & 0xFF);
    sensorData[7] = (byte)(heading_int & 0xFF);
    int16_t pitch_int = (int16_t)(bno.pitch * 100);
    sensorData[8] = (byte)((pitch_int >> 8) & 0xFF);
    sensorData[9] = (byte)(pitch_int & 0xFF);
    int16_t roll_int = (int16_t)(bno.roll * 100);
    sensorData[10] = (byte)((roll_int >> 8) & 0xFF);
    sensorData[11] = (byte)(roll_int & 0xFF);
}

void ReadSW()
{
    // 各スイッチの情報を1ビットずつ
    sensorData[12] = (digitalRead(SWpin[0]) << 7) + (digitalRead(SWpin[1]) << 6);
}

bool verifyCheckDigit(byte data[], int length, byte checkDigit)
{
    // XORが一致するか確認
    byte cd = 0x00;
    for (int i = 0; i < length; i++)
    {
        cd ^= data[i];
    }
    return (cd == checkDigit);
}

void ReadToF()
{
    uart6.write(0);
    while (uart6.available() < 9)
    {
        // wait for full data
    }
    for (int i = 0; i < 8; i++)
    {
        sensorData[13 + i] = uart6.read();
    }
    byte checksum = uart6.read();
    byte cd = 0x00;
    for (int i = 0; i < 8; i++)
    {
        cd ^= sensorData[13 + i];
    }
    if (cd != checksum)
    {
        // エラー。値を0にする
        for (int i = 0; i < 8; i++)
        {
            sensorData[13 + i] = 0;
        }
    }
    while (uart6.available())
    {
        uart6.read();
    }

    // ToFの値をintにしてuart1に出力
    // uart1.print("ToF: ");
    // for (int i = 0; i < 4; i++)
    // {
    //   int distance = ((int)sensorData[11 + i * 2] << 8) + (int)sensorData[12 + i * 2];
    //   uart1.print(distance);
    //   uart1.print(" ");
    // }
    // uart1.println();
}
