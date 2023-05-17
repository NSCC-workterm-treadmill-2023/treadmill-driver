# Treadmill Driver

This code is designed to replace the front panel of the Vision Fitness
[T9800 treadmill](https://www.toughtrain.com/vision-fitness/vision-fitness-treadmills/vision-t9800hrt-platform-treadmill) (now discontinued),
interfacing directly with the motor controller board in the bottom. This version only handles elevation.

## Usage

See the [API doc](docs/API.md) for recommended usage. You can still use the old way though:

Hold the Nucleo's user button down to raise the treadmill, increasing the elevation. Hold the button down a second time to lower it.
Hold it a third time to raise it again, and so on. The current elevation will be displayed on the LCD screen.

## Hardware

* STM32 Nucleo [G071RB](https://www.st.com/en/evaluation-tools/nucleo-g071rb.html), because it runs at 3.3V.
* A [I2C 1602 LCD](http://www.handsontec.com/dataspecs/module/I2C_1602_LCD.pdf) display, for displaying the current elevation.
* An [Arduino ethernet shield V1](https://docs.arduino.cc/retired/shields/arduino-ethernet-shield-without-poe-module).

## Setup

In Arduino IDE, install:

* [LiquidCrystal I2C](https://github.com/johnrickman/LiquidCrystal_I2C)
* [Ethernet](https://www.arduino.cc/reference/en/libraries/ethernet/)
* [MQTT](https://github.com/256dpi/arduino-mqtt)

All of those are available in the library manager.

The treadmill connector has a notch on top. Pin 1 is top left, pin 2 is under pin 1, pin 3 is to the right of pin 1, pin 4 is
under pin 2, and so on.

1. Install Arduino IDE. This will make it easy to use an ethernet shield later.
1. Open Arduino IDE. In the IDE, open the board manager, search for stm32, and install `STM32 MCU based boards`.
1. Choose Nucleo-64 as your board.
1. Under Tools > Board part number, choose G071RB.
1. Power the 1602 display off the Nucleo's 5V pin.
1. Connect 1602's SDA and SCL to the standard Arduino I2C pins (16 and 17, respectively).
1. Connect Arduino pin 2 to treadmill connector pin 15.
1. Connect Arduino pin 3 to treadmill connector pin 14.
1. Connect Arduino pin 4 to treadmill connector pin 13.
1. Connect Arduino pin 7 to treadmill connector pin 5.
1. Connect Arduino pin A1 to treadmill connector pin 4.
1. Connect Arduino pin 13 to ethernet shield ICSP SCK pin.
1. Connect Arduino pin 12 to ethernet shield ICSP MISO pin.
1. Connect Arduino pin 11 to ethernet shield ICSP MOSI pin.
1. Connect Arduino pin 10 to ethernet shield pin 10.
1. Connect treadmill connector pins 1, 3, 7 and 8 to a ground rail. These are the actual grounds.
1. Connect a Nucleo ground pin to the same ground rail.
1. Connect treadmill connector pins 9, 17, 19 and 20 to ground. These are pins that we're ignoring for now.
1. Leave the rest of them disconnected. The motor controller provides power on these, but we don't need it.

![Schematic](docs/schematic-v0.2.png)