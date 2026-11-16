include_directories(${CMAKE_SOURCE_DIR}/libs/raylib/src)

file(GLOB_RECURSE SRC 
    "fantasy.cpp" 
    "virtual_filesystem.cpp" 
    "${CMAKE_SOURCE_DIR}/sdk/src/*.cpp"
    "capabilities/*.cpp"
)
add_library(libkiddo STATIC ${SRC})
target_include_directories(libkiddo PRIVATE "${CMAKE_SOURCE_DIR}/lib/raylib/src")
if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_compile_definitions(libkiddo PUBLIC "KIDDO_NO_RODATA_BOUNDARIES")
else()
    find_package(Threads REQUIRED)
    find_package(CURL REQUIRED)
    target_link_libraries(libkiddo PUBLIC CURL::libcurl)
    target_compile_definitions(libkiddo PUBLIC "KIDDO_CAPABILITY_WIFI")
endif()

# add extra options as macros, especially the KIDDO_DISPLAY_ZOOM, if defined
if (DEFINED KIDDO_DISPLAY_ZOOM)
    target_compile_definitions(libkiddo PUBLIC "KIDDO_DISPLAY_ZOOM=${KIDDO_DISPLAY_ZOOM}")
endif()

# Sets given project to be used with KidDo SDK. 
#
# This must be called for libraries as well as executables (cartridges). For cartridges, an extra argument specifying th cartridge type must be supplied. 
function(link_with_libkiddo target_name)
    get_target_property(target_type ${target_name} TYPE)
    # TODO remove this and make it a compile time decision
    target_compile_definitions(${target_name} PRIVATE "KIDDO_CUSTOM_FILESYSTEM")
    target_include_directories(${target_name} PRIVATE "${CMAKE_SOURCE_DIR}/lib/platform/kiddo-fantasy")
    target_include_directories(${target_name} PRIVATE "${CMAKE_SOURCE_DIR}/sdk/backend/fantasy")
    if (target_type STREQUAL "EXECUTABLE")
        target_link_libraries(${target_name} PRIVATE 
            libkiddo
            raylib
            FatFS
            littlefs
            PNGdec
            PNGenc
            libhelix-mp3
            opus
        )
        if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
            #target_link_options(${target_name} PRIVATE
            #    "--preload-file" "${CMAKE_SOURCE_DIR}/sd@/sd"
            #)
            # nothing needs to be done here, emspcripten flags are set globally in the root CMakeLists.txt
        else()
            target_link_options(${target_name} PRIVATE "-Wl,-T,${CMAKE_SOURCE_DIR}/sdk/backend/fantasy/linker.ld" )    
            target_link_libraries(${target_name} PRIVATE 
                ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT}
            )
        endif()
        target_sources(${target_name} PRIVATE
            ${CMAKE_SOURCE_DIR}/sdk/backend/fantasy/global_new_delete.cpp
        )
    endif()
endfunction()

link_with_libkiddo(libkiddo) 


