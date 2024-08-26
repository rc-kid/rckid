
add_executable(hello-world "hello_world.cpp")
target_include_directories(hello-world PRIVATE "${CMAKE_SOURCE_DIR}/apps")
link_with_librckid(hello-world)
