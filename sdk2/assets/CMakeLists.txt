cmake_minimum_required(VERSION 3.12)

if (RCKID_BACKEND STREQUAL "FANTASY")

    function(add_utility util_name sources)
        add_executable(${util_name} "${sources}")
        target_include_directories(${util_name} PRIVATE "${CMAKE_SOURCE_DIR}/libs/raylib/src")
        target_include_directories(${util_name} PRIVATE "${CMAKE_SOURCE_DIR}/lib/platform/rckid-fantasy")

        target_link_libraries(${util_name} raylib opus libhelix-mp3 FatFS littlefs)
    endfunction()

    add_utility("font-converter" "font_converter.cpp")

    # Generates the SDK assets
    #
    # The assets are expressed and constexpr uint8_t arrays. See the description of appropriate utilities for the actual tranformation, which depends on the utility/contents used.
    add_custom_target(generate-sdk-assets
        COMMAND $<TARGET_FILE:font-converter> fonts/Iosevka.ttf ${CMAKE_SOURCE_DIR}/sdk2/include/assets/Iosevka16.h 16
        COMMAND $<TARGET_FILE:font-converter> fonts/Iosevka.ttf ${CMAKE_SOURCE_DIR}/sdk2/include/assets/Iosevka20.h 20
        COMMAND $<TARGET_FILE:font-converter> fonts/OpenDyslexic.otf ${CMAKE_SOURCE_DIR}/sdk2/include/assets/OpenDyslexic24.h 24
        COMMAND $<TARGET_FILE:font-converter> fonts/OpenDyslexic.otf ${CMAKE_SOURCE_DIR}/sdk2/include/assets/OpenDyslexic32.h 32
        COMMAND $<TARGET_FILE:font-converter> fonts/OpenDyslexic.otf ${CMAKE_SOURCE_DIR}/sdk2/include/assets/OpenDyslexic64.h 64
        COMMAND $<TARGET_FILE:font-converter> fonts/OpenDyslexic.otf ${CMAKE_SOURCE_DIR}/sdk2/include/assets/OpenDyslexic128.h 128

        DEPENDS font-converter
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

else()
    message("SDK utils are only available in RCKID_BACKEND=FANTASY, skipping")
endif()
