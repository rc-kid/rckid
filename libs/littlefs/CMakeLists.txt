cmake_minimum_required(VERSION 3.12)

file(GLOB SRC "*.c" "*.h")

add_library(littlefs ${SRC})
