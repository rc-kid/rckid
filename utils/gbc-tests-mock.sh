#!/bin/bash
mkdir -p build-mock
cd build-mock
export DISPLAY=:0
cmake --build . && cartridges/gbcemu/gbcemu-test