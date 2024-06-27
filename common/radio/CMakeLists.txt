cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SRC "*.h" "nrf.cpp")
add_library(librckid-radio-nrf ${SRC})
link_with_librckid(librckid-radio-nrf)