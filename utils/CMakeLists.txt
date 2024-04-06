cmake_minimum_required(VERSION 3.12)

# as those are commandline tools they are only provided in mock builds
if (ARCH STREQUAL "MOCK")
    add_executable(bytes-to-cpp "bytes-to-cpp.cpp")
    add_executable(int16-to-cpp "int16-to-cpp.cpp")
    add_executable(interp-tables "interp-tables.cpp")


    add_custom_target(assets 
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS bytes-to-cpp 
        COMMAND ${CMAKE_BINARY_DIR}/utils/bytes-to-cpp assets/icons/gameboy.png > assets/icons/gameboy.inc
    )

endif()


