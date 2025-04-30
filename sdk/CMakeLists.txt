cmake_minimum_required(VERSION 3.12)

# add the backend as selected
string(TOLOWER "${RCKID_BACKEND}" BACKEND_LOWER)
add_subdirectory("backends/${BACKEND_LOWER}")

# now add the utils if we are in fantasy build
if (RCKID_BACKEND STREQUAL "FANTASY")

    function(add_utility util_name sources)
        add_executable(${util_name} "${sources}")
        target_include_directories(${util_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/raylib/src")
        target_include_directories(${util_name} PRIVATE "${CMAKE_SOURCE_DIR}/libs/platform/pc")

        target_link_libraries(${util_name} raylib opus libhelix-mp3 FatFS littlefs)
    endfunction()

    add_utility("font-converter" "utils/font_converter.cpp")
    add_utility("binary-folder" "utils/binary_folder.cpp")
    add_utility("waveforms" "utils/waveforms.cpp")
    add_utility("font-tile-generator" "utils/font_tile_generator.cpp")

    # Generates the SDK assets
    #
    # The assets are expressed and constexpr uint8_t arrays. See the description of appropriate utilities for the actual tranformation, which depends on the utility/contents used.
    add_custom_target(generate-sdk-assets
        #COMMAND $<TARGET_FILE:font-converter> assets/fonts/Iosevka.ttf rckid/assets/fonts/Iosevka16.h 16
        #COMMAND $<TARGET_FILE:font-converter> assets/fonts/OpenDyslexic.otf rckid/assets/fonts/OpenDyslexic64.h 64
        #COMMAND $<TARGET_FILE:binary-folder> assets/images rckid/assets/images.h --raw-path raw/images
        #COMMAND $<TARGET_FILE:binary-folder> assets/icons/default/64 rckid/assets/icons_default_64.h --raw-path raw/icons --namespace rckid::assets::icons_default_64
        #COMMAND $<TARGET_FILE:waveforms> 256 rckid/assets/waveforms.h
        #COMMAND $<TARGET_FILE:font-tile-generator> assets/fonts/Iosevka.ttf rckid/assets/tiles/System16.h 16 8 16
        DEPENDS font-converter
        DEPENDS binary-folder
        DEPENDS waveforms
        DEPENDS font-tile-generator
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

else()
    message("SDK utils are only available in RCKID_BACKEND=FANTASY, skipping")
endif()


