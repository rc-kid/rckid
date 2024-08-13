
file(GLOB_RECURSE SRC "rckid_mk2.cpp" "${CMAKE_SOURCE_DIR}/sdk/rckid/*.h" "${CMAKE_SOURCE_DIR}/sdk/rckid/*.cpp")
add_library(librckid STATIC ${SRC})

pico_generate_pio_header(librckid ${CMAKE_CURRENT_SOURCE_DIR}/mic_pdm.pio)
pico_generate_pio_header(librckid ${CMAKE_CURRENT_SOURCE_DIR}/screen/ST7789_rgb.pio)
pico_generate_pio_header(librckid ${CMAKE_CURRENT_SOURCE_DIR}/screen/ST7789_rgb_double.pio)

# Sets given project to be used with RCKid SDK. 
#
# This must be called for libraries as well as executables (cartridges). 
function(link_with_librckid target_name)
    get_target_property(target_type ${target_name} TYPE)
    target_include_directories(${target_name} PRIVATE "${CMAKE_SOURCE_DIR}/libs/platform/rckid")
    target_include_directories(${target_name} PRIVATE "${CMAKE_SOURCE_DIR}/sdk/backends/arch_rckid_2/usb")
    if (target_type STREQUAL "EXECUTABLE")
        target_compile_options(${target_name} PRIVATE "-DPICO_XOSC_STARTUP_DELAY_MULTIPLIER=64")
        pico_add_extra_outputs(${target_name})
        pico_set_linker_script(${target_name} ${CMAKE_SOURCE_DIR}/sdk/backends/arch_rckid_2/memmap.ld)
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
        #FatFS
        tinyusb_board
        tinyusb_device
        #PNGdec
        #libhelix-mp3
    )
endfunction()

link_with_librckid(librckid) 

# swap rckid's malloc and free implementations for the default ones
pico_wrap_function(librckid malloc)
pico_wrap_function(librckid calloc)
pico_wrap_function(librckid realloc)
pico_wrap_function(librckid free)
