if (RCKID_BACKEND STREQUAL "FANTASY")
    # Set general compiler options, C++ standard and warning levels
    set(CMAKE_CXX_STANDARD 17)  
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    if (MSVC)
        add_compile_options(/W4)
    else()
        add_compile_options(-Wall -Wextra)
    endif()


    file(
        GLOB_RECURSE SDK_SRC 
        "${CMAKE_SOURCE_DIR}/sdk/tests/*.cpp"
        "${CMAKE_SOURCE_DIR}/sdk/tests/*.h"
    )
    file(
        GLOB_RECURSE PLATFORM_SRC 
        "${CMAKE_SOURCE_DIR}/libs/platform/tests/*.cpp"
        "${CMAKE_SOURCE_DIR}/libs/platform/tests/*.h"
    )
    file(
        GLOB_RECURSE GBCEMU_SRC 
        "${CMAKE_SOURCE_DIR}/gbcemu/tests/*.cpp" 
        "${CMAKE_SOURCE_DIR}/gbcemu/tests/*.h"
    )
    add_executable(tests-sdk ${SDK_SRC} ${PLATFORM_SRC} ${GBCEMU_SRC})
    target_link_libraries(tests-sdk PRIVATE libgbcemu)
    link_with_librckid(tests-sdk)


    add_custom_target(run-tests
        COMMAND tests-sdk
        DEPENDS tests-sdk
        COMMENT "Run tests..."
    )    
else()
    message("Tests are only available in RCKID_BACKEND=FANTASY, skipping")
endif()



