cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SRC "*.cpp" "*.c" "*.h")

add_library(libhelix-mp3 ${SRC})
