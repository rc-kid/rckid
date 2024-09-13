cmake_minimum_required(VERSION 3.12)

if (NOT DEFINED ARCH)
    message("RCKID: No architecture specified, defaulting to ARCH_RCKID_3")
    #set(ARCH "ARCH_RCKID_3")
    set(ARCH "ARCH_FANTASY")
endif()

if (ARCH STREQUAL "ARCH_FANTASY")
    message("RCKID: Building for the fantasy console (with Raylib)")
    project(rckid)
else()
    message("RCKID: Building for the actual hardware - RP2350 or RP2040")
    # skip the pico malloc library setup
    set(SKIP_PICO_MALLOC 1)
    # Pull in SDK (must be before project)
    include(${CMAKE_SOURCE_DIR}/sdk/backends/arch_rckid/pico-sdk/pico_sdk_init.cmake)
    project(rckid C CXX ASM)
    pico_sdk_init()
    if (PICO_SDK_VERSION_STRING VERSION_LESS "2.0.0")
        message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.0.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
    endif()
endif()

set(CMAKE_CXX_STANDARD 17)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

add_compile_definitions(${ARCH})
include_directories("sdk")
include_directories("libs")

add_subdirectory("libs/PNGdec")
add_subdirectory("libs/FatFS")

string(TOLOWER "${ARCH}" ARCH_LOWER)
add_subdirectory("sdk/backends/${ARCH_LOWER}")

add_subdirectory("test")

add_subdirectory("cartridges/hello-world")
add_subdirectory("cartridges/test")
add_subdirectory("cartridges/gbcemu")


# add utils target if we are building for the fantasy architecture (needs raylib)
if(${ARCH} STREQUAL "ARCH_FANTASY")
    add_subdirectory("sdk/utils")
endif()

# doxygen documentation
add_custom_target(docs
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND echo "Generating doxygen documentation..."
    COMMAND doxygen Doxyfile
)
