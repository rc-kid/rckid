# RCKid mk II
cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(${CMAKE_SOURCE_DIR}/libs/pico-sdk/pico_sdk_init.cmake)

project(rckid2 C CXX ASM)

# Initialize the SDK
pico_sdk_init()

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/assets)
include_directories(${CMAKE_SOURCE_DIR}/librckid)
include_directories(${CMAKE_SOURCE_DIR}/libs/sd-card)
include_directories(${CMAKE_SOURCE_DIR}/libs/PNGdec/src)
include_directories(${CMAKE_SOURCE_DIR}/libs/libhelix-mp3)

add_subdirectory("libs/sd-card/src")
add_subdirectory("libs/PNGdec")
add_subdirectory("libs/libhelix-mp3")

add_subdirectory("avr/rckid")
add_subdirectory("avr/rckid-test")
add_subdirectory("librckid")
add_subdirectory("cartridges/test/audio-test")
add_subdirectory("cartridges/test/sandbox")
add_subdirectory("cartridges/games")
add_subdirectory("cartridges/gbcemu")
