# FAB_Academy_StarTracker

 Fablab Final Project.

A star tracker, may point out the stars in the sky.

When you lost your distination, why dou't sit and admire the starry sky, they, the stars, may point you the way.

#### Connections

* servoXPin = 9;
* servoYPin = 10;
* pinSW = A0;
* pinX = A1;
* pinY = A2;
* softRX = 2;
* softTX = 3;

Two servo motors are used to point the stars, one for X axis, one for Y axis.

A0 is the mode select pin, when it is low, the star tracker is in the manual mode, when it is medium, it is in Serial mode, when it is high, it is in the wireless mode.

A1 and A2 are the analog pins for the X and Y axis.

Pin2 and Pin3 are the pins for the software serial, we use the Zigbee module for communication.

And connect the oled display to the I2C pins. Which are A4 and A5 for SDA and SCL pins respectively

#### Functions

#### Star Position Calculate Website

###### Database:

[http://www.stellar-database.com/](http://www.stellar-database.com/)

[http://simbad.u-strasbg.fr/simbad/](http://simbad.u-strasbg.fr/simbad/)
