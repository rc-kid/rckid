
# Future

> This will likely be RP2350 based version with 16bit display interface some time in 2025.

- fix the AVR - RP2040 voltage problem
- can we make RP2040 to be USB host & guest at the same time so that we can for instance attach keyboard? 

# MkII version 2.3 - 19/6/24

Fixes the sound & charging problems. Charge enable is wired to the other end of the charge programming resistor so leaving it floating disables charging and setting it low enables charging by providing path to ground with no voltage leaking when disabled.

However in this version the flaw with AVR running from VCC becomes apparent as with some chargers & devices the difference of voltage between AVR and 3.3V for I2C becomes too high and chip communication does not work when DC power is connected. 

> This is the version intended for xmas 24. 

# MkII version 2.2 - 9/3/24

Second iteration and my development device for a long time. Has modern layout, small screw holes, unified buttons and optimized layout. Small RGBs, fixed voltage comversion. Faulty speaker connection so speaker sound is extremely low volume. Faulty charging enable connection where the charge program resistor always goes to ground and charge en pin is wired directly to the charge en pin of the charger, thus leaving charge en to float enables charging and setting it high disables charging. This leaks voltage to the charger and through the charger to the VCC line which partially opens the VBATT/VCC switching mosfet lowering battery voltage. Contains INA219 for current measurement. 

# MkII version 0.3 - 29/10/23

This is the first version of MkII. PCB is immediately distinguishable by large holes for the screwposts (in newer versions the screws themselves go through the PCB), smaller size (no cutout for the speaker) and huge logo on the back. This was the first prototype with RP2040. Contains magnetometer and humidity sensor onboard and large Neopixel LEDs. Voltage converter between RGB and AVR did not work properly and the LEDs only worked on large enough vcc/vbatt.