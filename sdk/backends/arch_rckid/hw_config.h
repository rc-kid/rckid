#pragma once

/** Hardware Configuration
 
    We have 48 GPIO in total. The following is absolute must for the functionality

    - 12 for display : 8 for data + 4 control wires (CSX, DCX, WRX, TE)
    - 1 for display backlight (pwm)
    - 1 for rumbler (pwm)
    - 2 for audio out (pwm)
    - 2 for mic in (pdm)
    - 2 for I2C
    - 4 for SD card SPI
    - 1 for headphone detection / speker enable
    - 1 for charging detection
    - 2 for LEDs : 1 for power on, 1 for control
    - 7 for button matrix (3x4)
    - 1 power enable
    


 */