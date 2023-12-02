
cmake_minimum_required(VERSION 3.12)


# set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
)

file(GLOB_RECURSE SRC "rckid/./*.cpp" "rckid/*.h")

include_directories(${CMAKE_SOURCE_DIR/libs/sd-card/})

add_library(librckid ${SRC})

pico_generate_pio_header(librckid ${CMAKE_SOURCE_DIR}/librckid/rckid/gpu/st7789.pio)

pico_enable_stdio_uart(librckid ENABLED)



# pull in common dependencies
target_link_libraries(librckid
    pico_stdlib
    hardware_i2c
    hardware_pio
    hardware_spi
    hardware_dma
    hardware_pwm
    FatFs_SPI
    tinyusb_device
)

# create map/bin/hex file etc.
# pico_add_extra_outputs(rckid-test)

