
cmake_minimum_required(VERSION 3.12)

set(TARGET sandbox)

# set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

#add_compile_options(-Wall
#        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
#        -Wno-unused-function # we have some for the docs that aren't called
#        -Wno-maybe-uninitialized
#)


file(GLOB_RECURSE SRC "./*.cpp" "./*.h")
add_executable(${TARGET} ${SRC})
link_with_librckid(${TARGET})

#pico_generate_pio_header(rckid-test ${CMAKE_SOURCE_DIR}/librckid/rckid/st7789.pio)

#pico_enable_stdio_uart(rckid-test ENABLED)

# pull in common dependencies
#target_link_libraries(${TARGET} librckid)
#    pico_stdlib
#    hardware_i2c
#    hardware_pio
#    hardware_spi
#    hardware_dma
#)


# create map/bin/hex file etc.
#if (ARCH STREQUAL "RP2040")
#    pico_add_extra_outputs(${TARGET})
#    pico_set_linker_script(${TARGET} ${CMAKE_SOURCE_DIR}/librckid/memmap.ld)
#elseif (ARCH STREQUAL "MOCK")
#    target_link_libraries(${TARGET} ${CMAKE_SOURCE_DIR}/libs/raylib/src/libraylib.a)
#endif()