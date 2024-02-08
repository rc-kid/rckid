# RCKid mk II
cmake_minimum_required(VERSION 3.12)

if (NOT DEFINED ARCH)
    set(ARCH "RP2040")
endif()
if (ARCH STREQUAL "RP2040")
    message(INFO "Building for the RP2040 (RCKid)")
elseif (ARCH STREQUAL "MOCK")
    message(INFO "Building for the Desktop (Raylib & X11)")
    include_directories(${CMAKE_SOURCE_DIR}/libs/raylib/src)
else()
    message(FATAL_ERROR "Invalid architecture specified")
endif()

# Initialize the SDK
if (ARCH STREQUAL "RP2040")
    message("Building for RCKId hardware (RP2040)")
    # Pull in SDK (must be before project)
    include(${CMAKE_SOURCE_DIR}/libs/pico-sdk/pico_sdk_init.cmake)
    project(rckid2 C CXX ASM)
    pico_sdk_init()
    if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
        message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
    endif()
else()
    message("Building for local hw (mock)")
    project(rckid2)
    add_compile_definitions("LIBRCKID_MOCK")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/assets)
include_directories(${CMAKE_SOURCE_DIR}/librckid)
include_directories(${CMAKE_SOURCE_DIR}/libs/sd-card)
include_directories(${CMAKE_SOURCE_DIR}/libs/PNGdec/src)
include_directories(${CMAKE_SOURCE_DIR}/libs/libhelix-mp3)

add_subdirectory("librckid")
#add_subdirectory("libs/sd-card/src")
add_subdirectory("libs/PNGdec")
add_subdirectory("libs/libhelix-mp3")
add_subdirectory("cartridges/gbcemu")

add_subdirectory("cartridges/games")
add_subdirectory("cartridges/test/sandbox")


if (ARCH STREQUAL "RP2040")
    add_subdirectory("avr/rckid")
    add_subdirectory("avr/rckid-test")
    add_subdirectory("cartridges/test/audio-test")
endif()

