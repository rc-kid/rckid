cmake_minimum_required(VERSION 3.10)
project(utils-tests)

set (CMAKE_CXX_STANDARD 17)

add_definitions(-DTESTS)
file(GLOB_RECURSE SRC  *.cpp *.h)
add_executable(utils-tests ${SRC})
