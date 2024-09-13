if (ARCH STREQUAL "ARCH_FANTASY")
    file(GLOB_RECURSE SRC "*.cpp" "*.h")
    file(GLOB_RECURSE GBCEMU_SRC "${CMAKE_SOURCE_DIR}/cartridges/gbcemu/tests/*.cpp" "${CMAKE_SOURCE_DIR}/cartridges/gbcemu/tests/*.h")
    add_executable(tests ${SRC} ${GBCEMU_SRC})
    target_link_libraries(tests PRIVATE libgbcemu)
    link_with_librckid(tests)

    add_custom_target(run-tests
        COMMAND tests
        DEPENDS tests
        COMMENT "Run tests..."
    )    
else()
    message("Tests are only available in ARCH_FANTASY, skipping")
endif()



