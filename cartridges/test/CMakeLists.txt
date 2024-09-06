# the simplest program that simply cycles through red, green and blue full color on the display as fast as possible (note that in fantasy console this actually means at the preset 60 fps)
add_executable(display-fill "display_fill.cpp")
link_with_librckid(display-fill)

# test of the graphics app framework (RGB colors) 
add_executable(graphics-appRGB "graphics_appRGB.cpp")
link_with_librckid(graphics-appRGB)

# test of the graphics app framework (256 colors) 
add_executable(graphics-app256 "graphics_app256.cpp")
link_with_librckid(graphics-app256)

# test of the graphics app framework (16 colors)
add_executable(graphics-app16 "graphics_app16.cpp")
link_with_librckid(graphics-app16)


