#!/bin/bash
echo "Building utililities"
mkdir build-utils
cd build-utils
cmake ../utils
cmake --build .
