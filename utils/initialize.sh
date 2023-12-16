#!/bin/bash 

sudo apt update
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential libstdc++-arm-none-eabi-newlib

mkdir -p libs
cd libs
git clone https://github.com/raspberrypi/pico-sdk.git --branch master -o pico-sdk
cd pico-sdk
git submodule update --init
cd ..
git clone https://github.com/raspberrypi/pico-examples.git --branch master -o pico-examples

git clone https://github.com/zduka/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico.git -o sd-card

git clone https://github.com/rc-kid/PNGdec -o PNGdec
