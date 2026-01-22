cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c" "src/*.h")

add_library(PNGenc ${SRC})
