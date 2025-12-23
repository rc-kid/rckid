# Set general compiler options, C++ standard and warning levels
set(CMAKE_CXX_STANDARD 17)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# the default rc-kid cartridge with all latest apps and bells & whistles
add_executable(rckid-default "rckid-default.cpp")
target_link_libraries(rckid-default PRIVATE libgbcemu)
link_with_librckid(rckid-default)

# like default, but with parent mode forced on
add_executable(rckid-parent "rckid-parent.cpp")
target_link_libraries(rckid-parent PRIVATE libgbcemu)
link_with_librckid(rckid-parent)

# simple COM port logging verification
add_executable(logging "tests/logging.cpp")
link_with_librckid(logging)

# simple COM port logging verification
add_executable(hwtest "tests/hwtest.cpp")
link_with_librckid(hwtest)

# audio sine wave and audio codec test
add_executable(audiotest "tests/audiotest.cpp")
link_with_librckid(audiotest)

# blue screen of death working test
add_executable(bsodtest "tests/bsodtest.cpp")
link_with_librckid(bsodtest)

# the simplest program that simply cycles through red, green and blue full color on the display as fast as possible (note that in fantasy console this actually means at the preset 60 fps)
add_executable(display-fill "tests/display_fill.cpp")
link_with_librckid(display-fill)

# very basic test of the graphics app rendering (cycles through background colors and displays text).  
add_executable(graphics-appRGB "tests/graphics_appRGB.cpp")
link_with_librckid(graphics-appRGB)

