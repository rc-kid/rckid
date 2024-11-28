include_directories(${CMAKE_SOURCE_DIR}/libs/raylib/src)
find_package(Threads REQUIRED)

file(GLOB_RECURSE SRC "raylib.cpp" "${CMAKE_SOURCE_DIR}/sdk/rckid/*.h" "${CMAKE_SOURCE_DIR}/sdk/rckid/*.cpp" "${CMAKE_SOURCE_DIR}/sdk/assets-bin/*.cpp")
add_library(librckid STATIC ${SRC})
target_include_directories(librckid PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/raylib/src")


# Sets given project to be used with RCKid SDK. 
#
# This must be called for libraries as well as executables (cartridges). For cartridges, an extra argument specifying th cartridge type must be supplied. 
function(link_with_librckid target_name)
    get_target_property(target_type ${target_name} TYPE)
    target_include_directories(${target_name} PRIVATE "${CMAKE_SOURCE_DIR}/libs/platform/rckid-fantasy")
    target_include_directories(${target_name} PRIVATE "${CMAKE_SOURCE_DIR}/sdk/backends/arch_fantasy")
    if (target_type STREQUAL "EXECUTABLE")
        target_link_libraries(${target_name} PRIVATE 
            librckid
            raylib
            FatFS
            littlefs
            PNGdec
            ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT}
        )
    endif()
endfunction()

link_with_librckid(librckid) 


