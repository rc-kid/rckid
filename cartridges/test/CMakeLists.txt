# the simplest program that simply cycles through red, green and blue full color on the display as fast as possible (note that in fantasy console this actually means at the preset 60 fps)
add_executable(display-fill "display_fill.cpp")
link_with_librckid(display-fill)

# test of the graphics app framework. 
add_executable(graphics-app "graphics_app.cpp")
link_with_librckid(graphics-app)

