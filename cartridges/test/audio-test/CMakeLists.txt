
cmake_minimum_required(VERSION 3.12)

set(TARGET audio-test)

file(GLOB_RECURSE SRC "./*.cpp" "./*.h")
add_executable(${TARGET} ${SRC})
rckid_configure_target(${TARGET})
