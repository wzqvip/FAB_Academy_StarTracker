#include <Arduino.h>
#include <MPU6050.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include <U8x8lib.h>
#include <Wire.h>

#define I2C_ADDRESS 0x3C
#define U8LOG_WIDTH 16
#define U8LOG_HEIGHT 4

// Define Pin Usage
const int servoXPin = 9;
const int servoYPin = 10;
const int pinSW = A0;
const int pinX = A1;
const int pinY = A2;
const int softRX = 2;
const int softTX = 3;

// Initalize variables
int Xin, Yin, X, Y, SW;
bool Status = false;

int mainCount = 0;
Servo servoX;
Servo servoY;
// Adafruit_SSD1306 oled(128, 32, &Wire, OLED_RESET);
SoftwareSerial ZigbeeSerial(softRX, softTX);
MPU6050 mpu;
U8X8_SSD1306_128X32_UNIVISION_SW_I2C u8x8(/* clock=*/SCL, /* data=*/SDA,
                                          /* reset=*/U8X8_PIN_NONE);

uint8_t u8log_buffer[U8LOG_WIDTH * U8LOG_HEIGHT];
U8X8LOG u8x8log;

int16_t ax, ay, az;
int16_t gx, gy, gz;
unsigned long now, lastTime = 0;
float dt;                                                    //微分时间
float aax = 0, aay = 0, aaz = 0, agx = 0, agy = 0, agz = 0;  //角度变量
long axo = 0, ayo = 0, azo = 0;  //加速度计偏移量
long gxo = 0, gyo = 0, gzo = 0;  //陀螺仪偏移量

float pi = 3.1415926;
float AcceRatio = 16384.0;  //加速度计比例系数
float GyroRatio = 131.0;    //陀螺仪比例系数

uint8_t n_sample = 8;  //加速度计滤波算法采样个数
float aaxs[8] = {0}, aays[8] = {0}, aazs[8] = {0};  // x,y轴采样队列
long aax_sum, aay_sum, aaz_sum;                     // x,y轴采样和

float a_x[10] = {0}, a_y[10] = {0}, a_z[10] = {0}, g_x[10] = {0}, g_y[10] = {0},
      g_z[10] = {0};               //加速度计协方差计算队列
float Px = 1, Rx, Kx, Sx, Vx, Qx;  // x轴卡尔曼变量
float Py = 1, Ry, Ky, Sy, Vy, Qy;  // y轴卡尔曼变量
float Pz = 1, Rz, Kz, Sz, Vz, Qz;  // z轴卡尔曼变量

const int ServoMin = 470;
const int ServoMax = 2500;

/* There are three mode currenty .
1. Manual Mode
    Just tune by hand
2. Serial Mode
    Will only keep the wireless serial.
    Input the position and then track
3. Pointer Mode
    Point by hand and it will follow your lead*/
const int ModeSelect = 0;
const int WirelessSerial = 0;
String Mode;

bool servo_driver(int Xin, int Yin) {  // Function to drive the servos
  if (Xin <= 180 and Xin >= 0) {
    int xTemp = 180 - Xin;
    if (abs(xTemp - servoX.read()) > 5) {
      X = xTemp;
    }
    int yTemp = 180 - Yin;
    if (abs(yTemp - servoY.read()) > 5) {
      Y = yTemp;
    }
  } else if (Xin <= 360) {
    int xTemp = 360 - Xin;
    if (abs(xTemp - servoX.read()) > 5) {
      X = xTemp;
    }
    int yTemp = Yin;
    if (abs(yTemp - servoY.read()) > 5) {
      Y = yTemp;
    }
  } else {
    X = 0;
    Y = 0;
    return false;
  }
  servoX.write(X);
  servoY.write(Y);
  return true;
}

void WelcomeScreen() {
  u8x8.print("\f");
  u8x8log.setRedrawMode(1);
  u8x8.drawString(0, 0, "INITIALIZING...");
  delay(100);
  u8x8.draw2x2String(0, 1, "STARTRCK");
  delay(200);
  u8x8.clear();
  u8x8.drawString(0, 1, "SERVO READY!");
  u8x8.drawString(0, 2, "MPU6050 READY!");
  u8x8.drawString(0, 3, "SYSTEM READY!");
  delay(1000);
  u8x8.print("\f");

  u8x8.clearDisplay();
  u8x8log.setRedrawMode(0);
}

void oledDisplay111(String str1, String str2, String str3, String str4) {
  u8x8.drawString(0, 0, str1.c_str());
  u8x8.drawString(0, 1, str2.c_str());
  u8x8.drawString(0, 2, str3.c_str());
  u8x8.drawString(0, 3, str4.c_str());
}

void oledDisplay121(String str1, String str2, String str3) {
  u8x8.drawString(0, 0, str1.c_str());
  u8x8.draw2x2String(0, 1, str2.c_str());
  u8x8.drawString(0, 3, str3.c_str());
}

void Mode1() {
  oledDisplay111("MANUAL MODE", "X POS: " + String(Xin),
                 "Y POS: " + String(Yin), "WORKING");
  Xin = map(analogRead(pinX), 0, 1023, 0, 360);
  Yin = map(analogRead(pinY), 0, 1023, 0, 360);
  servo_driver(Xin, Yin);
}

void Mode2() {
  if (WirelessSerial) {
    ZigbeeSerial.println("Enable Wireless Mode");
    String a;
    int count = 0;
    if (ZigbeeSerial.available()) {
      char c = ZigbeeSerial.read();
      while (c != '!' && count < 100) {
        a += c;
        count++;
        Serial.print(c);
        c = ZigbeeSerial.read();
      }
    }
    if (a.length() > 0) {
      String value1, value2;
      for (int i = 0; i < a.length(); i++) {
        if (a.substring(i, i + 1) == ",") {
          value2 = a.substring(0, i);
          value1 = a.substring(i + 1);
          break;
        }
      }
      Xin = value1.toInt();
      Yin = value2.toInt();
    }
  } else {
    if (Serial.available() > 0) {
      String a = Serial.readString();
      String value1, value2;
      for (int i = 0; i < a.length(); i++) {
        if (a.substring(i, i + 1) == ",") {
          value2 = a.substring(0, i);
          value1 = a.substring(i + 1);
          break;
        }
      }
      Xin = value1.toInt();
      Yin = value2.toInt();
    }
  }
  String Stat;
  if (WirelessSerial) {
    Stat = "WIRELESS MODE";
  } else {
    Stat = "SERIAL MODE";
  }
  oledDisplay111("SERIAL MODE", "RECEIVE X: " + String(Xin),
                 "RECEIVE Y: " + String(Yin), Stat);
}

void Mode3() { oledDisplay121("POINTER MODE", "POINTING", "WORKING"); }

void mpuData() {
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  Serial.print("a/g:\t");
  Serial.print(ax);
  Serial.print("\t");
  Serial.print(ay);
  Serial.print("\t");
  Serial.print(az);
  Serial.print("\t");
  Serial.print(gx);
  Serial.print("\t");
  Serial.print(gy);
  Serial.print("\t");
  Serial.println(gz);
}

void initCalcMotion() {
  unsigned short times = 200;  //采样次数
  for (int i = 0; i < times; i++) {
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);  //读取六轴原始数值
    axo += ax;
    ayo += ay;
    azo += az;  //采样和
    gxo += gx;
    gyo += gy;
    gzo += gz;
  }

  axo /= times;
  ayo /= times;
  azo /= times;  //计算加速度计偏移
  gxo /= times;
  gyo /= times;
  gzo /= times;  //计算陀螺仪偏移
}

void calcMotion() {
  unsigned long now = millis();    //当前时间(ms)
  dt = (now - lastTime) / 1000.0;  //微分时间(s)
  lastTime = now;                  //上一次采样时间(ms)

  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);  //读取六轴原始数值

  float accx = ax / AcceRatio;  // x轴加速度
  float accy = ay / AcceRatio;  // y轴加速度
  float accz = az / AcceRatio;  // z轴加速度

  aax = atan(accy / accz) * (-180) / pi;  // y轴对于z轴的夹角
  aay = atan(accx / accz) * 180 / pi;     // x轴对于z轴的夹角
  aaz = atan(accz / accy) * 180 / pi;     // z轴对于y轴的夹角

  aax_sum = 0;  // 对于加速度计原始数据的滑动加权滤波算法
  aay_sum = 0;
  aaz_sum = 0;

  for (int i = 1; i < n_sample; i++) {
    aaxs[i - 1] = aaxs[i];
    aax_sum += aaxs[i] * i;
    aays[i - 1] = aays[i];
    aay_sum += aays[i] * i;
    aazs[i - 1] = aazs[i];
    aaz_sum += aazs[i] * i;
  }

  aaxs[n_sample - 1] = aax;
  aax_sum += aax * n_sample;
  aax = (aax_sum / (11 * n_sample / 2.0)) * 9 / 7.0;  //角度调幅至0-90°
  aays[n_sample - 1] = aay;   //此处应用实验法取得合适的系数
  aay_sum += aay * n_sample;  //本例系数为9/7
  aay = (aay_sum / (11 * n_sample / 2.0)) * 9 / 7.0;
  aazs[n_sample - 1] = aaz;
  aaz_sum += aaz * n_sample;
  aaz = (aaz_sum / (11 * n_sample / 2.0)) * 9 / 7.0;

  float gyrox = -(gx - gxo) / GyroRatio * dt;  // x轴角速度
  float gyroy = -(gy - gyo) / GyroRatio * dt;  // y轴角速度
  float gyroz = -(gz - gzo) / GyroRatio * dt;  // z轴角速度
  agx += gyrox;                                // x轴角速度积分
  agy += gyroy;                                // x轴角速度积分
  agz += gyroz;

  /* kalman start */
  Sx = 0;
  Rx = 0;
  Sy = 0;
  Ry = 0;
  Sz = 0;
  Rz = 0;

  for (int i = 1; i < 10; i++) {  //测量值平均值运算
    a_x[i - 1] = a_x[i];          //即加速度平均值
    Sx += a_x[i];
    a_y[i - 1] = a_y[i];
    Sy += a_y[i];
    a_z[i - 1] = a_z[i];
    Sz += a_z[i];
  }

  a_x[9] = aax;
  Sx += aax;
  Sx /= 10;  // x轴加速度平均值
  a_y[9] = aay;
  Sy += aay;
  Sy /= 10;  // y轴加速度平均值
  a_z[9] = aaz;
  Sz += aaz;
  Sz /= 10;

  for (int i = 0; i < 10; i++) {
    Rx += sq(a_x[i] - Sx);
    Ry += sq(a_y[i] - Sy);
    Rz += sq(a_z[i] - Sz);
  }

  Rx = Rx / 9;  //得到方差
  Ry = Ry / 9;
  Rz = Rz / 9;

  Px = Px + 0.0025;              // 0.0025在下面有说明...
  Kx = Px / (Px + Rx);           //计算卡尔曼增益
  agx = agx + Kx * (aax - agx);  //陀螺仪角度与加速度计速度叠加
  Px = (1 - Kx) * Px;            //更新p值

  Py = Py + 0.0025;
  Ky = Py / (Py + Ry);
  agy = agy + Ky * (aay - agy);
  Py = (1 - Ky) * Py;

  Pz = Pz + 0.0025;
  Kz = Pz / (Pz + Rz);
  agz = agz + Kz * (aaz - agz);
  Pz = (1 - Kz) * Pz;

  /* kalman end */

  Serial.print(agx);
  Serial.print(",");
  Serial.print(agy);
  Serial.print(",");
  Serial.print(agz);
  Serial.println();
}

void setup() {  // Setup function
  Serial.begin(115200);
  while (!Serial) {
  }
  Serial.println("Serial Started");
  ZigbeeSerial.begin(115200);
  ZigbeeSerial.println("ZigbeeSerial Started");

  pinMode(servoXPin, OUTPUT);
  pinMode(servoYPin, OUTPUT);
  pinMode(pinSW, INPUT);

  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_u);

  u8x8log.begin(u8x8, U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
  WelcomeScreen();

  servoX.attach(servoXPin, ServoMin, ServoMax);
  servoY.attach(servoYPin, ServoMin, ServoMax);
  Xin = 0;
  Yin = 0;
  servoX.write(Xin);
  servoY.write(Yin);
  // mpu.initialize();
  // Serial.println(mpu.testConnection());

  delay(200);
  // oledDisplay121("StarTracker", "Complete", "Wait for command.");
}

void loop() {
  if (ModeSelect == 0) {  // Normal Mode
    SW = analogRead(pinSW);
  } else {  // Debug Mode
    SW = ModeSelect;
  }

  if (SW < 300) {  // Manual Mode
    Mode = "Manual";
  } else if (SW < 700) {  // Serial Mode
    Mode = "Serial";
  } else if (SW < 1023) {  // Wireless Mode
    Mode = "Wireless";
  }

  Mode3();

  delay(50);
  mainCount++;
  if (mainCount >= 10) {
    mainCount = 0;
    u8x8log.print("\f");
    Serial.println("Yaw: " + String(Xin) + " Pitch: " + String(Yin) +
                   " Status: " + Mode);
    delay(100);
  }
}
