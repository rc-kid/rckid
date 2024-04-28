# RCKid mk II
cmake_minimum_required(VERSION 3.12)

if (NOT DEFINED ARCH)
    set(ARCH "RP2040")
endif()

if (ARCH STREQUAL "RP2040")
    message("RCKID: Building for the RP2040 (RCKid)")
    # skip the pico malloc library setup
    set(SKIP_PICO_MALLOC 1)
    # Pull in SDK (must be before project)
    include(${CMAKE_SOURCE_DIR}/libs/pico-sdk/pico_sdk_init.cmake)
    project(rckid2 C CXX ASM)
    pico_sdk_init()
    if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
        message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
    endif()

    set(PLATFORM_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include/platform/rp2040")

elseif (ARCH STREQUAL "MOCK")
    message("RCKID: Building for the Desktop (Raylib & X11)")
    include_directories(${CMAKE_SOURCE_DIR}/libs/raylib/src)
    #add_compile_definitions("LIBRCKID_MOCK")

    project(rckid2)
    find_package(Threads REQUIRED) 

    set(PLATFORM_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include/platform/mock")
else()
    message(FATAL_ERROR "Invalid architecture specified")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)


include_directories(${PLATFORM_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include/platform/peripherals)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/assets)
include_directories(${CMAKE_SOURCE_DIR}/librckid)
include_directories(${CMAKE_SOURCE_DIR}/libs/sd-card)
include_directories(${CMAKE_SOURCE_DIR}/libs/PNGdec/src)
include_directories(${CMAKE_SOURCE_DIR}/libs/libhelix-mp3)

add_subdirectory("utils")
add_subdirectory("librckid")
add_subdirectory("libs/PNGdec")
add_subdirectory("libs/libhelix-mp3")
#add_subdirectory("cartridges/gbcemu")


add_subdirectory("cartridges/test/simple")
#add_subdirectory("cartridges/games")
#add_subdirectory("cartridges/test/sandbox")
#add_subdirectory("cartridges/test/hw-test")
#add_subdirectory("cartridges/test/audio-test")
#add_subdirectory("cartridges/test/benchmarks")
#add_subdirectory("cartridges/test/sd-blocking")

if (ARCH STREQUAL "RP2040")
    add_subdirectory("avr/rckid")
    add_subdirectory("avr/rckid-test")
endif()


# Initialize the SDK
#if (ARCH STREQUAL "RP2040")
#    # Pull in SDK (must be before project)
#    include(${CMAKE_SOURCE_DIR}/libs/pico-sdk/pico_sdk_init.cmake)
#    project(rckid2 C CXX ASM)
#    pico_sdk_init()
#    if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
#        message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
#    endif()
#else()
#    project(rckid2)
#    find_package(Threads REQUIRED) 
#endif()



#if (ARCH STREQUAL "RP2040")
#    
#    include_directories(${CMAKE_SOURCE_DIR}/include/platform/rp2040)
#elseif(ARCH STREQUAL "MOCK")
#    include_directories(${CMAKE_SOURCE_DIR}/include/platform/mock)
#endif()








#file(GLOB_RECURSE LIBRCKID_SRC "librckid/rckid/*.cpp" "librckid/rckid/*.h")


# configures the given target as RCKid project. Adds all required libraries and in case of a cartridge image also sets the VRAM size. The default size is 77760, which is enough for 320x240x8bpp + two rows of 16bpp (240 * 2 * 2)  
function(deprecate_rckid_configure_target target_name) 
    #set(rckid_vram_size 77760)
    #set(rckid_vram_size 10240)
    set(rckid_vram_size 153600)
    if (${ARGV1})
        set(rckid_vram_size ${ARGV1})
    endif()
    message("VRAM size for project ${target_name}: ${rckid_vram_size}")
    #if (NOT ${target_name} STREQUAL "librckid")
    #    target_link_libraries(${target_name} librckid)
    #endif()
    get_target_property(target_type ${target_name} TYPE)
    if (target_type STREQUAL "EXECUTABLE")
        target_sources(${target_name} PRIVATE ${LIBRCKID_SRC} "${CMAKE_SOURCE_DIR}/librckid/rckid/graphics/bitmap.s")
        target_compile_options(${target_name} PRIVATE "-DRCKID_VRAM_SIZE=${rckid_vram_size}")
        target_compile_options(${target_name} PRIVATE "-DPICO_XOSC_STARTUP_DELAY_MULTIPLIER=64")
    endif()
if (ARCH STREQUAL "RP2040")
    target_compile_options(${target_name} PRIVATE -Wall -O2)
    pico_generate_pio_header(${target_name} ${CMAKE_SOURCE_DIR}/librckid/rckid/ST7789_rgb.pio)
    pico_generate_pio_header(${target_name} ${CMAKE_SOURCE_DIR}/librckid/rckid/ST7789_rgb_double.pio)

    if (target_type STREQUAL "EXECUTABLE")
        pico_add_extra_outputs(${target_name})
        pico_set_linker_script(${target_name} ${CMAKE_SOURCE_DIR}/librckid/memmap.ld)
    endif()
    target_link_libraries(${target_name}
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
elseif (ARCH STREQUAL "MOCK")
    target_link_libraries(${target_name}
        PNGdec
        libhelix-mp3
    )
    target_link_libraries(${target_name} ${CMAKE_SOURCE_DIR}/libs/raylib/src/libraylib.a)
    target_link_libraries(${target_name} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif()
endfunction()
