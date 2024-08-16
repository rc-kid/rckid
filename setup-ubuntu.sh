#! /bin/bash

RCKID_DIR=$(pwd)
echo "Setting up RCKid development in ${RCKID_DIR}"
mkdir -p libs

echo "    installing packages for RCKid SDK..."
sudo apt update -qq
sudo apt-get install -y -qq cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential libstdc++-arm-none-eabi-newlib ninja-build doxygen graphviz

echo "    installing pico-sdk..."
cd ${RCKID_DIR}/sdk/backends/arch_rckid
git clone https://github.com/raspberrypi/pico-sdk.git --branch master -o pico-sdk
cd pico-sdk
git submodule update --init
cd ${RCKID_DIR}

echo "    installing packages for RCKid fantasy console..."
echo "        x11 & graphics..."
sudo apt-get install -y -qq libx11-dev libxcursor-dev libxrandr-dev libxinerama-dev libxi-dev libgl1-mesa-dev libwayland-dev libxkbcommon-dev

echo "        curl..."
sudo apt-get install -y -qq libcurl4-openssl-dev curl

echo "        raylib..."
cd ${RCKID_DIR}/sdk/backends/arch_fantasy
#sudo apt-get install vlc alsa-utils
git clone https://github.com/raysan5/raylib.git
#cd raylib/src
#make PLATFORM=PLATFORM_DESKTOP
cd ${RCKID_DIR}

# TODO this 
#echo "Installing platformio..."
#sudo apt-get install -y -qq install python3 python3.10-venv

#echo "Building RCKid & cartridges..."
#cd ${RCKID_DIR}
#echo "    fantasy console (build)..."
#mkdir -p build
#cd build
#cmake .. -DARCH=ARCH_FANTASY -GNinja
#cmake --build .
#cd ..
#
#echo "    RCKid mk III (build-mk3)..."

