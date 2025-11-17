# RCKid mk3

This is the third iteration of the RCKid handheld gaming device for kids. The handheld is a homage to 90's handheld consoles with easy controls and swappable cartridges that offer both software *and* hardware extensibility. 

Technical specifications:

- powered by RP2350 (2x ARM Cortex M-33 or RISC-V, 520KB RAM)
- 320x240 IPS LCD screen at 60fps and 65536 colors (framebuffer and tilemap modes)
- mono speaker, stereo headphones out, I2S 16bit sound at 48kHz
- 3 axis accelerometer, pedometer, ambient light sensor
- rumbler
- DPAD, A, B, Sel and Start keys with own backlight, Home and dedicated volume buttons
- SD card for large file storage
- ~1000mAh Li-Pol battery for ~10hrs (?) of operation

Cartridge specifications:

- flash up to 16MB
- 10 GPIO pins
- 3V3 capable of 600mA loads
- optional PSRAM support (up to 8MB)

Almost identical to mkII with RP2040, but ~1mm wider for easier & more secure battery installation and ~1mm thicker to accomodate for accessible SD card holder and display connector. The cartridge connector is also upgraded from CNC machinec pins that are expensive on the cartridge side and quite fragile to custom built cartridge connector for "cheap" gold fingered cartridges.

![RCKid mkII](docs/photos/rckid.jpg "RCKid prototype. The specks of dust on screen are not speck of dust but moving stars in the game:)")

## Directory Structure

- `cartridges` folder contains specific cartridge ROMs that can be build as part of RCKid
- `datasheets` contains copies of datasheets of the hardware used in RCKid
- `hardware` contains hardware related files, such as schematics, PCB layouts and case drawings
- `libs` holds mostly 3rd party libraries that are part of the RCKid SDK
- `sdk` contains the `librckid` SDK library files and all backends
- `lego-remote` AVR firmware for a remote client to control lego bricks, at the moment just parked here from mkI

## Building

Although there is technically nothing that should prevent building the software on Windows, only Linux is officially supported (actually Ubuntu 24.04 running on WSL:). The `setup-ubuntu.sh` must first be executed which installs all the required packages and sets up subprojects, etc.

RCKid uses `cmake` so the following builds the fantasy console & all cartridges on linux:

    mkdir build
    cd build
    cmake ..
    cmake --build .

To build RCKid for the device (mk III in this example), do the following:

    mkdir build-mk3
    cd build-mk3
    cmake .. -DRCKID_BACKEND=MK3
    cmake --build .

### Build Arguments

RCKid's build can be customized using various arguments, such as the `-DARCH` showed above to build for a particular architecture. This section lists other useful build arguments:

- `-DRCKID_WAIT_FOR_SERIAL` runs RCKid to the end of the initialize() function and then waits for a single byte to be sent on the USB-Serial. This is useful to ensure that a serial monitor on the pc is up & running before RCKid's firmware starts doing stuff so that it can be captured properly.

## SDK

The SDK library is at the core of RCKid as it provides an abstraction layer over the console's hardware. Furthermore, it makes RCKid also a fantasy console by being able to run on a PC for most of the features. Therefore the SDK comes in two folder, `rckid` where the common interface resides, and `backends` where specific implementation for the various hardware versions and fantasy consoles is implemented. 

> For now, fantasy console via raylib (Windows and Linux) and  RCKid mk3 (RP2350) are supported. There are plans for future platforms & variations. Note that the fantasy platform is mostly for debugging only.

## Debugging on the Device

    sudo apt-get install pkg-config libjim-dev libudev-dev
    git clone https://github.com/raspberrypi/openocd.git
    
    cd openocd
    ./bootstrap
    ./configure --disable-werror --enable-sysfsgpio --enable-bcm2835gpio
    make -j4
    sudo make install

Connect the SWD port for the RCKid (on devel-server the wires are, from top to bottom, looking from the back):

         | GND   |
    -----|-------|---------
    blue | green | yeallow
    

Then run openocd on the rpi with the following command:

    openocd -f interface/raspberrypi-swd.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "bindto 0.0.0.0"

And to run the debugger, can run gdb from the computer that compiled the cartridges:

    gdb app.elf
    target remote IP_ADDR:3333

Where `IP_ADDR` is the IP address of the rpi server. 

(from https://betanet.net/view-post/using-openocd-on-raspberry-pi-4-a)

### AVR Serial TX

AVR does not support on-chip debugging, but to provide at least some debugging hints, the TX pin is available on the debugging header together with AVR UPDI and RP SWD pins. To connect to it connect the TX pin (green cable in devel-server case) and then launch picocom at 115200 baud:

    picocom -b 115200 /dev/ttyAMA0

Where `/dev/ttyAMA0` is your USB device, this one is the default with devel-server. 

> To exit picocom, use C-A C-X. 

### Debug Cartridge

For easier debugging, the debug cartridge has protruding wires to some of the GPIO pins available, namely:

Wire Color | Function
-----------|-----------
Black      | GND
Red        | 3V3
Green      | GPIO 12, UART0 TX
White      | GPIO 13, UART0 RX
Blue       | GPIO 14
Yellow     | GPIO 15

## Attribution

- doxygen theme: https://github.com/jothepro/doxygen-awesome-css
- icons are from flaticon, for detailed attribution, please see ATTRIBUTION.md file 
