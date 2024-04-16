#!/bin/bash
mkdir -p build-mock
cd build-mock
export DISPLAY=:0
if [$# -eq 0]; then 
    cmake .. -DARCH=MOCK -DCMAKE_BUILD_TYPE=Debug
fi
cmake --build .
if [ $? -eq 0 ]; then
    if [ $# -eq 1 ]; then 
      cartridges/$1
    fi
fi