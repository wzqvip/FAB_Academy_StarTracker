//#define Slave (1)  // Use this to define if the Arduino is the master or slave

#include <Arduino.h>
#include <I2Cdev.h>
#include <Wire.h>

#ifndef Slave
#include <Servo.h>
#include <SoftwareSerial.h>
#include <U8x8lib.h>
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
int Xcal = 0;
int Ycal = 0;
// bool Status = false;
int mainCount = 0;
Servo servoX;
Servo servoY;
bool ManualMode = true;
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
const int WirelessSerial = 1;
String Mode;

U8X8_SSD1306_128X32_UNIVISION_SW_I2C u8x8(/* clock=*/SCL, /* data=*/SDA,
                                          /* reset=*/U8X8_PIN_NONE);
uint8_t u8log_buffer[U8LOG_WIDTH * U8LOG_HEIGHT];
U8X8LOG u8x8log;
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
    if (Y > 180) {
        Y = 180;
    } else if (Y < 0) {
        Y = 0;
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
    u8x8.drawString(0, 3, "PLEASE WAIT");
    delay(200);
    u8x8.clear();
    u8x8.drawString(0, 0, "INITIALIZING...");
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

void ManualDriver() {
    oledDisplay111("MANUAL MODE", "X POS: " + String(Xin),
                   "Y POS: " + String(Yin), "WORKING");
    Xin = map(analogRead(pinX), 0, 1023, 0, 360);
    Yin = map(analogRead(pinY), 0, 1023, 0, 180);
    servo_driver(Xin, Yin);
}

void SerialDriver() {
    if (WirelessSerial) {
        String a;
        int count = 0;
        if (ZigbeeSerial.available()) {
            ZigbeeSerial.println("Client Received Message!");
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
    servo_driver(Xin, Yin);
    String Stat;
    if (WirelessSerial) {
        Stat = "WIRELESS MODE";
    } else {
        Stat = "SERIAL MODE";
    }
    oledDisplay111("SERIAL MODE", "RECEIVE X: " + String(Xin),
                   "RECEIVE Y: " + String(Yin), Stat);
}

#endif

#ifdef Slave
#include <MPU6050_6Axis_MotionApps20.h>
#include <SoftwareSerial.h>
MPU6050 mpu;
uint8_t fifoBuffer[64];
Quaternion q;
VectorFloat gravity;
float ypr[3];

void PositionSend() {
    if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
        mpu.dmpGetQuaternion(&q, fifoBuffer);
        mpu.dmpGetGravity(&gravity, &q);
        mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
        Serial.print("$");
        Serial.print(ypr[0] * 180 / M_PI);
        Serial.print(" ");
        Serial.print(ypr[1] * 180 / M_PI);
        Serial.print(" ");
        Serial.print(ypr[2] * 180 / M_PI);
        Serial.println(";");
    }
    delay(200);
    String Stat;
    Stat += String(ypr[0] * 180 / M_PI);
    Stat += ",";
    Stat += String(-ypr[2] * 180 / M_PI);
    Stat += "!";
    ZigbeeSerial.println(Stat);
}
#endif

SoftwareSerial ZigbeeSerial(softRX, softTX);

void setup() {  // Setup function
    Wire.begin();
    Serial.begin(115200);
    while (!Serial) {
    }
    Serial.println("Serial Started");
    ZigbeeSerial.begin(115200);
    ZigbeeSerial.println("ZigbeeSerial Started");

#ifndef Slave
    Serial.println("Master Mode");
    pinMode(servoXPin, OUTPUT);
    pinMode(servoYPin, OUTPUT);
    pinMode(pinSW, INPUT);

    u8x8.begin();
    u8x8.setFont(u8x8_font_chroma48medium8_u);
    u8x8log.begin(u8x8, U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
    WelcomeScreen();

    servoX.attach(servoXPin, ServoMin, ServoMax);
    servoY.attach(servoYPin, ServoMin, ServoMax);
    X = 0;
    Y = 90;
    Xin = X;
    Yin = Y;
    servoX.write(Xin);
    servoY.write(Yin);

#endif  // Master

#ifdef Slave
    ZigbeeSerial.println("Slave Mode");
    mpu.initialize();
    if (mpu.testConnection()) {
        Serial.println("MPU6050 connection successful");
    } else {
        Serial.println("MPU6050 connection failed");
    };
    mpu.dmpInitialize();
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);
    mpu.PrintActiveOffsets();
    mpu.setDMPEnabled(true);
#endif /* Slave */

    delay(200);
}

void loop() {
#ifndef Slave
    if (ModeSelect == 0) {  // Normal Mode
        if (ManualMode) {
            ManualDriver();
            delay(1);
        } else {
            SerialDriver();
            delay(10);
            //   if (btnState == LOW) {
            //     ManualMode = true;
            //   }
        }
    } else {  // Debug Mode
        if (ManualMode) {
            ManualDriver();
            delay(1);
        } else {
            SerialDriver();
            delay(10);
        }
    }

    // delay(1);
    // mainCount++;
    // if (mainCount >= 10) {
    //     mainCount = 0;
    //     u8x8log.print("\f");
    //     Serial.println("Yaw: " + String(Xin) + " Pitch: " + String(Yin) +
    //                    " Status: " + ManualMode);
    //     delay(1);
    // }
#endif

#ifdef Slave
    PositionSend();
#endif
}
