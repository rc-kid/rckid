cmake_minimum_required(VERSION 3.12)

project(rckid)

set(CMAKE_CXX_STANDARD 17)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT DEFINED ARCH)
    message("RCKID: No architecture specified, defaulting to ARCH_RCKID_3")
    #set(ARCH "ARCH_RCKID_3")
    set(ARCH "ARCH_FANTASY")
endif()

add_compile_definitions(${ARCH})
include_directories("sdk")
include_directories("libs")

string(TOLOWER "${ARCH}" ARCH_LOWER)
add_subdirectory("sdk/backends/${ARCH_LOWER}")

add_subdirectory("test")

add_subdirectory("cartridges/hello-world")
