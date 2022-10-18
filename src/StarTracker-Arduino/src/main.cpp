#include <Arduino.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiAvrI2c.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#define I2C_ADDRESS 0x3C

#define UseWirelessSerial 0;

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
SSD1306AsciiAvrI2c oled;
SoftwareSerial ZigbeeSerial(softRX, softTX);
const int ServoMin = 470;
const int ServoMax = 2500;

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

void oledDisplay(int x, int y, String s, String Status) {  // Function to drive the oled
    oled.println("Yaw: " + String(x) + " Pit: " + String(y));
    oled.set2X();
    oled.println(s);
    oled.set1X();
    oled.println(Status);
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

    oled.begin(&Adafruit128x32, I2C_ADDRESS);
    oled.setI2cClock(20000);
    oled.setFont(Adafruit5x7);
    oled.clear();

    servoX.attach(servoXPin, ServoMin, ServoMax);
    servoY.attach(servoYPin, ServoMin, ServoMax);
    Xin = 150;
    Yin = 44;
}

void loop() {
    SW = analogRead(pinSW);
    if (0 < SW < 300) {  // Manual Mode
        Serial.println("Manual Mode");
        Xin = map(analogRead(pinX), 0, 1023, 0, 360);
        Yin = map(analogRead(pinY), 0, 1023, 0, 360);
        servo_driver(Xin, Yin);
        Serial.println("Yaw: " + String(Xin) + " Pitch: " + String(Yin) + " Status: " + String(Status));

    } else if (300 < SW < 700) {  // Serial Mode
        Serial.println("Serial Mode");
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
            Serial.println("Yaw: " + String(Xin) + " Pitch: " + String(Yin) + " Status: " + String(Status));
        }

    } else if (700 < SW < 1023) {  // Wireless Mode
        Serial.println("Wireless Mode");
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
            Serial.println("Yaw: " + String(Xin) + " Pitch: " + String(Yin) + " Status: " + String(Status));
        }
    }
    delay(2000);
}
