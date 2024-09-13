
cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SRC_LIB "./lib/*.h" "./lib/*.cpp")
add_library(libgbcemu ${SRC_LIB})
link_with_librckid(libgbcemu)

add_executable(gbcemu "gbcemu.cpp")
target_link_libraries(gbcemu PRIVATE libgbcemu)
link_with_librckid(gbcemu)

#add_compile_options(
#    -Wall
#)

# The libgbcemu emulator itself
#include_directories(${PICO_SDK_PATH}/include)
#file(GLOB_RECURSE SRC_LIB "./lib/*.h" "./lib/*.cpp")
#add_library(libgbcemu ${SRC_LIB})
#rckid_configure_target(libgbcemu)

# RCKid frontend to the emulator
#set(TARGET gbcemu)
#add_executable(${TARGET} "gbcemu.cpp")
#target_link_libraries(${TARGET} libgbcemu)
#rckid_configure_target(${TARGET})

#if (ARCH STREQUAL "MOCK")
#    file(GLOB_RECURSE SRC_TESTS "tests/*.h" "tests/*.cpp")
#    add_executable(gbcemu-test ${SRC_TESTS})
#    target_link_libraries(gbcemu-test libgbcemu)
#    rckid_configure_target(gbcemu-test)
#endif()



