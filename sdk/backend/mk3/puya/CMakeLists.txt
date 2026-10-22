cmake_minimum_required(VERSION 3.20)

# Standalone project for the PY32F030 (cortex-m0+). The mk3 backend builds it via ExternalProject_Add so that it gets
# its own toolchain instead of inheriting the RP2350 (pico SDK) one. It can also be configured on its own.
set(PLATFORM_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/platform/py32f030)
cmake_path(NORMAL_PATH PLATFORM_ROOT)

# must be set *before* project - configure for arm-none-eabi for crosscompiling
set(CMAKE_TOOLCHAIN_FILE ${PLATFORM_ROOT}/CMSIS/toolchain-arm-none-eabi.cmake
    CACHE FILEPATH "toolchain file")

project(mk4_puya LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CPU flags (shared by the compiler and the linker)
set(MCU_FLAGS
    -mcpu=cortex-m0plus
    -mthumb
    -mfloat-abi=soft
)

set(LINKER_SCRIPT ${PLATFORM_ROOT}/CMSIS/py32f030.ld)

file(GLOB LL_DRIVER_SOURCES
    "${PLATFORM_ROOT}/LL/Source/*.c"
)
add_executable(${PROJECT_NAME}
    ${PLATFORM_ROOT}/CMSIS/startup_py32f030.S
    ${PLATFORM_ROOT}/CMSIS/PY32F0xx/Source/system_py32f0xx.c
    ${LL_DRIVER_SOURCES}
    puya-mk4.cpp
    # add your exactutable here
)

# important for Puya's CMSIS libraries so that they know what exact chip to use
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PY32F030x8
    USE_FULL_LL_DRIVER
)

# CMSIS include folders
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PLATFORM_ROOT}/CMSIS/PY32F0xx/Include
    ${PLATFORM_ROOT}/CMSIS/Core/Include
    ${PLATFORM_ROOT}/LL/Include
    ${PLATFORM_ROOT}
    ${PLATFORM_ROOT}/..
)

target_compile_options(${PROJECT_NAME} PRIVATE
    ${MCU_FLAGS}
    #-ffreestanding ## we want std::function & friends
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    # C++-only flags: no runtime type info / exception unwinder on bare metal
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti -fno-threadsafe-statics>
    $<$<CONFIG:Debug>:-Og -g3>
    $<$<CONFIG:Release>:-O2>
)

target_link_options(${PROJECT_NAME} PRIVATE
    ${MCU_FLAGS}
    -T${LINKER_SCRIPT}
    -nostartfiles                # startup_py32f030.S provides Reset_Handler/vectors instead
    -specs=nano.specs            # newlib-nano: smaller libc for embedded targets
    -specs=nosys.specs           # stub out syscalls (_write, _sbrk, ...) - no OS
    -Wl,--gc-sections
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--print-memory-usage
)

# Re-link automatically if the linker script changes.
set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_DEPENDS ${LINKER_SCRIPT}
    SUFFIX ".elf"
)

# Produce .hex/.bin alongside the .elf and print a size summary, same as a
# typical Keil/CubeIDE post-build step.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    BYPRODUCTS
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMENT "Generating ${PROJECT_NAME}.hex / .bin"
)



