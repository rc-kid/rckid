#!/bin/bash
mkdir -p build-mock
cd build-mock
rm -rf ./*
cmake .. -DARCH=MOCK -DCMAKE_BUILD_TYPE=Debug
cmake --build .