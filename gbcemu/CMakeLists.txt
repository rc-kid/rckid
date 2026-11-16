
cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SRC_LIB "gbcemu/*.h" "gbcemu/*.cpp")
add_library(libgbcemu ${SRC_LIB})
link_with_libkiddo(libgbcemu)

if (KIDDO_BACKEND STREQUAL "FANTASY")

    # Generates the ROMs
    add_custom_target(generate-gbcemu-roms
        COMMAND $<TARGET_FILE:binary-folder> roms/blargg-test-roms/cpu_instrs/individual tests/roms/blargg_cpu_instrs.h --raw-path raw --namespace kiddo::gbcemu::rom::blargg::instrs
        DEPENDS binary-folder
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

else()
    message("GBCEmu roms are only available in KIDDO_BACKEND=FANTASY, skipping")
endif()
