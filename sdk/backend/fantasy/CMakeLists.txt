include_directories(${CMAKE_SOURCE_DIR}/libs/raylib/src)

file(GLOB_RECURSE SRC 
    "fantasy.cpp" 
    "virtual_filesystem.cpp" 
    "${CMAKE_SOURCE_DIR}/sdk/src/*.cpp"
    "capabilities/*.cpp"
)
add_library(librckid STATIC ${SRC})
target_include_directories(librckid PRIVATE "${CMAKE_SOURCE_DIR}/lib/raylib/src")
if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    #set(EM_FLAGS
    #    "-sUSE_PTHREADS=1"
    #    "-sPTHREAD_POOL_SIZE=4"
    #)
    #target_link_options(librckid PRIVATE ${EM_FLAGS})
    target_compile_definitions(librckid PUBLIC "RCKID_NO_RODATA_BOUNDARIES")
else()
    find_package(Threads REQUIRED)
    find_package(CURL REQUIRED)
    target_link_libraries(librckid PUBLIC CURL::libcurl)
    target_compile_definitions(librckid PUBLIC "RCKID_CAPABILITY_WIFI")
endif()

# Sets given project to be used with RCKid SDK. 
#
# This must be called for libraries as well as executables (cartridges). For cartridges, an extra argument specifying th cartridge type must be supplied. 
function(link_with_librckid target_name)
    get_target_property(target_type ${target_name} TYPE)
    # TODO remove this and make it a compile time decision
    target_compile_definitions(${target_name} PRIVATE "RCKID_CUSTOM_FILESYSTEM")
    target_include_directories(${target_name} PRIVATE "${CMAKE_SOURCE_DIR}/lib/platform/rckid-fantasy")
    target_include_directories(${target_name} PRIVATE "${CMAKE_SOURCE_DIR}/sdk/backend/fantasy")
    if (target_type STREQUAL "EXECUTABLE")
        target_link_libraries(${target_name} PRIVATE 
            librckid
            raylib
            FatFS
            littlefs
            PNGdec
            PNGenc
            libhelix-mp3
            opus
        )
        if (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
            #set(EM_FLAGS
            #    "-sUSE_PTHREADS=1"
            #    "-sPTHREAD_POOL_SIZE=4"
            #)
            #target_link_options(${target_name} PRIVATE ${EM_FLAGS})
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

link_with_librckid(librckid) 


