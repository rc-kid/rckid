# RCKid Library
#
# A superset of the pico sdk that supports all the basic HW and SW of the handheld. The idea is for the library to be linked with the cartridge executables. 

cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SRC "rckid/*.cpp" "rckid/*.h" "rckid/*.s" "usb.cpp" "apps/*.h" "apps/*.cpp")
add_library(librckid ${SRC})

target_compile_options(librckid PRIVATE -Wall -O2)

if (ARCH STREQUAL "RP2040")
pico_generate_pio_header(librckid ${CMAKE_CURRENT_SOURCE_DIR}/rckid/mic_clk.pio)
pico_generate_pio_header(librckid ${CMAKE_CURRENT_SOURCE_DIR}/rckid/ST7789_rgb.pio)
pico_generate_pio_header(librckid ${CMAKE_CURRENT_SOURCE_DIR}/rckid/ST7789_rgb_double.pio)
endif()

function(link_with_librckid target_name)
    get_target_property(target_type ${target_name} TYPE)
    if (ARCH STREQUAL "RP2040")
        
        if (target_type STREQUAL "EXECUTABLE")
            message("RCKID: Target executable ${target_name} will be linked with librckid")
            target_compile_options(${target_name} PRIVATE "-DPICO_XOSC_STARTUP_DELAY_MULTIPLIER=64")
            pico_add_extra_outputs(${target_name})
            pico_set_linker_script(${target_name} ${CMAKE_SOURCE_DIR}/librckid/memmap.ld)
            target_link_libraries(${target_name} librckid)
            #set_property(TARGET ${target_name} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
        endif()

        target_link_libraries(${target_name}
            pico_stdlib
            pico_rand
            pico_multicore
            pico_unique_id
            hardware_i2c
            hardware_pio
            hardware_spi
            hardware_dma
            hardware_pwm
            FatFS
            tinyusb_board
            tinyusb_device
            PNGdec
            libhelix-mp3
        )

    elseif (ARCH STREQUAL "MOCK")

        if (target_type STREQUAL "EXECUTABLE")
            message("RCKID: Target executable ${target_name} will be linked with librckid")
            target_link_libraries(${target_name} librckid)
        endif()

        target_link_libraries(${target_name}
            PNGdec
            libhelix-mp3
        )

        target_link_libraries(${target_name} ${CMAKE_SOURCE_DIR}/libs/raylib/src/libraylib.a)
        target_link_libraries(${target_name} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})

    endif()
endfunction()

link_with_librckid(librckid)


if (ARCH STREQUAL "RP2040")
    pico_wrap_function(librckid malloc)
    pico_wrap_function(librckid calloc)
    pico_wrap_function(librckid realloc)
    pico_wrap_function(librckid free)
endif()

target_include_directories(librckid PUBLIC ${CMAKE_SOURCE_DIR}/librckid)



