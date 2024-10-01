# TODO

- is it ok to remove the cartridge while powered on?

- use NOR gate for pin from RP and button so that pressing the button powers rpi on
- https://jlcpcb.com/partdetail/Msksemi-SN74LVC1G04DBVRMS/C7434171

- actually don't do anything crazy, have the RP always powered on (10uA), but allow disengaging the cartridge's power supply - might be able to do this even for sleep? 

## RP2350

- 2 -- I2C - connected to accelerometer, light detector, INA219 if present
- 8 -- display data
- 4 -- display control
- 1 -- display read (?)
- 1 -- backlight PWM
- 1 -- rumbler PWM
- 2 -- audio PWM out
- 2 -- mic PDM in
- 4 -- SD-CARD SPI
- 10 -- cartridge (including QSPI second SS, SPI and I2C)
- 2 -- 5V en and neopixel out
- 1 -- battery voltage detection
- 1 -- headphone detection / audio on
- 2 -- charging detection & control (or just 1 for charging detection)
- 7 -- matrix for buttons (3x4)

> 48(46) pins that we absolutely need (-2)

This just might work. 

### Problems

- how to enter bootloader mode (seems to be possible over software alone by forcing the USB open close at 1200 bps, there also seems to be a way to start BOOTSEL programatically if I read the datasheet right)
- how to reset (can be SW on watchdog and detection)

## SDK

- update the order of args in drawing methods (pos, color) etc
- add space invaders :)

- add blocking send & test it

- add position & things to renderers for bitmap

- account for PNG's transparency by allowing it to be overlayed on existing bitmap
- maybe even RGBA colors? 
- rumbler & LED effect done event

- recording in fantasy
- audio in mkII

- mp3 decoder library
- libopus decoder & encoder library
- jpeg decoding library

- filesystem file & folder abstractions
- add filesystem support with an ISO file specified for the SD card, using the same fatfs mechanics

- can swap RAM arenas to disk if necessary

### mkIII

- when switching to mkIII fix the order in app.cpp (tick before update)

## PCB

- see if the riser for the cartridge connector is necessary, or the cartridge leads can be bent, soldered and then the connector glued



## Tetris

- pause & game over and stuff
- drawing speed might be too slow for 60fps (check)


## Mk II Hardware

- the tick takes too long it seems (when to I2C comms with AVR there are no resets & FPS is correct at 60fps)
- this only affect the black prototype - can be wrong AVR programming

- menu app is weird, the generator vs action does not seem to work well
- not sure why, try some log prints
- does not work with 5V from laptop
- for mk II can disable running RP2040 when on DC power perhaps
- avr tick speed seems to be an issue as well
- won't power on with long home button key - can be related to the above

- remove charging current from state






## Mk III Hardware

- the 3v3 rail to onboard sensors & RTC is always on, this enables time & steps tracking to be valid even across cartridges. The RTC memory (if present) can be used for some basic storage as well  
- bootloader switch can be optionally on the cartridge. Otherwise SW boot is possible as well
- the RP and cartridge can be powered down (and will), and the home button press turns it on 
- software can enable bootloader mode as well


- maybe keep avr because it can: 
- act as RTC
- act as Home button press detection 
- help with resetting 
- aleviate the pin count pressure
- actually keep for sure

Pins:

- 2 -- I2C - connecterd to AVR, accel, light detector & INA219 if present (?)
- 8 -- display data
- 4 -- display control
- 2 -- audio PWM out
- 1 -- mic PDM in
- 4 -- SD-CARD SPI
- 10 -- cartridge (including QSPI secons SS, SPI, I2C, UART, programmable if necessary)
- 8 -- top plate buttons 
- 39 

AVR

- 3 -- buttons (home, vol up, vol down)
- 2 -- I2C
- 1 -- backlight PWM
- 1 -- rumbler PWM
- 1 -- 3V3 power on
- 1 -- 5V power on
- 1 -- neopixel
- 1 -- charging (detect)
- 1 -- vbatt
- 1 -- audio detection
- 13

- how to deal with power & I2C? 
- power goes to 3v3 switching, which *always* powers the AVR & sensors
- the sensors should not leak voltage to I2C
 
