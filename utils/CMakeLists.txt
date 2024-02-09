cmake_minimum_required(VERSION 3.12)

# as those are commandline tools they are only provided in mock builds
if (ARCH STREQUAL "MOCK")
    add_executable(bytes-to-cpp "bytes-to-cpp.cpp")
    add_executable(int16-to-cpp "int16-to-cpp.cpp")
    add_executable(interp-tables "interp-tables.cpp")
endif()