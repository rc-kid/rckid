cmake_minimum_required(VERSION 3.12)

if (ARCH STREQUAL "RP2040")
    set(TARGET sd-blocking)

    add_subdirectory("${CMAKE_SOURCE_DIR}/libs/sd-card/src" "sd-card")

    file(GLOB_RECURSE SRC "./*.cpp" "./*.h")
    add_executable(${TARGET} ${SRC})
    rckid_configure_target(${TARGET})

    target_link_libraries(${TARGET} FatFs_SPI)
endif()

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