# TODO

> Due to lack of displays, I have only 2 prototypes that should be working with a display & battery, those will be used for xmas, I have an extra prototype that works with display, but not with battery (faulty charger), but can be tested for the AAA cells. 

- add blit that ignores particular color

- verify charging & the new AVR code on a prototype board,2 then put it in the new case

- maybe not use joystick, but keep it as an extra file so that each app can configure it properly, including things like position. But how to integrate with settings? 

- clean music & tone generators, ensure other frequencies work as well, document, benchmark

- check the critical battery warning & new AVR
- add logo when clearing the screen
- can easily port littlefs for cartridge storage filesystem
- USB serial does not display all text, sometimes garbled, as if not blocking to send

- move games & apps to SDK, unless they belong in cartridges 

- make mp3 player and video as a TV so that the mp3's have daily schedule that you can do

- update the order of args in drawing methods (pos, color) etc

- add blocking send & test it

- account for PNG's transparency by allowing it to be overlayed on existing bitmap (right now it is set to black which looks ok on black background)
- maybe even RGBA colors? 
- rumbler & LED effect done event

- recording in fantasy

- mp3 decoder library (added, write API)
- libopus decoder & encoder library
- jpeg decoding library

- add a way to silence button down presses (such as btnPressedClear() function that makes any subseuent )

- filesystem file & folder abstractions
- add filesystem support with an ISO file specified for the SD card, using the same fatfs mechanics

- can swap RAM arenas to disk if necessary

- make timer run a ... b ... a as well (then I can replace the blinking code in clock)

## GalaxyInvaders

## Sokoban

- add new levels, use images 

## Fantasy

- implement alarm

## PCB

- see if the riser for the cartridge connector is necessary, or the cartridge leads can be bent, soldered and then the connector glued

## Mk II Hardware

- I2C problems might be somewhat mitigated by extra pullups in the cartridge, or add a special mode where if DC power is enabled we stop querying and just display that we have entered charging and only stop when charging is stopped? - as a really ugly hack, but will work 

- does not work with 5V from laptop

## Mk III

> on RP2350 we have more memory, so maybe ditch the arena memory allocator and just have full framebuffer form 320x240 allocated as graphics memory at the beginning forever. On 2350 this is some 30% of RAM which is ok. This should greatly simplify the rendering pipeline.

> With ability to read from the display uninterrupted, we can also reinitialize this any time with the existing framebuffer for all kinds of cool effects.

- the 3v3 rail to onboard sensors & RTC is always on, this enables time & steps tracking to be valid even across cartridges. The RTC memory (if present) can be used for some basic storage as well  

### Display woes

> The displays I ordered for the prototypes are 8bit interface, while the larger batch is 16. This in itself would not be the worst, but it seems that the 8bit display can no longer be purchased. I have 2 working prototypes for xmas, so if all goes well, it *might* work still, but the plan for the future is to use the extra pins on RP2350B for the 16bit interface so that I can use the displays I have ordered already. 

The RPI has 48 GPIO, the necessary pins are:

- 20 (16 + 4) for the display
- 2 pins for I2C communication with the AVR
- 10 pins for the cartridge
- 4 pins for the SD card
- 2 pins for stereo audio out
- 2 pins for the PDM microphone

This is 40 pins already, with 8 pins left. Those can either be used for the buttons, or provide extras such as:

- 1 pin will make I2S sound output possibility
- 2 pins will make the SDIO interface to the SD card possibility
- 1 for display read as well

This leaves us with 4 free pins on the RP2350 side. 

Then there is the ATTiny3217, which has 21 GPIO pins:

- 2 pins for I2C
- 1 pin for power
- 2 pins for RGB & 5V power
- 1 vbatt (ADC)
- 1 vcc (ADC)
- 1 pin charging detection
- 1 pin charging on/off
- 7 pins for the buttons (3x4 matrix, including the home button)

This is 16 pins, with 5 pins left on the AVR side. 

Things we are missing is audio on/off and headphone detection (1 or 2 pins), backlight (1pin), rumbler (1pin)

I can put RP in charge of the audio, while keeping the backlihght & rumbler on AVR, leaving RP with 2 free pins (UART for debugging or some aoutput pins) and 3 pins on AVR (uart for debugging and some leds, etc.)

> Turns out it actually might work even with the 16bit display and the 16bit resolution can even give faster communication and thus more time available for draw in framebuffer mode. 
