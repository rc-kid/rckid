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
    add_compile_definitions("LIBRCKID_MOCK")
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
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

function(rckid_configure_target target_name) 
    if (NOT ${target_name} STREQUAL "librckid")
        target_link_libraries(${target_name} librckid)
    endif()
    target_compile_options(${target_name} PRIVATE -Wall)
if (ARCH STREQUAL "RP2040")
    get_target_property(target_type ${target_name} TYPE)
    if (target_type STREQUAL "EXECUTABLE")
        pico_add_extra_outputs(${target_name})
        pico_set_linker_script(${target_name} ${CMAKE_SOURCE_DIR}/librckid/memmap.ld)
    endif()
    target_link_libraries(${target_name}
        pico_stdlib
        pico_rand
        pico_unique_id
        hardware_i2c
        hardware_pio
        hardware_spi
        hardware_dma
        hardware_pwm
        #FatFs_SPI
        tinyusb_board
        tinyusb_device
        PNGdec
        libhelix-mp3
    )
elseif (ARCH STREQUAL "MOCK")
    target_link_libraries(${target_name}
        PNGdec
        libhelix-mp3
    )
    target_link_libraries(${target_name} ${CMAKE_SOURCE_DIR}/libs/raylib/src/libraylib.a)
endif()
endfunction()

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/assets)
include_directories(${CMAKE_SOURCE_DIR}/librckid)
include_directories(${CMAKE_SOURCE_DIR}/libs/sd-card)
include_directories(${CMAKE_SOURCE_DIR}/libs/PNGdec/src)
include_directories(${CMAKE_SOURCE_DIR}/libs/libhelix-mp3)

add_subdirectory("utils")
add_subdirectory("librckid")
#add_subdirectory("libs/sd-card/src")
add_subdirectory("libs/PNGdec")
add_subdirectory("libs/libhelix-mp3")
add_subdirectory("cartridges/gbcemu")

add_subdirectory("cartridges/games")
add_subdirectory("cartridges/test/sandbox")
add_subdirectory("cartridges/test/hw-test")
add_subdirectory("cartridges/test/audio-test")


if (ARCH STREQUAL "RP2040")
    add_subdirectory("avr/rckid")
    add_subdirectory("avr/rckid-test")
endif()

