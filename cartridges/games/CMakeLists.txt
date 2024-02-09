
cmake_minimum_required(VERSION 3.12)

set(TARGET games)

add_compile_options(
    -Wall
)

file(GLOB_RECURSE SRC "./*.cpp" "./*.h")

add_executable(${TARGET} ${SRC})

rckid_configure_target(${TARGET})

# create map/bin/hex file etc.
#if (ARCH STREQUAL "RP2040")
#    pico_add_extra_outputs(${TARGET})
#    pico_set_linker_script(${TARGET} ${CMAKE_SOURCE_DIR}/librckid/memmap.ld)
#elseif (ARCH STREQUAL "MOCK")
#    target_link_libraries(${TARGET} ${CMAKE_SOURCE_DIR}/libs/raylib/src/libraylib.a)
#endif()