# FAB_Academy_StarTracker

 Fablab Final Project.

[See at github.com/wzqvip/FAB_Academy_StarTracker](https://github.com/wzqvip/FAB_Academy_StarTracker "Code")

A star tracker, may point out the stars in the sky.

When you lost your distination, why dou't sit and admire the starry sky, they, the stars, may point you the way.

#### Ways to operate this

* Use X and Y adjust lever.
* Type in the direction of the star.
* Type in the name of the star.
* Point by hand and it will follow your finger.

#### Materials:

See the readme on [wikifactory](https://wikifactory.com/+fablabo/shanghai-tech-fablab-2022) and the poster.

#### Star Position Calculate Website

###### Database:

[http://www.stellar-database.com/](http://www.stellar-database.com/)

[http://simbad.u-strasbg.fr/simbad/](http://simbad.u-strasbg.fr/simbad/)



#### Compile

Use PlatformIO

```
Processing nanoatmega328 (platform: atmelavr; board: nanoatmega328; framework: arduino)
---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Verbose mode can be enabled via `-v, --verbose` option
CONFIGURATION: https://docs.platformio.org/page/boards/atmelavr/nanoatmega328.html
PLATFORM: Atmel AVR (4.0.0) > Arduino Nano ATmega328
HARDWARE: ATMEGA328P 16MHz, 2KB RAM, 30KB Flash
DEBUG: Current (avr-stub) On-board (avr-stub, simavr)
PACKAGES:
 - framework-arduino-avr @ 5.1.0
 - toolchain-atmelavr @ 1.70300.191015 (7.3.0)
LDF: Library Dependency Finder -> https://bit.ly/configure-pio-ldf
LDF Modes: Finder ~ chain, Compatibility ~ soft
Found 10 compatible libraries
Scanning dependencies...
Dependency Graph
|-- I2Cdevlib-Core @ 1.0.0
|   |-- Wire @ 1.0
|-- I2Cdevlib-MPU6050 @ 1.0.0
|   |-- I2Cdevlib-Core @ 1.0.0
|   |   |-- Wire @ 1.0
|   |-- Wire @ 1.0
|-- Servo @ 1.1.8
|-- SoftwareSerial @ 1.0
|-- U8g2 @ 2.33.15
|   |-- SPI @ 1.0
|   |-- Wire @ 1.0
|-- Wire @ 1.0
Building in release mode
Checking size .pio\build\nanoatmega328\firmware.elf
Advanced Memory Usage is available via "PlatformIO Home > Project Inspect"
RAM:   [=====     ]  49.2% (used 1008 bytes from 2048 bytes)
Flash: [=====     ]  45.2% (used 13892 bytes from 30720 bytes)
=========================================================================================================== [SUCCESS] Took 1.50 seconds ===========================================================================================================
```
