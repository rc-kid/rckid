# Set general compiler options, C++ standard and warning levels
set(CMAKE_CXX_STANDARD 17)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# simple COM port logging verification
add_executable(logging "logging.cpp")
link_with_librckid(logging)

# simple COM port logging verification
add_executable(hwtest "hwtest.cpp")
link_with_librckid(hwtest)

# blue screen of death working test
add_executable(bsodtest "bsodtest.cpp")
link_with_librckid(bsodtest)

# the simplest program that simply cycles through red, green and blue full color on the display as fast as possible (note that in fantasy console this actually means at the preset 60 fps)
add_executable(display-fill "display_fill.cpp")
link_with_librckid(display-fill)

# very basic test of the graphics app rendering (cycles through background colors and displays text).  
add_executable(graphics-appRGB "graphics_appRGB.cpp")
link_with_librckid(graphics-appRGB)

# displays PNG image 
add_executable(display-png "display_png.cpp")
link_with_librckid(display-png)

# displays simple ui elements to verify the framework works
add_executable(display-ui "display_ui.cpp")
target_link_libraries(display-ui PRIVATE libgbcemu)
link_with_librckid(display-ui)

# simple gbc emu app test
# TODO might delete this when no longer needed
add_executable(gbcemu-app "gbcemu_app.cpp")
target_link_libraries(gbcemu-app PRIVATE libgbcemu)
link_with_librckid(gbcemu-app)


