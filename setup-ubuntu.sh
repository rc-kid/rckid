#! /bin/bash

KIDDO_DIR=$(pwd)
echo "Setting up kidDo development in ${KIDDO_DIR}"

echo "    installing packages for kidDo SDK..."
sudo apt update -qq
sudo apt-get install -y -qq cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential libstdc++-arm-none-eabi-newlib ninja-build doxygen graphviz exfat-fuse

echo "    installing pico-sdk..."
cd ${KIDDO_DIR}/lib
git clone https://github.com/raspberrypi/pico-sdk.git --branch master -o pico-sdk
cd pico-sdk
git submodule update --init
cd ${KIDDO_DIR}

echo "    installing packages for kidDo fantasy console..."
echo "        x11 & graphics..."
sudo apt-get install -y -qq libx11-dev libxcursor-dev libxrandr-dev libxinerama-dev libxi-dev libgl1-mesa-dev libwayland-dev libxkbcommon-dev

echo "        curl..."
sudo apt-get install -y -qq libcurl4-openssl-dev curl

echo "        raylib..."
cd ${KIDDO_DIR}/lib
#sudo apt-get install vlc alsa-utils
git clone https://github.com/raysan5/raylib.git
#cd raylib/src
#make PLATFORM=PLATFORM_DESKTOP
cd ${KIDDO_DIR}

echo "        emscripten..."
sudo apt-get install -y -qq python3 nodejs
cd ${KIDDO_DIR}/lib
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
cd ${KIDDO_DIR}

#echo "Installing platformio..."
sudo apt-get -y install python3-venv
curl -fsSL -o get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py && python3 get-platformio.py
rm get-platformio.py
#ln -s /home/YOUR_USERNAME/.platformio/penv/bin/platformio ~/.local/bin/pio

#echo "Building kidDo & cartridges..."
#cd ${KIDDO_DIR}
#echo "    fantasy console (build)..."
#mkdir -p build
#cd build
#cmake .. -DARCH=ARCH_FANTASY -GNinja
#cmake --build .
#cd ..
#
#echo "    kidDo mk III (build-mk3)..."

