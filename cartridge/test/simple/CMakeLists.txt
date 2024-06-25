
cmake_minimum_required(VERSION 3.12)

# simply blinks an LED on GPIO 21. Does not use RCKid library, only the Pico SDK
if (ARCH STREQUAL "RP2040")
    add_executable(blinky-test "blinky.cpp")
    target_link_libraries(blinky-test
        pico_stdlib
    )
    pico_add_extra_outputs(blinky-test)
endif()

# the simplest RCKid library based test that connect to the display and fills it with different colors in a loop using primitive bitbanging
add_executable(display-test "display.cpp")
link_with_librckid(display-test)

# framebuffer tests - no app, primitive main loop that simply shows memory info and display update timings for various framebuffers
add_executable(fb-hello-rgb "fb-rgb.cpp" "fb-test.h")
link_with_librckid(fb-hello-rgb)

add_executable(fb-hello-256 "fb-256.cpp" "fb-test.h")
link_with_librckid(fb-hello-256)

add_executable(fb-hello-2x-rgb "fb-2x-rgb.cpp" "fb-test.h")
link_with_librckid(fb-hello-2x-rgb)

add_executable(fb-hello-2x-256 "fb-2x-256.cpp" "fb-test.h")
link_with_librckid(fb-hello-2x-256)

# simple application that displays the timings of its main loop, uses a framebuffer app
add_executable(simple-app "simple-app.cpp")
link_with_librckid(simple-app)

