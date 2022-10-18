#include <Servo.h>

int pinY = A0;
int pinX = A1;
int pinSW = A2;

int Xin,Yin,X,Y,SW;
Servo servoX;
Servo servoY;

void setup(){
  servoX.attach(9,470,2500);
  servoY.attach(10,470,2500);



  Xin = 150;
  Yin = 44;



  if (Xin <= 180 and Xin >= 0){
    X = 180 - Xin;
    Y = 180 - Yin;
  }
  else if(Xin <= 360){
    X = 360 - Xin;
    Y = Yin;
  }
  else{
    X = 0;
    Y = 0;
  }
}

void loop(){
  servoX.write(X);
  servoY.write(Y);
  delay(100);
}