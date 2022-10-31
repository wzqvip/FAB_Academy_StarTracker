#include <Arduino.h>
// #include <SSD1306Ascii.h>
// #include <SSD1306AsciiAvrI2c.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MPU6050.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#define I2C_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1  // This is needed but not used.

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
Servo servoX;
Servo servoY;
Adafruit_SSD1306 oled(128, 32, &Wire, OLED_RESET);
SoftwareSerial ZigbeeSerial(softRX, softTX);
MPU6050 mpu;

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

  float a_x[10] = {0}, a_y[10] = {0}, a_z[10] = {0}, g_x[10] = {0},
        g_y[10] = {0}, g_z[10] = {0};  //加速度计协方差计算队列
  float Px = 1, Rx, Kx, Sx, Vx, Qx;    // x轴卡尔曼变量
  float Py = 1, Ry, Ky, Sy, Vy, Qy;    // y轴卡尔曼变量
  float Pz = 1, Rz, Kz, Sz, Vz, Qz;    // z轴卡尔曼变量

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
    X = 180 - Xin;
    Y = 180 - Yin;
  } else if (Xin <= 360) {
    X = 360 - Xin;
    Y = Yin;
  } else {
    X = 0;
    Y = 0;
    return false;
  }
  servoX.write(X);
  servoY.write(Y);
  return true;
}

void Draw_Init_Interface(void) {
  for (size_t i = 0; i < 46; i = i + 5) {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(WHITE);
    oled.setCursor(25, 0);
    oled.println("Initialize... System");
    oled.drawRect(38, 18, 51, 6,
                  WHITE);  //以（38，38）为起点绘制一个长度51宽度为6的矩形
    oled.drawLine(40, 19, 40 + i, 40, WHITE);  //循环绘制线条达到显示进度的效果
    oled.drawLine(40, 21, 40 + i, 41, WHITE);
    oled.display();
    // delay(10);
    oled.clearDisplay();
  }
  oled.display();
}

void oledDisplay121(String str1, String str2,
                    String str3) {  // Function to drive the oled
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println(str1);
  oled.setTextSize(2);
  oled.println(str2);
  oled.setTextSize(1);
  oled.println(str3);
}

void oledDisplay111(String str1, String str2,
                    String str3) {  // Function to drive the oled
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println(str1);
  oled.println(str2);
  oled.println(str3);
}

void Mode1() {
  oledDisplay111("Manual Mode", "X: " + String(Xin), "Y: " + String(Yin));
  Xin = map(analogRead(pinX), 0, 1023, 0, 360);
  Yin = map(analogRead(pinY), 0, 1023, 0, 360);
  servo_driver(Xin, Yin);
  delay(3000);  // This has a shorter delay to make the servos more responsive
}

void Mode2() {
  oledDisplay111("Serial Mode", "X: " + String(Xin), "Y: " + String(Yin));
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
    Status = servo_driver(Xin, Yin);
    delay(1000);
  }
}

void Mode3() {
  oledDisplay111("Wireless Mode", "X: " + String(Xin), "Y: " + String(Yin));
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
    Status = servo_driver(Xin, Yin);

    delay(1000);
  }
}

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
  
    unsigned short times = 200;             //采样次数
    for(int i=0;i<times;i++)
    {
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz); //读取六轴原始数值
        axo += ax; ayo += ay; azo += az;      //采样和
        gxo += gx; gyo += gy; gzo += gz;
    
    }
    
    axo /= times; ayo /= times; azo /= times; //计算加速度计偏移
    gxo /= times; gyo /= times; gzo /= times; //计算陀螺仪偏移
}

void calcMotion() {
  unsigned long now = millis();             //当前时间(ms)
    dt = (now - lastTime) / 1000.0;           //微分时间(s)
    lastTime = now;                           //上一次采样时间(ms)
 
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz); //读取六轴原始数值
 
    float accx = ax / AcceRatio;              //x轴加速度
    float accy = ay / AcceRatio;              //y轴加速度
    float accz = az / AcceRatio;              //z轴加速度
 
    aax = atan(accy / accz) * (-180) / pi;    //y轴对于z轴的夹角
    aay = atan(accx / accz) * 180 / pi;       //x轴对于z轴的夹角
    aaz = atan(accz / accy) * 180 / pi;       //z轴对于y轴的夹角
 
    aax_sum = 0;                              // 对于加速度计原始数据的滑动加权滤波算法
    aay_sum = 0;
    aaz_sum = 0;
  
    for(int i=1;i<n_sample;i++)
    {
        aaxs[i-1] = aaxs[i];
        aax_sum += aaxs[i] * i;
        aays[i-1] = aays[i];
        aay_sum += aays[i] * i;
        aazs[i-1] = aazs[i];
        aaz_sum += aazs[i] * i;
    
    }
    
    aaxs[n_sample-1] = aax;
    aax_sum += aax * n_sample;
    aax = (aax_sum / (11*n_sample/2.0)) * 9 / 7.0; //角度调幅至0-90°
    aays[n_sample-1] = aay;                        //此处应用实验法取得合适的系数
    aay_sum += aay * n_sample;                     //本例系数为9/7
    aay = (aay_sum / (11*n_sample/2.0)) * 9 / 7.0;
    aazs[n_sample-1] = aaz; 
    aaz_sum += aaz * n_sample;
    aaz = (aaz_sum / (11*n_sample/2.0)) * 9 / 7.0;
 
    float gyrox = - (gx-gxo) / GyroRatio * dt; //x轴角速度
    float gyroy = - (gy-gyo) / GyroRatio * dt; //y轴角速度
    float gyroz = - (gz-gzo) / GyroRatio * dt; //z轴角速度
    agx += gyrox;                             //x轴角速度积分
    agy += gyroy;                             //x轴角速度积分
    agz += gyroz;
    
    /* kalman start */
    Sx = 0; Rx = 0;
    Sy = 0; Ry = 0;
    Sz = 0; Rz = 0;
    
    for(int i=1;i<10;i++)
    {                 //测量值平均值运算
        a_x[i-1] = a_x[i];                      //即加速度平均值
        Sx += a_x[i];
        a_y[i-1] = a_y[i];
        Sy += a_y[i];
        a_z[i-1] = a_z[i];
        Sz += a_z[i];
    
    }
    
    a_x[9] = aax;
    Sx += aax;
    Sx /= 10;                                 //x轴加速度平均值
    a_y[9] = aay;
    Sy += aay;
    Sy /= 10;                                 //y轴加速度平均值
    a_z[9] = aaz;
    Sz += aaz;
    Sz /= 10;
 
    for(int i=0;i<10;i++)
    {
        Rx += sq(a_x[i] - Sx);
        Ry += sq(a_y[i] - Sy);
        Rz += sq(a_z[i] - Sz);
    
    }
    
    Rx = Rx / 9;                              //得到方差
    Ry = Ry / 9;                        
    Rz = Rz / 9;
  
    Px = Px + 0.0025;                         // 0.0025在下面有说明...
    Kx = Px / (Px + Rx);                      //计算卡尔曼增益
    agx = agx + Kx * (aax - agx);             //陀螺仪角度与加速度计速度叠加
    Px = (1 - Kx) * Px;                       //更新p值
 
    Py = Py + 0.0025;
    Ky = Py / (Py + Ry);
    agy = agy + Ky * (aay - agy); 
    Py = (1 - Ky) * Py;
  
    Pz = Pz + 0.0025;
    Kz = Pz / (Pz + Rz);
    agz = agz + Kz * (aaz - agz); 
    Pz = (1 - Kz) * Pz;
 
    /* kalman end */
 
    Serial.print(agx);Serial.print(",");
    Serial.print(agy);Serial.print(",");
    Serial.print(agz);Serial.println();
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

  oled.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS);
  oled.setTextColor(WHITE);
  oled.display();
  Draw_Init_Interface();
  servoX.attach(servoXPin, ServoMin, ServoMax);
  servoY.attach(servoYPin, ServoMin, ServoMax);
  Xin = 150;
  Yin = 44;
  mpu.initialize();
  Serial.println(mpu.testConnection());

  delay(200);
  oledDisplay121("StarTracker", "Complete", "Wait for command.");
}

void loop() {
  if (ModeSelect == 0) {  // Normal Mode
    SW = analogRead(pinSW);
  } else {  // Debug Mode
    SW = ModeSelect;
  }

  if (0 < SW < 300) {  // Manual Mode
    Mode = "Manual";
  } else if (300 < SW < 700) {  // Serial Mode
    Mode = "Serial";
  } else if (700 < SW < 1023) {  // Wireless Mode
    Mode = "Wireless";
  }

  int count = 0;
  if (count >= 50) {
    count = 0;
    Serial.println("Yaw: " + String(Xin) + " Pitch: " + String(Yin) +
                   " Status: " + Mode);
  }
}
