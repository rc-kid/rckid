# Set general compiler options, C++ standard and warning levels
set(CMAKE_CXX_STANDARD 17)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

message("RCKID: mk3 backend selected (RP2350)")

file(GLOB_RECURSE SRC "mk3.cpp" "radio.cpp" "wifi.cpp" "sd/*.cpp" "usb/*.cpp" "screen/*.cpp" "${CMAKE_SOURCE_DIR}/sdk/rckid/*.h" "${CMAKE_SOURCE_DIR}/sdk/rckid/*.cpp")
add_library(librckid STATIC ${SRC})

pico_generate_pio_header(librckid ${CMAKE_CURRENT_SOURCE_DIR}/screen/ST7789_rgb16.pio)
pico_generate_pio_header(librckid ${CMAKE_CURRENT_SOURCE_DIR}/audio/i2s_out16.pio)
pico_generate_pio_header(librckid ${CMAKE_CURRENT_SOURCE_DIR}/audio/i2s_in16.pio)
pico_generate_pio_header(librckid ${CMAKE_CURRENT_SOURCE_DIR}/audio/i2s_mclk.pio)
pico_generate_pio_header(librckid ${CMAKE_CURRENT_SOURCE_DIR}/sd/sd_spi.pio)

# Sets given project to be used with RCKid SDK. 
#
# This must be called for libraries as well as executables (cartridges). 
function(link_with_librckid target_name)
    get_target_property(target_type ${target_name} TYPE)
    target_compile_options(${target_name} PRIVATE "-DPICO_XOSC_STARTUP_DELAY_MULTIPLIER=128")
    target_include_directories(${target_name} PRIVATE "${CMAKE_SOURCE_DIR}/libs/platform/rp2350")
    target_include_directories(${target_name} PRIVATE "${CMAKE_SOURCE_DIR}/sdk/backends/mk3")
    # required for tiny usb library to find the tusb_config.h file
    target_include_directories(${target_name} PRIVATE "${CMAKE_SOURCE_DIR}/sdk/backends/mk3/usb")
    if (target_type STREQUAL "EXECUTABLE")
        pico_add_extra_outputs(${target_name})
        pico_set_linker_script(${target_name} ${CMAKE_SOURCE_DIR}/sdk/backends/mk3/memmap.ld)
        target_link_libraries(${target_name} PRIVATE librckid)
        #set_property(TARGET ${target_name} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()

    target_link_libraries(${target_name} PRIVATE
        pico_stdlib
        pico_unique_id
        #pico_cyw43_arch_poll
        pico_cyw43_arch_lwip_poll
        pico_lwip_http
        pico_lwip_mbedtls
        pico_mbedtls
        tinyusb_device
        tinyusb_board
        pico_rand
        pico_multicore
        hardware_i2c
        hardware_pio
        hardware_spi
        hardware_dma
        hardware_pwm
        hardware_sync
        hardware_flash
        FatFS
        littlefs
        PNGdec
        PNGenc
        libhelix-mp3
        opus
    )
endfunction()

link_with_librckid(librckid) 

# swap rckid's malloc and free implementations for the default ones
pico_wrap_function(librckid malloc)
pico_wrap_function(librckid calloc)
pico_wrap_function(librckid realloc)
pico_wrap_function(librckid free)
