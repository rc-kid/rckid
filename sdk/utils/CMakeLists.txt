function(add_utility util_name)
    add_executable(${util_name} "${util_name}.cpp")
    target_include_directories(${util_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/raylib/src")
    target_include_directories(${util_name} PRIVATE "${CMAKE_SOURCE_DIR}/libs/platform/pc")

    target_link_libraries(${util_name} raylib)
endfunction()

add_utility("generate-assets")

add_custom_target(generate-sdk-assets 
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/sdk/assets
    COMMAND echo "Generating SDK assets..."
    COMMAND generate-assets assets.def ${CMAKE_SOURCE_DIR}/sdk/rckid/assets rckid::assets
    DEPENDS generate-assets
)
