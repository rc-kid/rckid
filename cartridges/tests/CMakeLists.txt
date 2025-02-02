# Set general compiler options, C++ standard and warning levels
set(CMAKE_CXX_STANDARD 17)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# the simplest program that simply cycles through red, green and blue full color on the display as fast as possible (note that in fantasy console this actually means at the preset 60 fps)
add_executable(display-fill "display_fill.cpp")
link_with_librckid(display-fill)

# test of the graphics app framework (RGB colors) 
add_executable(graphics-appRGB "graphics_appRGB.cpp")
link_with_librckid(graphics-appRGB)

