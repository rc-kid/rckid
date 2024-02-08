#!/bin/bash 

echo "Installing packages..."
sudo apt update
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential libstdc++-arm-none-eabi-newlib curl python3 python3.10-venv 
sudo apt install libx11-dev libxcursor-dev libxrandr-dev libxinerama-dev libxi-dev libgl1-mesa-dev

cd libs

if [[ ! -f libs/get-platformio.py ]]; then 
    echo "Installing platformio..."
    curl -fsSL -o get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
    python3 get-platformio.py
fi

echo "Installing pico-sdk..."
git clone https://github.com/raspberrypi/pico-sdk.git --branch master -o pico-sdk
cd pico-sdk
git submodule update --init
cd ..

#echo "Installing pico-examples..."
#git clone https://github.com/raspberrypi/pico-examples.git --branch master -o pico-examples

echo "Installing raylib for mock builds..."
cd libs
#sudo apt-get install vlc alsa-utils
git clone https://github.com/raysan5/raylib.git
cd raylib/src
make PLATFORM=PLATFORM_DESKTOP
cd ../../..


echo "Installing FatFS..."
git clone https://github.com/zduka/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico.git sd-card

#git clone https://github.com/rc-kid/PNGdec -o PNGdec

