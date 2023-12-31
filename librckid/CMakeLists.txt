
cmake_minimum_required(VERSION 3.12)


# set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        -O2
)

file(GLOB_RECURSE SRC "rckid/*.cpp" "rckid/*.h")

add_library(librckid ${SRC})

pico_generate_pio_header(librckid ${CMAKE_SOURCE_DIR}/librckid/rckid/ST7789_rgb.pio)
pico_generate_pio_header(librckid ${CMAKE_SOURCE_DIR}/librckid/rckid/ST7789_rgb_double.pio)

pico_enable_stdio_uart(librckid ENABLED)



# pull in common dependencies
target_link_libraries(librckid
    #pico_stdlib
    pico_rand
    pico_unique_id
    hardware_i2c
    hardware_pio
    hardware_spi
    hardware_dma
    hardware_pwm
    FatFs_SPI
    tinyusb_board
    tinyusb_device
    PNGdec
    libhelix-mp3
)

# create map/bin/hex file etc.
# pico_add_extra_outputs(rckid-test)

