# TODO

- create a string class that allows storing char const * or std::string like, something like owning string_view

- how to deal with non-16 bpp bitmaps 

- serialize & deserialize vs load & save

- image can have pixel arrays form ROM as well - just ensure on the fantasy that we are not deleting the memory if it comes form "ROM"
- allow bitmaps to render with transparent color
- when updating multiple attributes of a widget the recalculate after each one of them is not necessary
- also maybe change the resize to change and make it general method for ui change stuff

- add rgb and rumbler effects to the SDK (commented out in mkII)

- ensure that buffers use allocators properly when they are created to construct stuff
- check audio playback, check the documentation for audio playback

- fix platform for PC and attiny acording to the RP2040 platform
- waiting for display update done could make the cpu sleep

- update the DataSync app so that it actually works as intended

- should audio volume be uint8, or something else? 
- add specialization for 16 bpp bitmap renderColumn that simply does memcopy

# DevBoard PCB & first batch

- [X] order the cartridge connector pcb (populated)
- [X] order the cartridge pcb (unpopulated)
- [ ] order the devboard (populated, x2)
- [ ] create display riser
- [X] order the displays (test batch first)
- [ ] pcb board for the battery connector - can be inside the speaker hole perhaps

# PCB

- R125 (ilim for PMIC is wrong value)
- verify with prototype that R124 (PWR_INT pullup is needed)

> This is updated TODO list for mkIII. It's split into 

- 2.8" 320x240 with fpc: https://www.aliexpress.com/item/1005004629215040.html?pdp_npi=4%40dis%21CZK%21CZK%208.90%21CZK%208.90%21%21%210.36%210.36%21%402103891017375828457015857e954c%2112000035688288567%21sh%21CZ%213305825785%21X&spm=a2g0o.store_pc_allItems_or_groupList.new_all_items_2007508297226.1005004629215040

- 3.9" 320x320 https://www.aliexpress.com/item/1005006170307995.html?pdp_npi=4%40dis%21CZK%21CZK%20299.00%21CZK%20299.00%21%21%2112.09%2112.09%21%40211b819117375814797795352e5cdc%2112000036095035873%21sh%21CZ%213305825785%21X&spm=a2g0o.store_pc_allItems_or_groupList.new_all_items_2007508297226.1005006170307995

> MK III ideas
- buy the RP recommended crystal at jlcpcb
- check if I can solder the inductor myself (might be just able to)
- test the 90 degree cartridge connector feasibility
- better sound driver (PCM5102) for fully 16bit sound
- simplify HW - no light sensor, no notification LED. This makes for smaller PCB, fewer parts. Arguably can keep the light detector and put it under the display, 
- can use DMA & control blocks to do the 256 palette lookup with FB w/o involving the CPU


- when using NiMH batteries, due to very flat discharge curve, the battery voltage reading might not be really useful

> After XMAS, but can be done with RP2040:

- https://www.tme.eu/cz/en/details/ds1002-01-1x16r13/pin-headers/connfly/ds1002-01-1-16r13/ (90 deg cartridge header)

- see if we can have simpler cartridge connector w/o the riser board

- I need a much better way to connect the battery in v3 - soldering and then stuffing it in the enclosure is beyond pain

- generate assets does not generate ui tiles yet - and the ones we have are a bit off
- make text input better looking and the whole idea of modal apps nicer to work with

- the SD card initialization routine does not work for SDHC cards, only for SDXC (not a blocker for xmas)

- add fantasy mode where cartridge and SD card are folders on local drive for simplicity

- document stack protection check
 
- add the screen off mode as part of the API 

- add file browser as own widget

- on fantasy malloc & free replacement is not complete - causes trouble especially with strings
- STR does not work with MemoryArena, only on heap (see Data Sync)

- trace arena is broken as it creates new arena for the tracking, which traces itself:)
- memory leaks in fantasy where we have pointers that are in fantasy heap, but not on arena or heap (see raylib.cpp feee() method)

- tud_deinit (already disconnect does) freezes the device, not sure why? - but interestingly if DC is disconnected first, then it all works, even repeatedly - this no longer seems to be the case
- maybe move platform to sdk?

- hall of fame and others can be apps of their own that just reuse the canvas from previous run, but delegate the controls? How though

- add blit that ignores particular color

- maybe not use joystick, but keep it as an extra file so that each app can configure it properly, including things like position. But how to integrate with settings? 

- clean music & tone generators, ensure other frequencies work as well, document, benchmark

- move games & apps to SDK, unless they belong in cartridges 

- make mp3 player and video as a TV so that the mp3's have daily schedule that you can do

- update the order of args in drawing methods (pos, color) etc

- add blocking send & test it

- account for PNG's transparency by allowing it to be overlayed on existing bitmap (right now it is set to black which looks ok on black background)
- maybe even RGBA colors? 
- rumbler & LED effect done event

- recording in fantasy

- libopus decoder & encoder library
- jpeg decoding library

- can swap RAM arenas to disk if necessary

- make timer run a ... b ... a as well (then I can replace the blinking code in clock)

- can have audio player run in the background on second core for most other games, that way people can play & listen to music

## Mk II Hardware

- I2C problems might be somewhat mitigated by extra pullups in the cartridge, or add a special mode where if DC power is enabled we stop querying and just display that we have entered charging and only stop when charging is stopped? - as a really ugly hack, but will work 

- does not work with 5V from laptop

## Mk III

> on RP2350 we have more memory, so maybe ditch the arena memory allocator and just have full framebuffer form 320x240 allocated as graphics memory at the beginning forever. On 2350 this is some 30% of RAM which is ok. This should greatly simplify the rendering pipeline.

> With ability to read from the display uninterrupted, we can also reinitialize this any time with the existing framebuffer for all kinds of cool effects.

- the 3v3 rail to onboard sensors & RTC is always on, this enables time & steps tracking to be valid even across cartridges. The RTC memory (if present) can be used for some basic storage as well 
- maybe put SD card under the display, together with RPI, this will save enormous amount of space on the PCB around the buttons for rather small increase in total height, which is probably ok
- have version with both solderable contants and FPC for the display?  

- can have sinking USB-C https://jlcpcb.com/partdetail/Xunpu-TYPEC_302BRP16SC21/C5760470
- I can technically solder headphone jack 

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
