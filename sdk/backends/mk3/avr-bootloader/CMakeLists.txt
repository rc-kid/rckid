cmake_minimum_required(VERSION 3.10)
project(i2c-programmer)

set (CMAKE_CXX_STANDARD 17)

if(DEFINED ARCH_RPI)
else()
    add_definitions(-DARCH_MOCK)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/rckid)

if(DEFINED ARCH_RPI)
    message(STATUS "Building for raspberry pi")
    add_definitions(-DARCH_RPI)
    find_path(PIGPIO_INCLUDE_DIR 
            NAMES pigpio.h pigpiod_if.h pigpiod_if2.h
            HINTS /usr/local/include)        
    #find_library(PIGPIO_LIBRARY 
    #        NAMES libpigpio.so
    #        HINTS /usr/local/lib)
    #include_directories(${PIGPIO_INCLUDE_DIR})
else()
    message(STATUS "Mockup build")
    add_definitions(-DARCH_MOCK)
endif()



add_library(libi2cprogrammer "src/programmer.cpp")

file(GLOB_RECURSE SRC  src/i2c-programmer.cpp)
add_executable(i2c-programmer ${SRC})
target_link_libraries(i2c-programmer libi2cprogrammer)

if(DEFINED ARCH_RPI)
    #target_link_libraries(i2c-programmer ${PIGPIO_LIBRARY})
    target_link_libraries(i2c-programmer wiringPi)
endif()
