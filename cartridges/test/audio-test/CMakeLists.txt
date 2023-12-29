
cmake_minimum_required(VERSION 3.12)

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
)


file(GLOB_RECURSE SRC "./*.cpp" "./*.h")

add_executable(audio-test ${SRC})

target_link_libraries(audio-test librckid)

# create map/bin/hex file etc.
pico_add_extra_outputs(audio-test)
pico_set_linker_script(audio-test ${CMAKE_SOURCE_DIR}/librckid/memmap.ld)
