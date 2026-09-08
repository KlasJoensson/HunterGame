# Hunter game

## Overview

A small game made for learning how to connect a display and a joystick to a board from Nordic semiconductor. The board used is nRF52840-DK.

## The parts connected to the board

### The display

The display used was a GC9A01 1.28" round SPI display with a resolution of 240x240 pixels.</br>
The display is connected to the board via pin P1.10 to P1.15, ground and 3 V VDD. As in the table below:</br>

|Pin on display|Pin on the board|
|--------------|----------------|
|  SCL / SCK   |      P1.15     |
|  SDA / MOSI  |      P1.13     |
|     CS       |      P1.12     |
|     DC       |      P1.11     |
|     RST      |      P1.10     |
|     VCC      |    VDD  (3*V3) |
|     GND      |       GND      |

### The Joystick

The joystick was a TAC-2 joystick. It was connected to the board via a 9-pin serial connector with cords from six of the nine pins (the three unused pins of the joystick wasn't connected) to pin P1.04 to P1.05 and ground, as shown in the table below:</br>

|Pin (connector) | Pin on the board | Joystick input |
|----------------|------------------|----------------|
|       1        |      P1.05       |       Up       |
|       2        |      P1.06       |      Down      |
|       3        |      P1.07       |      Left      |
|       4        |      P1.08       |      Right     |
|       5        |       ---        |       ---      |
|       6        |      P1.04       |     Buttons    |
|       7        |       ---        |       ---      |
|       8        |       GND        |     GND/VCC    |
|       9        |       ---        |       ---      |

## The game

The game is started by pressing one of the buttons, as the text on the display states. The player is represented by a circle that starts in the center of the screen and can be moved with help of the joystick. After 2,5 seconds a smaller circle is displayed and the idea is to catch it before it, after about five seconds, turns in to a square.</br>
If the player catches the small circle before it becomes a square he will get one pont (the points are displayed in the bottom of the screen) and a new small circle will appear after approximately 2,5 seconds.</br>
Once the small circle becomes a square will move towards the player and tries to catch it for about 5 seconds, if it dose it's game over. If the player manage to escape a new small circle to catch is created after approximately 2,5 seconds.</br>
As the points increase the small "food" circle will appear faster, but it will become a hunter faster as well...
