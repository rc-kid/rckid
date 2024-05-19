# RCKid mk II

This repository is the RCKid mark II. Unlike the first version, the mark II attempts to mimic the olden handheld consoles more closely by using a much simpler main chip (RP2040 vs RPi Zero) and replaceable cartridges that can extend the handheld with both new software *and* hardware. 

This also means that the chip is seriously "underpowered" for most tasks mark I handled with ease, such as playing videos and emulating old games. Mark II puts much more emphasis on own content creation and fun (at least for me) by recreating the feel of old consoles for the programmers as well. 

This readme is intended for those who wish to develop their own cartridges for RCKid. When applicable, links will be put here for user guides, etc. 

> Please note that this is my toy project and as such will likely move *veeeery* slowly as its requirements in almost every department exceed my current capabilities (and when mistakes appear, probably my buget as well since the main PCB must be machine-assembled). 

## Building

After cloning the repository, run the `setup.sh` file:

    git clone git@github.com:rc-kid/rckid
    cd rckid
    bash setup.sh

This should install all necessary packages, including platformio and 3rd party libraries used by RCKid. 

> I'm building RCKid on Ubuntu 22.04 WSL, which is the only "officially" supported distribution, other linux flavours should work equally well, but the package manager commands must be updated.  

Building all cartridges that are part of this repo is then as easy as running the following:

    mkdir build
    cd build
    cmake ..
    cmake --build . 

The various cartridges will then be available under the `build/cartridges` folder and its subfolders in the `UF2` format. 

### Mock Builds

Alternatively, several parts of the RCKid library can also build locally on your dev machine for easier debugging and testing. To do so, run the following from RCKid's root folder:

    mkdir build-mock
    cd build-mock
    cmake .. -DARCH=MOCK
    cmake --build . 

> Be careful, for debugging under WSL, read the following [bug](https://github.com/microsoft/WSL/issues/8516) for now (installing gdb from the PPA as suggested in the discussion works for now)

## Debugging

TODO 

## Overview

The repository consists of various frameworks, libraries and executables for different architectures. The most important ones are:

- `include/platform` contains header only very low level hardware abstraction layer unifying all the platforms used (RP2040, ATTiny, etc.). The platform framework supports basic gpio operations, cpu features and i2c/spi communication
- `include/utils` various utilities
- `cartridges` contains the cartridges built for the RCKid. Each cartridge has its own image that can be flashed (see below)
- `avr/rckid` is the firmware for the AVR chip that takes care of reading user input, monitoring charging & power consumption and power modes
- `librckid` contains the RCKid SDK, see its own readme for more details. 
- `lbs` are 3rd party libraries used by the SDK
- `utils` are various utilities for RCKid development
- `hardware` contains schematics, PCB layout files and 3D models for the device and cartridges


## Quick Notes

- Debug mode, activated when normal mode is entered while the select button is pressed

- the oscillator is slow to start, see https://forums.raspberrypi.com/viewtopic.php?t=317631


==========



That said, I am hopeful, that over time, the following will be possible:

- music player
- own games (2d platformers, etc.)
- video player (not generic formats, but special preencoded videos tailodr toward the meagre processing power and memory of RP2040)
- games emulator (I am hoping at least C64 or Gameboy, having GBC would be epic)
- walkie talkie (like v1)
- remote controller (like v1)
- internet connectivity via either a WiFi to NRF beacon, or a dedicated ESP8266 cartridge (the later is more portable, but will drain battery)


- screw inserts (https://www.prusa3d.com/product/threaded-inserts-m2-short-100-pcs/)
- hole height 4mm, 3.2mm diameter and 1.3mm wall thickness around, this makes the actual structure thickness at least 1.6 + 1.3 = 2.9, which sort of barely fits, but ok:)

RCKid mk II is an RP2040 (the chip that powers RPi Pico) handheld device that can be used to play own games, emulate some very old consoles, play music and video files and act as a walkie-talkie or remote controller with the RCKid mk I protocol.

To resemble the handheld game consoles of the past, RCKid mk II uses swappable cartridges that can extend its functionality. At minimum, the cartridge must provide the flash chip for the RP2040 inside, but extra power, I2C, SPI and 4 GPIO pins are provided for HW expansion. 

## Device

The device consts of the RP2040 itself, a 2.8" 8bit 8080 display, audio in & out, accelerometer and other sensors.  

## Cartridges

The default cartridge 

## WiFi Beacon





This is an initial repository for my new project, RP2040 based smaller RCKid with actual cartridges for real nostalgia feel. The project should be relatively cheap, easy to build and its parts should be plentiful. The emphasis this time is not on emulation, but on creating own games and getting smart with the cartridges like in the olden days.

> !! Can preorder parts at JLCPCB to ensure they are available to me when I want to assemble them 

## Device

The device itself will contain the basic necessary hardware, including:

- 2.8" screen
- RP2040
- SD card for memory
- audio outputs (headphones & mono speaker)
- microphone
- battery & charger (https://www.tme.eu/cz/details/accu-lp503759_cl/akumulatory/cellevia-batteries/l503759/)
- sensors (accel, photo, ...)
- rumbler

### Stackup

> total stackup is 11mm

- 1mm plexiglass cover
- 3mm display & buttons & room for top 
- 1mm main PCB
- 5mm battery
- 1mm bottom 


### Display

> https://www.aliexpress.com/item/1005004635814413.html?spm=a2g0o.store_pc_groupList.8148356.36.28a0cb5fia4tAE&pdp_npi=3%40dis%21CZK%21CZK%20146.38%21CZK%20146.38%21%21%21%21%21%4021038edf16929925175593334e3f07%2112000029921223776%21sh%21CZ%213305825785#nav-review 

This should be a nice IPS screen with smaller margins for minimal footprint. Also seems to have all the required, including the tearing effect synchronization. Alternatively any non-IPS ILI9341 or ST7789 screen with the same pinout should be compatible as well at the expense of the display's active area not being centered perfectly 

Driven in 8080 8bit mode by the RP2040. 

### SD Card

Attached to the RP2040, uses the SPI interface. Contains the 





### RP2040 (30 pins)

- 12 8080 8bit display
- 4 SD card SPI
- 1 microphone
- 2 audio out

- 2 I2C (cartridge connector)
- 4 SPI (cartridge connector & SRAM)
- 4 GPIO (cartridge connector)

- free: 1 pin (used as CS for the SRAM chip)

### ATTiny1616 (17 pins)

> Using the smaller xx16 part as they are more readily available

- 2 I2C
- 7 (3x4 matrix) buttons (11 buttons - 4 dpad, A, B, Sel, Start, VolUp, VolDown, HOME) - 1 free
- 1 backlight
- 3 power, VBATT sense, charging sense
- 2 neopixel power + neopixel
- 2 headphones, speaker on

## Cartridges

Cartridges and the device connect via a single row of 20 precision (SIP connector, single row) pins with 2.54mm spacing. The connector will provide the following interfaces:

- QSPI for flash memory (6 pins)
- SPI (4)
- I2C (2)
- GPIO (4)
- GND (2)
- VBATT (1)
- 3V3 (1)

At a minimum, the cartridge must provide the QSPI flash for the RP2040. The design should allow for the inclusion of NRF24L01P and LoRa modules and IR sensors/diodes as well so that different cartridges can be created by populating different components:

- base cartridge (flash only)
- connected cartridge (flash + NRF/LoRA)
- IR remote (flash + IR)

Optional cartridges in next generation would include: 

- camera (SPI models only with buffer)
- WiFi connected cartridge (ESP8266 or some such)

## Lessons learned

- many screws necessary for the 3D printed parts to fit
- order plexiglass covers (https://plasticexpress.cz/), fairly cheap





# Things to try

- don't use battery sense pin on AVR (either VCC if not charging, or show charging when charging) (saves 1pin)
- BQ25895 or when used this, we can read voltage & charging current for better visualization (saves 2 pins)
- rename the pins in the cartridge connector
- INA3221 can tell me charging, VBUS and VSYS all at once. Maybe worth a try? 
- can then use smaller I2C charger w/o the status 


# Attributions

<div>Icons made by <a href="https://www.flaticon.com/authors/pixel-perfect" title="Pixel perfect">Pixel perfect</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/vectors-market" title="Vectors Market">Vectors Market</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/srip" title="srip">srip</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/aldo-cervantes" title="Aldo Cervantes">Aldo Cervantes</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/gowi" title="GOWI">GOWI</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.freepik.com" title="Freepik">Freepik</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/smashicons" title="Smashicons">Smashicons</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/umeicon" title="Umeicon">Umeicon</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/bukeicon" title="bukeicon">bukeicon</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/mavadee" title="mavadee">mavadee</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/wanicon" title="wanicon">wanicon</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/good-ware" title="Good Ware">Good Ware</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/apien" title="apien">apien</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/surang" title="surang">surang</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div><div>Icons made by <a href="https://www.flaticon.com/authors/chahir" title="chahir">chahir</a> from <a href="https://www.flaticon.com/" title="Flaticon">www.flaticon.com</a></div>

