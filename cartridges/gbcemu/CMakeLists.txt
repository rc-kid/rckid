
cmake_minimum_required(VERSION 3.12)

add_compile_options(
    -Wall
)

# The libgbcemu emulator itself
include_directories(${PICO_SDK_PATH}/include)
file(GLOB_RECURSE SRC_LIB "./lib/*.h" "./lib/*.cpp")
add_library(libgbcemu ${SRC_LIB})

# pull in common dependencies - this also adds include path
if (ARCH STREQUAL "RP2040")
    target_link_libraries(libgbcemu
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
endif()

# RCKid frontend to the emulator
set(TARGET gbcemu)

#file(GLOB SRC "./*.cpp" "./*.h")

add_executable(${TARGET} "gbcemu.cpp")

target_link_libraries(${TARGET} libgbcemu librckid)
## create map/bin/hex file etc.

if (ARCH STREQUAL "RP2040")
    pico_add_extra_outputs(${TARGET})
    pico_set_linker_script(${TARGET} ${CMAKE_SOURCE_DIR}/librckid/memmap.ld)
endif()

if (ARCH STREQUAL "MOCK")
    file(GLOB_RECURSE SRC_TESTS "tests/*.h" "tests/*.cpp")
    add_executable(gbcemu-test ${SRC_TESTS})
    target_link_libraries(gbcemu-test libgbcemu librckid)
endif()



