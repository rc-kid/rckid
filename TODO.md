# TODO

- remove easing and use interpolation for its purposes as well

- turn audio off when powering off for sure
- check the critical battery warning & new AVR
- temperature does not seem to be right
- effects and direct RGB settings not playing nicely together either (setting normal value should clear effect & green for )
- add logo when clearing the screen
- add audio volume (scale the 16bit ints)

- make mp3 player and video as a TV so that the mp3's have daily schedule that you can do

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

- USB serial does not display all text, sometimes garbled, as if not blocking to send
- when sliding puzzle is entered and then left, pong no longer works, not sure why...
- has to do with spurious button presses - or maybe not clened well when app transitions? 

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

- I2C problems might be somewhat mitigated by extra pullups in the cartridge, or add a special mode where if DC power is enabled we stop querying and just display that we have entered charging and only stop when charging is stopped? - as a really ugly hack, but will work 

- does not work with 5V from laptop

- update the modal stuff how it works and the focus & blur methods to make it more resilient. Add docs warning for making sure to call the parent focus blur !!!!!

## Mk III

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

