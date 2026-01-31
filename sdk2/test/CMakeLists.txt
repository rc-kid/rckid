cmake_minimum_required(VERSION 3.12)

# SDK tests
#
# The unittests are only available when building for the FANTASY console backend. 
# TODO does it have to be? 
if (RCKID_BACKEND STREQUAL "FANTASY")

    file(GLOB_RECURSE SDK_SRC  "*.cpp" "*.h")
    file(
        GLOB_RECURSE PLATFORM_SRC 
        "${CMAKE_SOURCE_DIR}/lib/platform/test/*.cpp"
        "${CMAKE_SOURCE_DIR}/lib/platform/test/*.h"
    )
    add_executable(tests-sdk ${SDK_SRC} ${PLATFORM_SRC})
    link_with_librckid(tests-sdk)

    add_custom_target(run-tests
        COMMAND tests-sdk
        DEPENDS tests-sdk
        COMMENT "Run tests..."
    )    
else()
    message("Tests are only available in RCKID_BACKEND=FANTASY, skipping")
endif()
