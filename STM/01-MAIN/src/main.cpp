#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "device/device.h"
#define I2C_SDA PB7
#define I2C_SCL PB6

HardwareSerial uart1(PA10, PA9);              // USB
HardwareSerial uart2(PA3, PA2);               // RPi
HardwareSerial uart3(PC_11_ALT1, PC_10_ALT1); // STS3032
HardwareSerial uart4(PA1, PA0);               // UnitV Left
HardwareSerial uart5(PD_2, PC_12);            // UnitV Right

UnitV unitv_L(&uart4);
UnitV unitv_R(&uart5);
void ReadUnitV();

LoadCell loadcell(PC0, PC2);
void ReadLoadcell();

BNO055 bno(55, &Wire);
void ReadBNO();

const int SWpin[2] = {PA13, PA12};
void ReadSW();

STS3032 sts3032(&uart3);
void driveSTS3032(int leftSpeed, int rightSpeed);

Buzzer buzzer(PB8);
LED led(PA14, 1);

const int Servo_L = PC9;
const int Servo_R = PB9;
Servo servo_L;
Servo servo_R;
void DropRescueKit(bool isLeft);

void init_i2c();

Display ssd1306(128, 64, -1); // width, height, resetPin

byte sensorData[8]; // UnitV_L,UnitV_R, Loadcell_L, Loadcell_R, BNO055_heading, BNO055_pitch,BNO055_roll,SW
void checkRPi();
bool verifyCheckDigit(byte[] data, int length, byte checkDigit);

void setup()
{
  uart1.begin(115200);

  servo_L.attach(Servo_L);
  servo_R.attach(Servo_R);
  servo_L.write(0);
  servo_R.write(180);

  init_i2c();
  delay(50);

  uart1.println(bno.begin());

  delay(50);
  ssd1306.begin();
  // // ssd1306.runDemo();

  led.setBrightness(20);
  led.turnOff();
  sts3032.isDisabled = false;
  sts3032.stop();

  uart2.begin(115200);
}
void checkRPi();
void loop()
{

  checkRPi();
  ReadUnitV();
  checkRPi();
  ReadLoadcell();
  checkRPi();
  ReadBNO();
  checkRPi();
  ReadSW();
}

void checkRPi()
{
  if (uart2.available())
  {

    byte type = uart2.read();
    byte seq = uart2.read();
    if (type = 0x00)
    {
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
        for (int i = 0; i < 8; i++)
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
    else if (type == 1)
    {
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
      // 救助キット投下指令
      byte side = uart2.read();
      byte num = uart2.read();
      byte CD = uart2.read();
      byte data[4] = {type, seq, side, num};
      if (verifyCheckDigit(data, 4, CD))
      {
        for (int i = 0; i < num; i++)
        {
          DropRescueKit(side == 0);
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
      // LED制御命令
      byte r = uart2.read();
      byte g = uart2.read();
      byte b = uart2.read();
      byte CD = uart2.read();
      byte data[5] = {type, seq, r, g, b};
      if (verifyCheckDigit(data, 5, CD))
      {
        led.setColor(r, g, b);
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
}

void DropRescueKit(bool isLeft)
{
  if (isLeft)
  {
    servo_L.write(70);
    delay(400);
    servo_L.write(0);
    delay(400);
  }
  else
  {
    servo_R.write(110);
    delay(400);
    servo_R.write(180);
    delay(400);
  }
}

void driveSTS3032(int leftSpeed, int rightSpeed)
{
  for (int i = 0; i < 5; i++)
  {
    sts3032.LeftDrive(leftSpeed);
    sts3032.RightDrive(rightSpeed);
  }
}

void ReadUnitV()
{
  unitv_L.read();
  sensorData[0] = (byte)(unitv_L.status & 0xFF);
  unitv_R.read();
  sensorData[1] = (byte)(unitv_R.status & 0xFF);
}

void ReadLoadcell()
{
  loadcell.read();
  sensorData[2] = (byte)(loadcell.values[0] & 0xFF);
  sensorData[3] = (byte)(loadcell.values[1] & 0xFF);
}

void ReadBNO()
{
  bno.read();
  sensorData[4] = (byte)(((int)bno.heading / 2) & 0xFF);
  sensorData[5] = (byte)(((int)bno.pitch / 2) & 0xFF);
  sensorData[6] = (byte)(((int)bno.roll / 2) & 0xFF);
}

void ReadSW()
{
  // 各スイッチの情報を1ビットずつ
  sensorData[7] = (digitalRead(SWpin[0]) << 7) + (digitalRead(SWpin[1]) << 6);
}

bool verifyCheckDigit(byte[] data, int length, byte checkDigit)
{
  // XORが一致するか確認
  byte cd = 0x00;
  for (int i = 0; i < length; i++)
  {
    cd ^= data[i];
  }
  return (cd == checkDigit);
}