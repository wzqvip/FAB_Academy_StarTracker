#define Slave (1)  // Use this to define if the Arduino is the master or slave

#include <Arduino.h>
#include <I2Cdev.h>
#include <SPI.h>
#include <Wire.h>

#ifndef Slave
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ServoSmooth.h>
#include <SoftwareSerial.h>

#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 32     // OLED display height, in pixels
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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
// bool Status = false;
int mainCount = 0;
ServoSmooth servoX;
ServoSmooth servoY;

bool ManualMode = false;
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
const int ModeSelect = 1;
const int WirelessSerial = 1;
String Mode;

bool rowed = false;

SoftwareSerial ZigbeeSerial(softRX, softTX);

bool servo_driver(int Xin, int Yin) {  // Function to drive the servos

    if (Xin <= 180 and Xin >= 0) {
        X = 180 - Xin;
        Y = 180 - Yin;
    } else if (Xin <= 360) {
        X = 360 - Xin;
        Y = Yin;
    } else {
        return false;
    }
    servoX.setTargetDeg(X);
    servoY.setTargetDeg(Y);
    return true;
}

void WelcomeScreen() {
    display.clearDisplay();

    display.setTextSize(2);  // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 0);
    display.println(F("Initializ Standby"));
    display.display();  // Show initial text
    delay(100);

    // Scroll in various directions, pausing in-between:
    display.startscrollright(0x00, 0x0F);
    delay(2500);
}

void oledDisplay111(String str1, String str2, String str3, String str4) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println((str1));
    display.println((str2));
    display.println((str3));
    display.println((str4));
    display.display();
}

void oledDisplay121(String str1, String str2, String str3) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.println(str1);
    display.setTextSize(2);
    display.println(str2);
    display.setTextSize(1);
    display.println(str3);
    display.display();
}

void ManualDriver() {
    oledDisplay111("MANUAL MODE", "X POS: " + String(Xin),
                   "Y POS: " + String(Yin), "WORKING");

    Xin = map(analogRead(pinX), 0, 1023, 0, 360);
    Yin = map(analogRead(pinY), 0, 1023, 0, 90);
    if (mainCount % 10 == 0) {
        servo_driver(Xin, Yin);
    }
}

void SerialDriver() {
    if (WirelessSerial) {
        String a;
        int count = 0;
        if (ZigbeeSerial.available()) {
            // ZigbeeSerial.println("Client Received Message!");
            Serial.println("Received Message!");
            char c = ZigbeeSerial.read();
            while (c != '!' && count < 100) {
                a += c;
                count++;
                c = ZigbeeSerial.read();
            }
            Serial.println(a);
        }
        if (a.length() > 0) {
            String value1, value2;
            for (unsigned int i = 0; i < a.length(); i++) {
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
            Serial.println("Received Message!");
            String a = Serial.readString();
            String value1, value2;
            for (unsigned int i = 0; i < a.length(); i++) {
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
const int softRX = 2;
const int softTX = 3;

SoftwareSerial ZigbeeSerial(softRX, softTX);

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
        delay(200);
        String Stat = "";
        Stat += String(ypr[0] * 180 / M_PI);
        Stat += ",";
        Stat += String(-ypr[2] * 180 / M_PI);
        Stat += "!";
        ZigbeeSerial.println(Stat);
        Serial.print("Send Message: " + Stat);
    }
}
#endif

void setup() {  // Setup function
    Wire.begin();
    Serial.begin(115200);
    while (!Serial) {
    }
    Serial.println("Serial Started");
    ZigbeeSerial.begin(115200);
    ZigbeeSerial.println("ZigbeeSerial Started");
    pinMode(LED_BUILTIN, OUTPUT);

#ifndef Slave
    Serial.println("Master Mode");
    pinMode(servoXPin, OUTPUT);
    pinMode(servoYPin, OUTPUT);
    pinMode(pinSW, INPUT);

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ;  // Don't proceed, loop forever
    }
    display.display();
    delay(2000);  // Pause for 2 seconds
    display.clearDisplay();

    WelcomeScreen();

    servoX.attach(servoXPin, ServoMin, ServoMax);
    servoY.attach(servoYPin, ServoMin, ServoMax);
    servoX.setSpeed(100);
    servoY.setSpeed(200);
    servoX.setAccel(0.2);
    servoY.setAccel(0.5);
    servoX.setAutoDetach(false);
    servoY.setAutoDetach(false);

    X = 0;
    Y = 90;
    Xin = X;
    Yin = Y;
    servoX.setTargetDeg(Xin);
    servoY.setTargetDeg(Yin);
    display.stopscroll();
    display.clearDisplay();
#endif  // Master

#ifdef Slave
    digitalWrite(LED_BUILTIN, LOW);
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
    digitalWrite(13, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);
#endif /* Slave */

    delay(200);
}

void loop() {
#ifndef Slave
    if (ModeSelect == 0) {  // Normal Mode
        if (ManualMode) {
            ManualDriver();
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

    mainCount++;
    if (mainCount >= 30) {
        mainCount = 0;
        Serial.println("Yaw: " + String(Xin) + " Pitch: " + String(Yin) +
                       " Status: " + ManualMode);
    }

    servoX.tick();
    servoY.tick();

#endif

#ifdef Slave
    PositionSend();
    digitalWrite(LED_BUILTIN, HIGH);
#endif
}
