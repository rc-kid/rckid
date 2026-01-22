cmake_minimum_required(VERSION 3.12)

# Set general compiler options, C++ standard and warning levels
set(CMAKE_CXX_STANDARD 17)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# RCKid backend selection
#
# This is done via the RCKID_BACKEND flag. When not specified, FANTASY is selected for fantasy backend. The following options are supported:
#
# -DRCKID_BACKEND=FANTASY for the fantasy console using raylib for graphics and sound, should run at least on on x86 and Aarch64, Linux and Windows
# -DRCKID_BACKEND=MK3 for the mkIII RCKid device, powered by RP2350
if (NOT DEFINED RCKID_BACKEND)
    message("RCKID: No backend specified, defaulting to RCKID_BACKEND_FANTASY")
    set(RCKID_BACKEND "FANTASY")
endif()

# Based on the backend selected, pull in necessary libraries and setup the cmake environment. For fantasy backend, we need to initialize raylib, while the hardware backends pull-in and initialize the Raspberry Pi Pico SDK
if (RCKID_BACKEND STREQUAL "MK3")
    # if backend is mk3 ensure tell the pico SDK to use RP2350 instead of RP2040
    set(PICO_BOARD none)
    set(PICO_PLATFORM rp2350)
    message("RCKID: Building for the mk3 (RP2350)")

    add_definitions(-DCYW43_DEFAULT_PIN_WL_DATA_OUT=15)
    add_definitions(-DCYW43_DEFAULT_PIN_WL_DATA_IN=15)
    add_definitions(-DCYW43_DEFAULT_PIN_WL_HOST_WAKE=15)
    add_definitions(-DCYW43_DEFAULT_PIN_WL_CLOCK=16)
    add_definitions(-DCYW43_DEFAULT_PIN_WL_CS=14)
    add_definitions(-DCYW43_DEFAULT_PIN_WL_REG_ON=13)
    #add_definitions(-DCYW43_DEFAULT_PIN_WL_REG_ON=13) # this is for BT
    add_definitions(-DCYW43_PIN_WL_DYNAMIC=0)
    add_definitions(-DPICO_CYW43_SUPPORTED=1)
    set(PICO_CYW43_SUPPORTED true)

    # skip the pico malloc library setup
    set(SKIP_PICO_MALLOC 1)
    # Pull in SDK (must be before project)
    include(${CMAKE_SOURCE_DIR}/lib/pico-sdk/pico_sdk_init.cmake)
    project(rckid C CXX ASM)
    # Tell pico SDK we are using 16MB cartridges
    add_compile_definitions(PICO_FLASH_SIZE_BYTES=16777216)
    # initialize the Pico SDK
    pico_sdk_init()
    if (PICO_SDK_VERSION_STRING VERSION_LESS "2.0.0")
        message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.0.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
    endif()
elseif (RCKID_BACKEND STREQUAL "FANTASY")
    message("RCKID: Building for the fantasy console (with Raylib)")
    project(rckid)
    add_definitions(-DSUPPORT_CUSTOM_FRAME_CONTROL)
    add_subdirectory(lib/raylib)
else()
    message(FATAL_ERROR "Unknown RCKid backend selected: ${RCKID_BACKEND}. Supported backends: FANTASY, MK3")
endif()

add_compile_options(-Wall -Wextra)

# adds the specific backend definition so that the code knows as well which backend we are using
add_compile_definitions("RCKID_BACKEND_${RCKID_BACKEND}")

add_compile_options(-fmacro-prefix-map=/home/peta/devel/rckid/=/)

# setup general include directories
include_directories("sdk2/include")
include_directories("lib")
include_directories("gbcemu")

# Add 3rd party libraries (note that these are part of the repo as they often required changes to be made)
add_subdirectory("lib/PNGdec")
add_subdirectory("lib/PNGenc")
add_subdirectory("lib/FatFS")
add_subdirectory("lib/littlefs")
add_subdirectory("lib/libhelix-mp3")
add_subdirectory("lib/libopus")

# add the SDK itself
add_subdirectory("sdk2")

# add tests for various projects
#add_subdirectory("tests")


#add_subdirectory("gbcemu")

add_subdirectory("cartridges2")
#add_subdirectory("cartridges/hello-world")
#add_subdirectory("cartridges/gbcemu")

# doxygen documentation
add_custom_target(docs
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND echo "Generating doxygen documentation..."
    COMMAND doxygen Doxyfile
)
