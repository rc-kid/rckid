# cmake wrapper for the platformio task 
add_custom_target(rckid-avr
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/rckid/avr  
    COMMAND "pio" "run"
)