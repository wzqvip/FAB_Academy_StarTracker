#include <Arduino.h>
// #include <SSD1306Ascii.h>
// #include <SSD1306AsciiAvrI2c.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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
  Serial.println("Manual Mode");
  oledDisplay111("Manual Mode", "X: " + String(Xin), "Y: " + String(Yin));
  Xin = map(analogRead(pinX), 0, 1023, 0, 360);
  Yin = map(analogRead(pinY), 0, 1023, 0, 360);
  servo_driver(Xin, Yin);
  Serial.println("Yaw: " + String(Xin) + " Pitch: " + String(Yin) +
                 " Status: " + String(Status));
  delay(100);  // This has a shorter delay to make the servos more responsive
}

void Mode2() {
  Serial.println("Serial Mode");
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
    Serial.println("Yaw: " + String(Xin) + " Pitch: " + String(Yin) +
                   " Status: " + String(Status));
    delay(2000);
  }
}

void Mode3() {
  Serial.println("Wireless Mode");
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
    Serial.println("Yaw: " + String(Xin) + " Pitch: " + String(Yin) +
                   " Status: " + String(Status));
    delay(2000);
  }
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
  delay(200);
  oledDisplay121("StarTracker", "Complete", "Wait for command.");
}

void loop() {
  if (ModeSelect == 0) {  // Normal Mode
    SW = analogRead(pinSW);
  } else {  // Debug Mode
    SW = ModeSelect;
  }

  int count = 0;
  if (count >= 50) {
    count = 0;
  }

  if (0 < SW < 300) {  // Manual Mode

  } else if (300 < SW < 700) {  // Serial Mode

  } else if (700 < SW < 1023) {  // Wireless Mode
  }
