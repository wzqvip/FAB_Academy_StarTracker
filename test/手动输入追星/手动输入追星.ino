#include <Servo.h>

int pinY = A0;
int pinX = A1;
int pinSW = A2;

int Xin,Yin,X,Y,SW;
Servo servoX;
Servo servoY;

void setup(){
  servoX.attach(10,470,2500);
  servoY.attach(9,470,2500);


<<<<<<< Updated upstream:src/手动输入追星/手动输入追星.ino
  X = 0;
  Y = 0;
  // X = 90;
  // Y = 90;
  // X = 180;
  // Y = 180;

  Xin = 0;
  Yin = 0;
  // Xin = 154;
  // Yin = 71;
=======
  Xin = 150;
  Yin = 44;
>>>>>>> Stashed changes:src/手动输入追星.ino




}

void loop(){
  if(Xin == 360)
    Xin = 0;
  if(Xin<360)
    Xin++;
  Yin=Xin/4;


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


  servoX.write(X);
  servoY.write(Y);
  delay(100);
}