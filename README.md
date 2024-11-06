# RCKid mk3

This is the third iteration of the RCKid handheld gaming device for kids. The handheld is a homage to 90's handheld consoles with limited and easy controls, and swappable cartridges that offer both software *and* hardware extensibility. 

Technical specifications:

- powered by RP2350 (2x ARM Cortex M-33 or RISC-V, 512KB RAM)
- 320x240 IPS LCD screen at 60fps and 65536 colors (framebuffer and tilemap modes)
- mono speaker and stereo headphones out
- 3 axis accelerometer, ambient light sensor
- rumbler
- DPAD, A, B, Sel and Start keys with own backlight, Home and dedicated volume buttons
- SD card for large file storage
- ~1000mAh Li-Pol battery for ~10hrs of operation

Cartridge specifications:

- flash up to 16MB
- 10 GPIO pins
- 3V3 capable of 600mA loads
- optional PSRAM support (up to 8MB)

Externally, it is almost identical to the version 2 built upon RP2040, but internally uses the new RP2350 chip instead. This allows both simplifications in the design (no need for AVR coprocessor because of higher pin count on RP chip) as well as improvements (less RAM pressure, possibility of external RAM in cartridges and more CPU raw power that makes more things possible).

## Directory Structure

- `cartridges` folder contains specific cartridge ROMs that can be build as part of RCKid
- `datasheets` contains copies of datasheets of the hardware used in RCKid
- `hardware` contains hardware related files, such as schematics, PCB layouts and case drawings
- `libs` holds mostly 3rd party libraries that are part of the RCKid SDK
- `sdk` contains the `librckid` SDK library files and all backends

## Building

Windows & WSL Ubuntu are supported for both development and fantasy console, anothough linux is much more tested. On linux, the `setup-ubuntu.sh` must first be executed which installs all the required packages and sets up subprojects, etc.

> This step is necessary for windows development as well as it installs the rpi-pico sdk and raylib for fantasy console.

RCKid uses `cmake` so the following builds the fantasy console & all cartridges on linux:

    mkdir build-linux
    cd build-linux
    cmake ..
    cmake --build .

To build RCKid for the device (mk III in this example), do the following:

    mkdir build-mk3
    cd build-mk3
    cmake .. -DARCH=ARCH_RCKID_3
    cmake --build .

And finally, under Windows, enter the Visual Studio Developers prompt and then type:

    mkdir build-win
    cd build-win
    cmake ..
    cmake --build . --config=Release

## SDK

The SDK library is at the core of RCKid as it provides an abstraction layer over the console's hardware. Furthermore, it makes RCKid also a fantasy console by being able to run on a PC for most of the features. Therefore the SDK comes in two folder, `rckid` where the common interface resides, and `backends` where specific implementation for the various hardware versions and fantasy consoles is implemented. 

> For now, fantasy console via raylib (Windows and Linux), RCKid mk3 (RP2350) and RCKid mk2 (RP2040) are supported. Once mk3 hardware is available the mk2 version will be retired. 

## Attribution

- doxygen theme: https://github.com/jothepro/doxygen-awesome-css
- icons are from flaticon, for detailed attribution, please see ATTRIBUTION.md file 
