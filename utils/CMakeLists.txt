cmake_minimum_required(VERSION 3.12)

project(rckid-utils)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/../libs/raylib/src)

add_executable(bytes-to-cpp "bytes-to-cpp.cpp")
add_executable(int16-to-cpp "int16-to-cpp.cpp")
add_executable(generate-interp-tables "generate-interp-tables.cpp")

add_executable(generate-rgb-332-to-565 "generate-rgb-332-to-565.cpp")
add_executable(generate-assets "generate-assets.cpp")
add_executable(generate-font "generate-font.cpp")
target_link_libraries(generate-font "${CMAKE_SOURCE_DIR}/../libs/raylib/src/libraylib.a")

#target_link_libraries(generate-assets PRIVATE stdc++fs)

# as those are commandline tools they are only provided in mock builds
#if (ARCH STREQUAL "MOCK")
#    add_executable(bytes-to-cpp "bytes-to-cpp.cpp")
#    add_executable(int16-to-cpp "int16-to-cpp.cpp")
#    add_executable(interp-tables "interp-tables.cpp")
#    add_executable(generate-assets "generate-assets.cpp")
#
#
#    add_custom_target(assets 
#        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#        DEPENDS bytes-to-cpp 
#        COMMAND ${CMAKE_BINARY_DIR}/utils/bytes-to-cpp assets/icons/gameboy.png > assets/icons/#gameboy.inc
#    )
#
#endif()



