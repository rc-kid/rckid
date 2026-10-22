# Toolchain file for bare-metal ARM Cortex-M builds with the GNU Arm
# Embedded Toolchain (arm-none-eabi-gcc). Requires it to be on PATH.
#
# Usage:
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=toolchain-arm-none-eabi.cmake
#   cmake --build build

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER   arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

set(CMAKE_OBJCOPY arm-none-eabi-objcopy CACHE FILEPATH "objcopy tool")
set(CMAKE_OBJDUMP arm-none-eabi-objdump CACHE FILEPATH "objdump tool")
set(CMAKE_SIZE    arm-none-eabi-size    CACHE FILEPATH "size tool")

# We're cross-compiling to bare metal: there's no OS to run a test binary on,
# so CMake's compiler sanity check must build a static lib instead of an exe.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Don't let CMake search the host system for libraries/headers/programs -
# only look inside the toolchain's own sysroot for libs/includes.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
