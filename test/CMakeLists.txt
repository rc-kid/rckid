if (ARCH STREQUAL "ARCH_FANTASY")
    file(GLOB_RECURSE SRC "*.cpp" "*.h")
    add_executable(tests ${SRC})
    link_with_librckid(tests)

    add_custom_target(run-tests
        COMMAND tests
        DEPENDS tests
        COMMENT "Run tests..."
    )    
else()
    message("Tests are only available in ARCH_FANTASY, skipping")
endif()



