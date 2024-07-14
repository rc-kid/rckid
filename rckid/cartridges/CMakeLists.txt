cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SRC "basic/cartridge.cpp")
add_library(librckid-cartridge-basic ${SRC})
link_with_librckid(librckid-cartridge-basic)

if (ARCH STREQUAL "RP2040")

    file(GLOB_RECURSE NRF_SRC "radio-nrf/*.h" "radio-nrf/*.cpp")
    add_library(librckid-cartridge-radio-nrf ${NRF_SRC})
    link_with_librckid(librckid-cartridge-radio-nrf)

    file(GLOB_RECURSE NRF_SRC "radio-wifi/*.h" "radio-wifi/cartridge.cpp" "radio-wifi/radio-uart.cpp")
    add_library(librckid-cartridge-radio-wifi ${NRF_SRC})
    link_with_librckid(librckid-cartridge-radio-wifi)

elseif (ARCH STREQUAL "MOCK")

    find_package(CURL REQUIRED)

    file(GLOB_RECURSE NRF_SRC "radio-wifi/*.h" "radio-wifi/cartridge.cpp" "radio-wifi/curl.cpp")
    add_library(librckid-cartridge-radio-wifi ${NRF_SRC})
    link_with_librckid(librckid-cartridge-radio-wifi)
    target_link_libraries(librckid-cartridge-radio-wifi ${CURL_LIBRARIES})

endif()
