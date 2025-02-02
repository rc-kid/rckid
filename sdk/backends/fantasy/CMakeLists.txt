include_directories(${CMAKE_SOURCE_DIR}/libs/raylib/src)
find_package(Threads REQUIRED)

# Set general compiler options, C++ standard and warning levels
set(CMAKE_CXX_STANDARD 17)  
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

file(GLOB_RECURSE SRC "fantasy.cpp" "${CMAKE_SOURCE_DIR}/sdk/rckid/*.h" "${CMAKE_SOURCE_DIR}/sdk/rckid/*.cpp" "${CMAKE_SOURCE_DIR}/sdk/assets-bin/*.cpp")
add_library(librckid STATIC ${SRC})
target_include_directories(librckid PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/raylib/src")

# Sets given project to be used with RCKid SDK. 
#
# This must be called for libraries as well as executables (cartridges). For cartridges, an extra argument specifying th cartridge type must be supplied. 
function(link_with_librckid target_name)
    get_target_property(target_type ${target_name} TYPE)
    target_include_directories(${target_name} PRIVATE "${CMAKE_SOURCE_DIR}/libs/platform/pc")
    target_include_directories(${target_name} PRIVATE "${CMAKE_SOURCE_DIR}/sdk/backends/fantasy")
    if (target_type STREQUAL "EXECUTABLE")
        target_link_libraries(${target_name} PRIVATE 
            librckid
            raylib
            FatFS
            littlefs
            PNGdec
            libhelix-mp3
            ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT}
        )
    endif()
endfunction()

link_with_librckid(librckid) 


