#!/bin/bash
mkdir -p build-mock
cd build-mock
cmake .. -DARCH=MOCK -DCMAKE_BUILD_TYPE=Debug
cmake --build .