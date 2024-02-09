
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
rckid_configure_target(librckid)

if (ARCH STREQUAL "RP2040")
    pico_generate_pio_header(librckid ${CMAKE_SOURCE_DIR}/librckid/rckid/drivers/ST7789_rgb.pio)
    pico_generate_pio_header(librckid ${CMAKE_SOURCE_DIR}/librckid/rckid/drivers/ST7789_rgb_double.pio)
endif()



