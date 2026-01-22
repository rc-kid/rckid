cmake_minimum_required(VERSION 3.12)

file(GLOB SRC "*.cpp" "*.c" "*.h")

add_library(FatFS ${SRC})
