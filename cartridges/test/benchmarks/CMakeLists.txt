cmake_minimum_required(VERSION 3.12)

set(TARGET benchmarks)

file(GLOB_RECURSE SRC "./*.cpp" "./*.h")
add_executable(${TARGET} ${SRC})
link_with_librckid(${TARGET} basic)

