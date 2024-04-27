
cmake_minimum_required(VERSION 3.12)

set(TARGET display-test)

file(GLOB_RECURSE SRC "./*.cpp" "./*.h")
add_executable(${TARGET} ${SRC})

link_with_librckid(${TARGET})

