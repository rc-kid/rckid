
cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SRC_LIB "gbcemu/*.h" "gbcemu/*.cpp")
add_library(libgbcemu ${SRC_LIB})
link_with_librckid(libgbcemu)
