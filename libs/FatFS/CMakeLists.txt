cmake_minimum_required(VERSION 3.12)

file(GLOB SRC "FatFS/*.cpp" "FatFS/*.c" "FatFS/*.h")

add_library(FatFS ${SRC})
