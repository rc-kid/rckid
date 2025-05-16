# TODO

- document host file system
- main menu mem efficiency can be improved by having 

- fix/check device fatal eror & stack protection
- check why device allocates every tick? DOes not seem to be display driver
- improve memory allocator (heap)
- verify other std::string uses

- add events for getting icon for files & on file selected for file browser
- add actions for file browser such as rename, copy, mkdir, etc.  
- rename filesystem to fs?

## HW

- connector height could be 2.2mm (with cartridge pcb height of 1.2mm this gives us 1mm width, which is within the 0.8 - 1.4mm working range for the spring contact)
- could this be better PMIC? https://www.ti.com/product/BQ25628
- slimmer connector https://jlcpcb.com/partdetail/XkbConnection-X05A10H40G/C528037
- order breakouts for the improved PMIC

## AVR

- see if we can run at 5MHz and still talk to neopixel
- verify I2C master works

## UI

- switch system tiles to using color16 instead of color256
- proper palette rendering, do offsets
- palettes in the UI stuff (header & textDialog)
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

- update the DataSync app so that it actually works as intended

## Others

- memory leaks via reusing large parts for small items, this way we eventually run out of memory
- waiting for display update done could make the cpu sleep
- serialize & deserialize vs load & save

# PCB Things To Fix

- [X] RP2350 does not have RUN pin connected (!!)
- [X] VREG_VIN should go to IOVDD
- [X] ADC_AVDD should to to IOVDD (?)
- [ ] test ADC_AVDD to IOVDD connection and if necessary


# AVR Breadboard tests

- [ ] create version that uses home button to go to deep sleep and wakes up when home button pressed (to be flashed on device)
- [X] add system notification 
- [ ] verify rumbler & rgb effects
- [ ] create version that uses home button to power VDD on or off (to be flashed)
- [X] verify that UPDI works when AVR powered with 3v3
- [X] switch to external osc for RTC on the devboard

# Devboard Power-Up Sequence

- [X] verify there are no shorts between VCC, VBATT, VDD, 3V3 and ground
- [X] add jumper wires for D and audio power (! these *must* be installed before the board is powered)
- [X] add connectors for battery
- [X] try powering up through battery
- [-] see if the PMIC keeps the VCC above threshold - what is the default value? - IT IS NOT - default settings?
- [X] try powering up via USB
- [ ] flash AVR that does not enable VDD, but can go to sleep and meausre the current consumption at sleep & power on
- [ ] enumerate I2C devices without VDD enabled
- [ ] flash AVR that can enable VDD and test
- [ ] verify that RP2350 will show itself as USB drive 
- [ ] enumerate I2C devices in power on mode (still via AVR)
- [ ] verify cartridge connector tolerances and no shorts present
- [ ] try flashing RP2350 with basic program that outputs on serial USB
- ...


# DevBoard PCB & first batch

- [X] order the cartridge connector pcb (populated)
- [X] order the cartridge pcb (unpopulated)
- [X] order the devboard (populated, x2)
- [ ] create display riser
- [X] order the displays (test batch first)
- [ ] pcb board for the battery connector - can be inside the speaker hole perhaps


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
