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
> verify the work on windows too

- filesystem file & folder abstractions
- add filesystem support with an ISO file specified for the SD card, using the same fatfs mechanics

### mkIII

- when switching to mkIII fix the order in app.cpp (tick before update)

## PCB

- see if the riser for the cartridge connector is necessary, or the cartridge leads can be bent, soldered and then the connector glued