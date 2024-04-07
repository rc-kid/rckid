cmake_minimum_required(VERSION 3.12)

project(blinky-test)

add_executable(blinky-test "blinky.cpp")

# link with stdlib for gpio & friends
target_link_libraries(blinky-test
   pico_stdlib
)

# create uf2
pico_add_extra_outputs(blinky-test)
