# DevBoard Checklist

- [X] verify there are no shorts between VCC, VBATT, VDD, 3V3 and ground
- [X] add jumper wires for D and audio power (! these *must* be installed before the board is powered)
- [X] add connectors for battery
- [X] try powering up through battery connector at 4.0V
- [X] try powering up through USB at 5 volts
- [-] try powering up through battery at 3.0 volts to see if the PMIC keeps the VCC above threshold 

> This could be due to the fact that OTG is not enabled, check if enabling it fixes the problem. If PMIC is changed, this might not be a problem any more.

- [X] verify cartridge connector integrity on unpopulated dev board
- [X] solder cartridge connector on dev board
- [X] verify that UPDI works when AVR powered with 3v3
- [X] breadboatd AVR test Serial out
- [X] breadboard AVR test RGB
- [X] breadboard AVR test rumbler
- [X] breadboard AVR test backlight
- [X] breadboard AVR test IRQ
- [X] breadboard AVR test buttons
- [X] breadboard AVR test I2C master ennumeration and some basic I2C comms
- [X] breadboard AVR test power on / power off
- [X] modify avr to not power rp2350 on, switch to ATTiny3217
    - [X] flash the AVR on device
    - [X] device AVR test Serial out
    - [X] device AVR test RGB
    - [X] device AVR test buttons
    - [X] device AVR test I2C master ennumeration
    - [X] switch to external osc for RTC on the devboard
    - [X] test device clock accuracy with external clock
- [X] modify AVR to turn on/off
- [X] device AVR test power on / power off
- [ ] device AVR test rumbler -- it runs off IOVDD, has to be checked after IOVDD en 
- [X] check that RP2350 shows on host machine as USB drive
- [X] enumerate I2C devices (now we should see also audio & radio)
- [X] check serial test on RPi Pico 2
- [X] flash RP2350 with serial test and see output
- [X] check that AVR can reboot RP
- [X] verify that AVR can reset RP2350 into bootloader mode
- [X] ensure logging works with initialize() call as well
- [X] check that RP can talk to I2C devices
- [X] check that RP can talk to SD card
- [X] check that RP can talk to the display & show data (worst but RST pin must be held high)
- [X] check TLV320 GPIO
- [X] check TLV320 power consumption after reset (seems small)
- [ ] check TLV320 power consumption in standby / sleep modes
- [ ] check TLV320 detects headphones and headphone button press 
- [ ] check TLV320 I2S DAC headphone output
- [ ] check TLV320 I2S DAC line out/speaker output
- [ ] check TLV320 can route Si4705 output
- [ ] check Si4705 works with headphones antenna
- [ ] check Si4705 works with internal antenna
- [ ] check TLV320 can record from radio
- [ ] check TLV320 can record from microphone
- [ ] check low power radio bypass via analog input 1 (requires rerouting mic & radio on the board) - maybe not necessary
- [ ] check SD card insertion detection
- [ ] check powered from real battery works
- [ ] check USB detection works when powered from battery
- [ ] check DataSync app
- [ ] check the above still works if we add USB ESD protection
- [ ] disable debug mode on by default on AVR (end of initialize)
- [X] order breakout for new PMIC chip and verify its functionality separately
- [ ] charging/discharging
- [ ] boost for below 3.3V battery
- [X] check that 2 LEDs under DPAD sides work ok (HW.md)
- [ ] check that titlting dpad is better 
- [ ] check new case works better with battery
- [ ] mk3 idle and keepalive timers are too high, move lower again
- [ ] check overclocking
- [ ] add way to clear memory when necessary (e.g. remove background, etc.)

# TODO

- Icon::toBitmap should really not use the arena, but heap instead to ensure that the icons that are created as part of the 
- or actually use arena, but in Image::create the image first according to the decoder's width and height, then enger new arena for the decoder, decode, leave arena, be done 

- convert info dialog to use icons
- see if we can enable exceptions, what are the runtime & stack costs of them running

- ColorRGB is weird, should be colorRGBA and then have Color565 as a version that wraps around 15bpp? Then change uint16_t everywhere in palettes

- maybe have bitmap to be variable bpp, unlike tiles. And then have canvas which is like bitmap, but fixed depth and drawing primitives. 

- comment PNG loading stuff

- figure out what to do with app when budget dies (maybe just call its save if applicable)
- display budget in header

- ok, so it is not out of memory, but stack protection kicks in 

- file browser should be similar to contacts, i.e. the dialog in launcher menu instead of ui element and separate browser app 

- main menu should also show next birthdays, alarm if any, and oher most current information

- add version of background wher the background just bounces so that the background can be some actual image

- todo figure out how context menu can be planted to apps (or perhaps let the dialogs handle them)

- detects headphones in, but not out, and does not detect cellular, only stereo. This could be because of two microphones being attached to the micbias

- when we do mk3, make sure that when a button press is detected, the idle flag is cleared 
- keepalive when plugged in can be indefinite? 

- clean-up the code around display initialization
- colors are revrsed on mk3. This can either be solved in software by using reversed color values, or in theory could be done by the pio itself. Is it necessary?

- SD card detection works-ish, the sd detect pin is floating when no card and connected to ground when card - figure how to use
- si4705 does not seem to support multiple start sessions, rewrite the Si4705 platform library for proper command sending & response polling
- then try an I2C device driver using the I2C queue, etc. 

- determine correct rumbler settings for ok fail and nudge

- document host file system

- fix/check device fatal eror & stack protection
- improve memory allocator (heap)
- verify other std::string uses

- add events for getting icon for files & on file selected for file browser
- add actions for file browser such as rename, copy, mkdir, etc.  


- make the popupmenu use animations and labels instead of the tilemap
- add icons for stopwatch, timer, audio Images and utilities


## HW

- could this be better PMIC? https://www.ti.com/product/BQ25628
(E) variant. Actually no - see https://e2e.ti.com/support/power-management-group/power-management/f/power-management-forum/1257114/faq-bq25620-how-is-vsys-regulated-for-bq25620-22-and-bq25628-29-what-conditions-determine-how-it-s-regulated-when-is-sysmin-threshold-valid, the VSYS is not regulated when charger not connected. We should go back to simple charger and buck-boost converters for 3v3
- use https://www.ti.com/product/TPS63001 instead - seems smallish enough hopefully
- could this be better audio codec? https://jlcpcb.com/partdetail/NuvotonTech-NAU88C22YG/C914209 (it's cheaper, has integrated speaker driver, simpler to interact with, but requires MCLK, that has to be provided by the chip, it has headphone detection and one extra GPIO, no HW reset pin required)
- slimmer connector https://jlcpcb.com/partdetail/XkbConnection-X05A10H40G/C528037
- swap mic & radio input so that we can do low power audio pass through from radio directly to headphone amps of the audio codec
- 4k7 Ohm pull up from ldo select to vcc (now direct connection)
- buy header pins (plenty for the connectors)

- A button should be square (turned 45deg)

## AVR

- see if we can run at 5MHz and still talk to neopixel
- I2C master enumeration works, but read register does not - is this true still? 

## UI

- proper palette rendering, do offsets
- palettes in the UI stuff (header & dialogs)
- when updating multiple attributes of a widget the recalculate after each one of them is not necessary
- also maybe change the resize to change and make it general method for ui change stuff

## Audio

- add square and white noise waveforms
- should audio volume be uint8, or something else? 
- in fantasy, add dedicated thread for audio buffer refill so that the audio is clean

## Graphics

- should colorRGB be 3 bytes? Or should it be true to the 565 representation? 
- visit loadImage to determine if the setAt is good enough (conversition from int16 to Color to int16 seems inefficient)
- image can have pixel arrays form ROM as well - just ensure on the fantasy that we are not deleting the memory if it comes form "ROM" - sth like lazybitmap? 

- optimize surface functions for common cases
- add specialization for 16 bpp bitmap renderColumn that simply does memcopy

## Apps

### AudioPlayer

- label scrolling
- maybe smaller icon?
- image picture
- song left/right, repeat/no repeat
- does not work on mk2

### DataSync

- update the DataSync app so that it actually works as intended

### GBCEmu

- tearing is kinda ugly, can be fixed by framebuffer, will cost around 23k, but then scaling & rendering can be done by other core

## Others

- memory leaks via reusing large parts for small items, this way we eventually run out of memory
- waiting for display update done could make the cpu sleep
- serialize & deserialize vs load & save

# PCB Things To Fix

- [X] RP2350 does not have RUN pin connected (!!)
- [X] VREG_VIN should go to IOVDD
- [X] ADC_AVDD should to to IOVDD (?)
- [ ] test ADC_AVDD to IOVDD connection and if necessary


- JLCPCB offers 3d printing as well - perhaps I can use it to print stuff from nicer materials such as transparent faceplate or even the case, etc. 

# PCB

- R125 (ilim for PMIC is wrong value)
- verify with prototype that R124 (PWR_INT pullup is needed)

> This is updated TODO list for mkIII. It's split into 

- 2.8" 320x240 with fpc: https://www.aliexpress.com/item/1005004629215040.html?pdp_npi=4%40dis%21CZK%21CZK%208.90%21CZK%208.90%21%21%210.36%210.36%21%402103891017375828457015857e954c%2112000035688288567%21sh%21CZ%213305825785%21X&spm=a2g0o.store_pc_allItems_or_groupList.new_all_items_2007508297226.1005004629215040

> MK III ideas
- can use DMA & control blocks to do the 256 palette lookup with FB w/o involving the CPU
- when using NiMH batteries, due to very flat discharge curve, the battery voltage reading might not be really useful

> After XMAS, but can be done with RP2040:

- https://www.tme.eu/cz/en/details/ds1002-01-1x16r13/pin-headers/connfly/ds1002-01-1-16r13/ (90 deg cartridge header)

- the SD card initialization routine does not work for SDHC cards, only for SDXC (not a blocker for xmas)

- tud_deinit (already disconnect does) freezes the device, not sure why? - but interestingly if DC is disconnected first, then it all works, even repeatedly - this no longer seems to be the case

- hall of fame and others can be apps of their own that just reuse the canvas from previous run, but delegate the controls? How though

- maybe not use joystick, but keep it as an extra file so that each app can configure it properly, including things like position. But how to integrate with settings? 

- make mp3 player and video as a TV so that the mp3's have daily schedule that you can do

- account for PNG's transparency by allowing it to be overlayed on existing bitmap (right now it is set to black which looks ok on black background)
- maybe even RGBA colors? 
- rumbler & LED effect done event

- recording in fantasy

- libopus decoder & encoder library
- jpeg decoding library

- can swap RAM arenas to disk if necessary

- make timer run a ... b ... a as well (then I can replace the blinking code in clock)

- can have audio player run in the background on second core for most other games, that way people can play & listen to music

## Mk III

- the 3v3 rail to onboard sensors & RTC is always on, this enables time & steps tracking to be valid even across cartridges. The RTC memory (if present) can be used for some basic storage as well 

- can have sinking USB-C https://jlcpcb.com/partdetail/Xunpu-TYPEC_302BRP16SC21/C5760470

### Display woes

> The displays I ordered for the prototypes are 8bit interface, while the larger batch is 16. This in itself would not be the worst, but it seems that the 8bit display can no longer be purchased. I have 2 working prototypes for xmas, so if all goes well, it *might* work still, but the plan for the future is to use the extra pins on RP2350B for the 16bit interface so that I can use the displays I have ordered already. 

> Turns out it actually might work even with the 16bit display and the 16bit resolution can even give faster communication and thus more time available for draw in framebuffer mode. 

## MkIII SDK Revision

> This is the next major rewrite of the SDK, probably will happen after Xmas and is tailored towards the RP2350, although RP2040 should benfit from it as well. 

https://www.tme.eu/cz/en/details/ds1002-01-1x16r13/pin-headers/connfly/ds1002-01-1-16r13/


# mk3 Updates

- check the IOVDD switch is proper and will work
- check that the large resistors on VBATT divider can still work with ADC
- check home button part of the matrix

# mk3 TODO

- terminating resistors on I2S
- should I use the PMIC chips? 
- check radio can output to the audio codec
- footprints for the devices on the RP2350 switching regulator are bad, check when the parts really exist in jlcpcb
- use https://jlcpcb.com/partdetail/skyworks_siliconLabs-SI4705_D60GMR/C2654632 for radio
- can use also SI4703, but that one does not have internal / external antenna - do I really need it? 
- this allows for shorter audio paths
- or just use module? 

