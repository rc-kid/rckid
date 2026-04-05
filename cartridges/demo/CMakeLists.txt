if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    add_executable(demo "demo.cpp")
    target_link_libraries(demo PRIVATE libgbcemu)
    link_with_librckid(demo)
    target_link_options(demo PRIVATE
        "--preload-file" "${CMAKE_SOURCE_DIR}/cartridges/demo/sd@/sd"
    )
endif()

