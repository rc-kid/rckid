# DevBoard Checklist

> X means done, - means cannot be done

- [-] check accelerometer works (using different accelerometer)
- [ ] check SD card insertion detection
- [-] check powered from real battery works
- [-] check USB detection works when powered from battery
- [-] check DataSync app
- [-] check the above still works if we add USB ESD protection
- [ ] disable debug mode on by default on AVR (end of initialize)
- [ ] check that tilting dpad is better 
- [ ] mk3 idle and keepalive timers are too high, move lower again
- [ ] add way to clear memory when necessary (e.g. remove background, etc.)
- [ ] draw LoRA cartridge
- [ ] add board for debug connectors

# MkIII Checklist

- [ ] verify rumbler
- [ ] verify battery operation works 
- [ ] add missing AVR features (charging detection, etc.)
- [ ] determine the actual speed & how to set it to 8MHz? Or even 5? 

- [X] buy torx M2x10mm (https://www.nerezka.cz/sroub-m-2-x-10-din-965tx-a2) 
- [X] buy threaded inserts for thermosets (https://www.tme.eu/cz/en/details/b2_bn1054/threaded-insertions/bossard/1386727/)
- [X] buy LEDs with proper forward voltage (https://www.tme.eu/cz/details/l128-4080ca3500001/vykonove-diody-led-emiter/lumileds/)
- [X] order extra batteries (TME)
- [ ] order headphone jacks (mouser)
- [ ] order AP2502 extras (mouser)

# TODO

- check power settings on AVR (seems to be completely off)

- RM2 cartridges do not connect flash to 3v3(!!)

- the rapid fire as is is kind of working, but maybe need per buttom settings, etc. 

- switch to INI for data storage in human readable form on the device
- add calendar

- rewrite popup to use ui style, icons, etc.
- and same for text dialog

- add counters, namely how long the redraw of the screen takes, from the beginning of render to the end of render
- also a counter on how long we wait for the display update to finish

- see if we can enable exceptions, what are the runtime & stack costs of them running
- exceptions seem to be worthy, very little cost, mostly in binary size, but stack increases are small-ish

- critical battery error
- add RGB signalization to the avr mkIII
- add interrupt for the charging pin as well when the device is off to detect VUSB insertion and charging enabled/disabled
- avr int as serial tx mabe add 0r resistor on the line to avoid bleeding into rp2350

- improve audio fidelity for GBCEmu
- change logo to petalface:)

- file browswer can also use the new carouselMenu?

- TODO would be good if image could work immediately with static in memory pictures
- do we need bitmap in graphics now? its more like bitmap is now image really


- allow saving external ram to cartridge when gbc game exits? 
- how to resume state when app restarted (automatic)

- for messages, add widgets for the various message types that can be viewed

- how to blitting & stuff? (bitmap is multi bpp, while surfaces such as canvas are fixed bpp), this makes blitting harder a bit

- might get super pretty front panels from here: https://www.hopesens-glass.com/


- audio codec I2C does not work when MCLK is active. This could be because of enormous I2C rise time for SDA & SCL (well over 1500ns, where 300ns is the limit) - there is sth in the bus design as this would suggest 1nF capacitance of the traces

- run at full speed with no vsync waiting to see how much free room there is

- USB connection is not detected
- make usb work in mkIII as well

- when stack protection fails, it will fail in the error code as well forever, update stack on device, do new thread on rckid? 
- perfect fit strategy seems to be less wasteful, but will have to add fallback
- merging chunks/splitting large ones? 
- add comments to memory 

- ColorRGB is weird, should be colorRGBA and then have Color565 as a version that wraps around 15bpp? Then change uint16_t everywhere in palettes

- comment PNG loading stuff

- display budget in header

- file browser should be similar to contacts, i.e. the dialog in launcher menu instead of ui element and separate browser app 

- main menu should also show next birthdays, alarm if any, and oher most current information

- add version of background wher the background just bounces so that the background can be some actual image

- when we do mk3, make sure that when a button press is detected, the idle flag is cleared 
- keepalive when plugged in can be indefinite? 

- clean-up the code around display initialization

- determine correct rumbler settings for ok fail and nudge

- document host file system

- fix/check device fatal eror & stack protection
- improve memory allocator (heap)
- verify other std::string uses

- add events for getting icon for files & on file selected for file browser
- add actions for file browser such as rename, copy, mkdir, etc.  

- make the popupmenu use animations and labels instead of the tilemap

## HW

- SD card hole in case bottom is too low (fixed, check)
- USB hole in case bottom is too low (fixed, check)
- cartridge insertion is too tight (the top part of bottom case) (fixed, check)
- bottom case needs extra plastic to line up perfectly with the pcb (fixed, check)
- side buttons are too thin
- pull-up for the headphone detect is too weak, try sth like 40kOhm? 

## AVRz

- see if we can run at 5MHz and still talk to neopixel

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

# PCB

> This is updated TODO list for mkIII. It's split into 

- 2.8" 320x240 with fpc: https://www.aliexpress.com/item/1005004629215040.html?pdp_npi=4%40dis%21CZK%21CZK%208.90%21CZK%208.90%21%21%210.36%210.36%21%402103891017375828457015857e954c%2112000035688288567%21sh%21CZ%213305825785%21X&spm=a2g0o.store_pc_allItems_or_groupList.new_all_items_2007508297226.1005004629215040

> MK III ideas
- can use DMA & control blocks to do the 256 palette lookup with FB w/o involving the CPU
- when using NiMH batteries, due to very flat discharge curve, the battery voltage reading might not be really useful

> After XMAS, but can be done with RP2040:

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


# Pins after revision

On the AVR side, I need the following:

- 1 charging status
- 1 battery/sys voltage
- 2 i2c
- 1 int to rp
- 7 buttons
- 1 backlight
- 1 rumbler
- 1 rgb enable
- 1 rgb
- 1 accel int
- 1 qspi ss
- 1 vdd en

Because of measuring the VBATT and sensing the charging status, we've lost the AVR_TX pin for debugging, which is a shame - can we recover it? 

Then on RP2350:

- 2 I2C
- 1 AVR IRQ
- 1 Radio IRQ
- 1 Radio Reset
- 16 display data
- 5 disp control rdx, wrx, dcx, csx, te
- 4 SD card (SPI)
- 2 SD card SDIO (???)
- 1 SD card insertion detection
- 8 Cartridge HSTX
- 2 Cartridge Analog/QSPI CE
- 5 I2S to codec MCLK, BCLK, LRCLK, ADC, DAC
- 1 Audio Codec Interrupt

This leaves 3 gpios on the radio if using external clock, 


^- is 49, but we only have 40, so the radio reset via audio chip will put us back in game, but ideally one more for radio clock? 



# Power Measurements

- 50% brightness, radio on 80mA @ 5V
- speaker at default volume 100mA @ 5V
- with extra power, we get at 120mA @5V
- headphones are small, ~82mA
