# RCKid mk3

RCKid is an open‑source handheld console designed for young creators. It’s built to be the first piece of technology that feels truly personal to a child — not just a screen to consume, but a tool to imagine, build, and share. RCKid also supports kids in everyday tools like a clock, alarm, piggy bank, contacts, or music player. This balance of fun and function turns RCKid into a trusted companion, introducing kids to digital literacy, technology, and STEM skills in a way that grows with them. For more information, have a look at the [grand vision](VISION.md) 

Starting at age 5, kids can design sprites, tiles, and music inside native games, learning problem‑solving naturally through play. As they grow, RCKid will provide more and more complex ways of control (visual blocks, scratch-like blocks, C++, Full C++ SDK). For more details see [development ladder](LADDER.md).

A defining feature is RCKid’s cartridge system — not just for games, but for extending hardware. Cartridges can add Wi‑Fi for messaging, radios for mesh networking, JACDAC peripherals, IR remotes, or even exposed pins for DIY tinkering. Each cartridge carries its own firmware, making creations tangible, shareable, and hackable.

Powered by the RP2350 MCU (dual Cortex‑M33, 520KB RAM, PIO), RCKid combines raw performance with developer‑friendly design. It features a 2.8" IPS display at 60 FPS, stereo audio up to 48kHz, SD storage, RGB‑backlit controls, accelerometer, haptics, FM radio, and flexible power (USB‑C Li‑Ion or AAA batteries). Everything is built to be fun, approachable, and resilient.

## Technical Details

- RP2350 MCU from RaspberryPi that mixes raw power (520KB RAM, 2x Cortex M33 cores at 150MHz with overclocking possibility) and ease of use (C++ SDK, micropython). Further supported by great community and skillfully designed so that programming it is *fun* even for experienced developers (PIO)
- unique cartridge system: the firmware is not stored on the device, but in every cartridge. Cartridges can be swapped, shared, or reprogrammed with any computer easily. On top of the mandatory FLASH for the firmware, cartridges contain 8 high speed digital pins (HSTX, SPI, I2C, UART, PWM) and 2 analog pins to enable hardware tinkering
- 2.8" 320x240 IPS display with 65536 colors. Perfect for retro gaming and pixel art with enough catchy detail, but not too many pixels to design. 60 FPS refresh rate.
- 16bit stereo sound (headphones & mono speaker) with up to 48kHz playback. Powerful enough for MP3 playback
- SD card for media & settings, up to 64GB supported. FAT32 and exFAT filesystem 
- DPAD, A, B, Select and Start buttons with customizable RGB backlight
- 3 axis accelerometer with integrated pedometer
- rumbler for haptic feedback (simple motor)
- FM radio with RDS
- 1300mAh LiIon rechargeable battery with USB-C charging, or 3x AAA batteries, both options should give around 10 hours of active time. 

![RCKid mkIII](docs/photos/mkIII-front.jpeg "RCKid prototype")

## Development

> The readme is mostly about setting the project up and understanding its development & internal structure. 

RCKid supports different backends, including a fantasy backend that allows running RCKid cartridges virtually on the PC, and the mkIII backend that runs on the actual device. Each backend provides its own implementation of the hardware abstraction layer (defined in `sdk/include/rckid/hal.h`) as well as capabilities (see below).  The fantasy backend, also includes test target (`run-tests`) that runs unit tests for the SDK and selected cartridges. It emulates the hardware of the device, including the display & sound using raylib, SD card and LittleFS on the cartridge using dedicated iso files and the FatFS and LittleFS libraries, or native folders via virtual filesystem (where the filesystem API is implemented for normal files, bypassing the fs libraries altogether for convenience). The fantasy backend also supports emspcripten.

## Project Structure

The project is organized into a few top level folders that branch into subfolders where appropriate. This is reflected in the brief list below:

- `sdk` : the SDK code (common firmware for the device, backends, ATTiny MCU, etc.)
- `sdk/assets` : asset files in their natural form (images, sounds, etc.)
- `sdk/backend` : the supported backends (implementation of the hardware abstraction layer)
- `sdk/backend/fantasy` : the fantasy backend for RCKid. This allows running RCKid cartridges virtually on the PC
- `sdk/backend/mk3` : the RP2350 and the device specific code
- `sdk/backend/avr` : firmware for the ATTiny3217 MCU responsible for IO, power management, etc
- `sdk/include/rckid` : SDK headers to be included into cartridges (the SDK API). Some code in this folder, rest in subfolders
- `sdk/include/rckid/apps` : applications for RCKid (pedometer, audio player, clock, etc.)
- `sdk/include/rckid/audio` : audio recording / playback, codecs, etc.
- `sdk/include/rckid/capabilities` : extra hardware capabilities (WiFi, pedometer, flashlight, etc.)
- `sdk/include/rckid/game` : game engine with C++ and dynamic bindings for kids to create their own games
- `sdk/include/rckid/graphics` : basic graphics utilities (colors & representation, bitmaps, fonts, etc.)
- `sdk/include/rckid/ui` : simple UI widgets toolkit 
- `sdk/include/assets` : assets for the rckid converted to constexpr arrays to be included with cartridges
- `sdk/src` : C++ implementation for the files in `sdk/include`, same structure internally
- `sdk/test` : unit tests (see testing chapter below)
- `lib` : contains 3rd party libraries that are copied / cloned as part of the SDK (including pico-sdk and raylib). Do not change code in here except for the platform below
- `lib/platform` : custom headers for different platforms (desktop, rp2350, ATTiny3217, Arduino, ...) for basic HW features and cross-platform utilities
- `cartridges` : cartridges for the RCkid
- `cartridges/text` : cartridges specifically for test purposes
- `gbcemu` : Gameboy emulator implementation for RCKid
- `datasheets` contains copies of datasheets of the hardware used in RCKid
- `hardware` contains hardware related files, such as schematics, PCB layouts and case drawings
- `lego-remote` AVR firmware for a remote client to control lego bricks, at the moment just parked here from mkI

## Important Files

Non-code files:

- `README.md` : basic intro the the repository, build instructions, etc.
- `VISION.md` : the vision for the project, you can read this for context when discussing non-code, but necessary for the code itself
- `LADDER.md` : the learning ladder for the project, which I am implementing in the `sdk/include/rckid/game` folder (from asset editor for youngest kids to visual editor to blocks to code DSL to full SDK)
- `HARDWARE.md` : more details about the hardware design, useful forhardware related questions

Code files:

- `sdk/include/rckid/hal.h` : functions that *must* be implemented by each backend to support the basic features (display, power, sound, io, etc.)
- `sdk/include/memory.h` : own memory management including custom heap on the device and virtual custom heap in fantasy mode to track memory consumption accurately



## Building

Although there is technically nothing that should prevent building the software on Windows, only Linux is officially supported (actually Ubuntu 24.04 running on WSL:). The `setup-ubuntu.sh` must first be executed which installs all the required packages and sets up subprojects, etc. The project uses `cmake` as the build system. Generally I use out of tree builds, which can be found in `build` prefixed folder, such as:

- `build` : for the fantasy target
- `build-mk3` : for the mkIII target
- `build-wasm` : for the fantasy target with emscripten

The build builds all libraries and the creates the executables/uf2 images in the `cartridges` subfolder. `platformio` is used for the ATTiny3217 firmware, which is independent project and can be built using the CLI. Useful source of build information is also the `.vscode/tasks.json` file, which contains tasks for building different targets. It also contains upload targets, which copy the build artifacts to another machine from which they are flashed.


RCKid uses `cmake` so the following builds the fantasy console & all cartridges on linux:

    mkdir -p build
    cd build
    cmake ..
    cmake --build . -j

To build RCKid for the device (mk III in this example), do the following:

    mkdir -p build-mk3
    cd build-mk3
    cmake .. -DRCKID_BACKEND=MK3
    cmake --build .

Finally, to build RCkid for the web using the emscripten toolchain, you can do the following:

    source ./lib/emsdk/emsdk_env.sh
    mkdir -p build-wasm
    cd build-wasm
    emcmake cmake ..
    cmake --build . -j

> Note that emscripten support is now very barebones, I have basically only checked that it works and added the build to CI so that I do not do something to jeopardize it.

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

Connect the SWD port for the RCKid (on devel-server the wires are, from top to bottom, looking from the back, from top to bottom):

         | GND   |
    -----|-------|---------
    blue | green | yellow
    

Then run openocd on the rpi with the following command:

    openocd -f interface/raspberrypi-swd.cfg -f target/rp2350.cfg -c "adapter speed 5000" -c "bindto 0.0.0.0"

And to run the debugger, can run gdb from the computer that compiled the cartridges:

    gdb app.elf
    target remote IP_ADDR:3333

Where `IP_ADDR` is the IP address of the rpi server. 

    monitor reset init
    

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
