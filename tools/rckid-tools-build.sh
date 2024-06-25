#!/bin/bash
echo "Building tools"
mkdir build-tools
cd build-tools
cmake ../tools
cmake --build .
