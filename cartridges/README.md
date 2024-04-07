# RCKid Cartridges

Contains all cartridge images that are build as part of the RCKid. The following cartridges are somewhat noteworthy, but hecking the actual code is always the best and most up-to-date way. 

## Test Cartridges

Those are intended to evaluate the most basic functionality of the device:

- `blinky` is a super simple standalone program that merely triggers the GPIO21 (available on the cartridge interface) in a 200ms interval so that the basic HW functionality of the system can be verified. Uses only the RPi Pico SDK. 
- `display` resets the display in a loop, while refreshing its entire area with red, green and blue. Useful for testing the stability of the display connection. 

