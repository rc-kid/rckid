cmake_minimum_required(VERSION 3.12)

file(GLOB_RECURSE SRC "basic/cartridge.cpp")
add_library(librckid-cartridge-basic ${SRC})
link_with_librckid(librckid-cartridge-basic)

file(GLOB_RECURSE NRF_SRC "radio-nrf/*.h" "radio-nrf/cartridge.cpp" "${CMAKE_SOURCE_DIR}/common/radio/*.h" "${CMAKE_SOURCE_DIR}/common/radio/nrf.cpp")
add_library(librckid-cartridge-radio-nrf ${NRF_SRC})
link_with_librckid(librckid-cartridge-radio-nrf)

file(GLOB_RECURSE NRF_SRC "radio-wifi/*.h" "radio-wifi/cartridge.cpp" "${CMAKE_SOURCE_DIR}/common/radio/*.h" "${CMAKE_SOURCE_DIR}/common/radio/uart.cpp")
add_library(librckid-cartridge-radio-wifi ${NRF_SRC})
link_with_librckid(librckid-cartridge-radio-wifi)
