# cmake wrapper for the platformio task 
add_custom_target(test-avr
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test-avr
    COMMAND "pio" "run"
)