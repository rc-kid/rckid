
cmake_minimum_required(VERSION 3.12)

add_executable(fb-hello-rgb "fb-rgb.cpp" "fb-test.h")
link_with_librckid(fb-hello-rgb)

add_executable(fb-hello-256 "fb-256.cpp" "fb-test.h")
link_with_librckid(fb-hello-256)

add_executable(fb-hello-2x-rgb "fb-2x-rgb.cpp" "fb-test.h")
link_with_librckid(fb-hello-2x-rgb)

add_executable(fb-hello-2x-256 "fb-2x-256.cpp" "fb-test.h")
link_with_librckid(fb-hello-2x-256)

