cmake_minimum_required(VERSION 3.12)

if (RCKID_BACKEND STREQUAL "FANTASY")

    function(add_utility util_name sources)
        add_executable(${util_name} "${sources}")
        target_include_directories(${util_name} PRIVATE "${CMAKE_SOURCE_DIR}/libs/raylib/src")
        target_include_directories(${util_name} PRIVATE "${CMAKE_SOURCE_DIR}/lib/platform/rckid-fantasy")

        target_link_libraries(${util_name} raylib opus libhelix-mp3 FatFS littlefs)
    endfunction()

    # converts font to glyphinfos & pixels
    add_utility("font-converter" "font_converter.cpp")
    # converts font to tileset
    add_utility("font-tile-generator" "font_tile_generator.cpp")
    # converts all files in a folder into binary arrays of their contents
    add_utility("binary-folder" "binary_folder.cpp")
    # converts given image into raw image in rom (see ImageDecoder.h for details)
    add_utility("raw-image-converter" "raw_image_converter.cpp")

    # Generates the SDK assets
    #
    # The assets are expressed and constexpr uint8_t arrays. See the description of appropriate utilities for the actual tranformation, which depends on the utility/contents used.
    add_custom_target(generate-sdk-assets
        # prepare the system icons, which are raw converted
        COMMAND $<TARGET_FILE:raw-image-converter> ${CMAKE_SOURCE_DIR}/sdk2/assets/icons/icons_64_png ${CMAKE_SOURCE_DIR}/sdk2/assets/icons/icons_64

        COMMAND $<TARGET_FILE:font-converter> fonts/Iosevka.ttf ${CMAKE_SOURCE_DIR}/sdk2/include/assets/Iosevka16.h 16
        COMMAND $<TARGET_FILE:font-converter> fonts/Iosevka.ttf ${CMAKE_SOURCE_DIR}/sdk2/include/assets/Iosevka24.h 24
        COMMAND $<TARGET_FILE:font-converter> fonts/OpenDyslexic.otf ${CMAKE_SOURCE_DIR}/sdk2/include/assets/OpenDyslexic24.h 24
        COMMAND $<TARGET_FILE:font-converter> fonts/OpenDyslexic.otf ${CMAKE_SOURCE_DIR}/sdk2/include/assets/OpenDyslexic32.h 32
        COMMAND $<TARGET_FILE:font-converter> fonts/OpenDyslexic.otf ${CMAKE_SOURCE_DIR}/sdk2/include/assets/OpenDyslexic64.h 64
        COMMAND $<TARGET_FILE:font-converter> fonts/OpenDyslexic.otf ${CMAKE_SOURCE_DIR}/sdk2/include/assets/OpenDyslexic128.h 128

        # images (logo, default background, etc)
        COMMAND $<TARGET_FILE:binary-folder> images ${CMAKE_SOURCE_DIR}/sdk2/include/assets/images.h --raw-path raw/images --namespace rckid::assets::images

        # 64x64 icons (mostly for UI)
        COMMAND $<TARGET_FILE:binary-folder> icons/icons_64 ${CMAKE_SOURCE_DIR}/sdk2/include/assets/icons_64.h --raw-path raw/icons_64 --namespace rckid::assets::icons_64
        # 24x24 icons (mostly for UI)
        COMMAND $<TARGET_FILE:binary-folder> icons/icons_24 ${CMAKE_SOURCE_DIR}/sdk2/include/assets/icons_24.h --raw-path raw/icons_24 --namespace rckid::assets::icons_24

        COMMAND $<TARGET_FILE:font-tile-generator> fonts/Iosevka.ttf ${CMAKE_SOURCE_DIR}/sdk2/include/assets/Iosevka16Tiles.h 16 8 16

        DEPENDS font-converter font-tile-generator binary-folder raw-image-converter
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

else()
    message("SDK utils are only available in RCKID_BACKEND=FANTASY, skipping")
endif()
