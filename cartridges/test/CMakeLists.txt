
cmake_minimum_required(VERSION 3.12)


# set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
)

file(GLOB_RECURSE SRC "./*.cpp" "./*.h")

add_executable(rckid-test ${SRC})

#pico_generate_pio_header(picoboy ${CMAKE_SOURCE_DIR}/src/graphics/ili9341.pio)
#pico_generate_pio_header(picoboy ${CMAKE_SOURCE_DIR}/src/audio/i2s.pio)

# pull in common dependencies
target_link_libraries(rckid-test
    pico_stdlib
    hardware_i2c
    hardware_pio
    hardware_spi
    hardware_dma
)

# create map/bin/hex file etc.
pico_add_extra_outputs(rckid-test)

