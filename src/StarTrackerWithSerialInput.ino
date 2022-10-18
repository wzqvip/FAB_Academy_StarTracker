#include <Servo.h>
#include <Wire.h>

int pinY = A0;
int pinX = A1;
int pinSW = A2;

int Xin, Yin, X, Y, SW;
boolean Status = false;
Servo servoX;
Servo servoY;

Xin = 150;
Yin = 44;

bool servo_driver(int Xin, int Yin) {
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

void setup() {
    Serial.begin(115200);
    pinMode(pinY, OUTPUT);
    pinMode(pinX, OUTPUT);
    pinMode(pinSW, INPUT);
    servoX.attach(9, 470, 2500);
    servoY.attach(10, 470, 2500);
}

void loop() {
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