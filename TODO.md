# DevBoard Checklist

> X means cone, - means cannot be done

- [X] verify there are no shorts between VCC, VBATT, VDD, 3V3 and ground
- [X] add jumper wires for D and audio power (! these *must* be installed before the board is powered)
- [X] add connectors for battery
- [X] try powering up through battery connector at 4.0V
- [X] try powering up through USB at 5 volts
- [-] try powering up through battery at 3.0 volts to see if the PMIC keeps the VCC above threshold 
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
- [X] device AVR test rumbler -- it runs off IOVDD, has to be checked after IOVDD en 
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
- [X] check NAU88C22 basic communication
- [X] check MCLK generation
- [X] check NAU88C22 radio aux to headphones (with 100uF cap ok)
- [X] check NAU88C22 radio aux to speaker
- [X] speaker works in BTL but there are some hiccups present, power issues, need caps next to the codec
- [X] check NAU88C22 power consumption with idle, speaker and headphones outputs
- [X] check NAU88C22 radio aux to ADC to DAC to headphones & speaker (only when I2S in master mode)
- [X] check NAU88C22 DAC output
- [X] check NAU88C22 ADC input (radio) - seems to work, have some trouble writing data fast enough in raw mode even at 8000kHz, but the I2S comms work - its also quite faint, but the adc boost is not running at 100%
- [X] check NAU88C22 ADC input with PGA (microphone) - but not with the one on board (?)
- [X] check NAU88C22 headphone detection - but makes very noisy fm radio
- [X] check NAU88C22 works in smaller capacitance settings for I2C, or with the low pass filters as per datasheet installed
- [X] check Si4705 works with headphones antenna
- [X] check the headphone outputs while using the antenna
- [X] check Si4705 works with internal antenna (seems to work, but not very well)
- [-] check accelerometer works (using different accelerometer)
- [ ] check SD card insertion detection
- [-] check powered from real battery works
- [-] check USB detection works when powered from battery
- [-] check DataSync app
- [-] check the above still works if we add USB ESD protection
- [ ] disable debug mode on by default on AVR (end of initialize)
- [X] charging/discharging - will use MCP charger from mkII
- [X] check AVR can read battery voltage with large resistors (220 and 330k seems to work, add 10 or 100nf cap to the ADC pin to ground)
- [X] check that 2 LEDs under DPAD sides work ok (HW.md)
- [ ] check that tilting dpad is better 
- [X] check new case works better with battery
- [ ] mk3 idle and keepalive timers are too high, move lower again
- [X] check overclocking -- seems to go to 250MHz w/o overvolt
- [ ] add way to clear memory when necessary (e.g. remove background, etc.)
- [X] verify resistor for backlight - using AP2502 in the end
- [ ] verify on forums the Si4705 headset antenna ESD & filters
- [ ] verify on forums the USB ESD protection
- [ ] verify on forums about the I2S termination resistors and their placement
- [X] should there be ground under switches? (no)
- [X] verify rumbler position & wire length
- [X] draw RM2 cartridge
- [ ] draw LoRA cartridge
- [X] draw NRF24l01P cartridge
- [X] make room for embedded antenna on device case
- [X] add board for battery connector
- [ ] add board for debug connectors
- [X] add holes in case for debug connectors
- [X] verify glass top has cutouts for antenna
- [X] add fused cartridge for resin printing
- [X] add fused buttons for resin printing
- [X] IR LED (via DMP1045u as it sources over 20mA)
- [X] white LED (can be wired via AP2502 which I can get from TME)

## Software Tasks

White LED from TME:

https://www.tme.eu/cz/details/hl-as2835-3cpct-ww/vykonove-diody-led-emiter/hongli-zhihui/hl-as-2835h466w-3c-s1-08l-pct-hr3/
https://www.tme.eu/cz/details/ap2502ktr-g1/stabilizator-napeti-obvody-dc-dc/diodes-incorporated/

IR LED from TME:
https://www.tme.eu/cz/details/ir204c_h16_l10/infracervene-diody-led/everlight/


# TODO

- rewrite serialization to have a function that can be overloaded as frind

- improve audio fidelity for GBCEmu
- change logo to petalface:)

- file browswer can also use the new carouselMenu?

- TODO would be good if image could work immediately with static in memory pictures
- do we need bitmap in graphics now? its more like bitmap is now image really


- allow saving external ram to cartridge when gbc game exits? 
- how to resume state when app restarted (automatic)
- do not allow to start measured app if we have no more allowance (otherwise it starts for a second, then stops)

- for messages, add widgets for the various message types that can be viewed

- how to blitting & stuff? (bitmap is multi bpp, while surfaces such as canvas are fixed bpp), this makes blitting harder a bit

- might get super pretty front panels from here: https://www.hopesens-glass.com/

- see if we can enable exceptions, what are the runtime & stack costs of them running

- can I create SD card SPI in PIO so that I can use SDIO and SPI as well? yes

- I2S playback working. Tone does not seem to work well (glitch data in square waveform, and maybe others). It also drops framerate to 30 when enabled which is weird
- for the input / output, it might be easier to use different format, such as PCM as it would allow to send whole 32bits in one loop

- audio codec I2C does not work when MCLK is active. This could be because of enormous I2C rise time for SDA & SCL (well over 1500ns, where 300ns is the limit) - there is sth in the bus design as this would suggest 1nF capacitance of the traces

- run at full speed with no vsycn waiting to see how much free room there is

- USB connection is not detected
- make usb work in mkIII as well
- can the SD initialization routine be improved? 

- when stack protection fails, it will fail in the error code as well forever, update stack on device, do new thread on rckid? 
- perfect fit strategy seems to be less wasteful, but will have to add fallback
- merging chunks/splitting large ones? 
- add comments to memory 

- ColorRGB is weird, should be colorRGBA and then have Color565 as a version that wraps around 15bpp? Then change uint16_t everywhere in palettes

- comment PNG loading stuff

- figure out what to do with app when budget dies (maybe just call its save if applicable)
- display budget in header

- file browser should be similar to contacts, i.e. the dialog in launcher menu instead of ui element and separate browser app 

- main menu should also show next birthdays, alarm if any, and oher most current information

- add version of background wher the background just bounces so that the background can be some actual image

- todo figure out how context menu can be planted to apps (or perhaps let the dialogs handle them)

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
- add icons for stopwatch, timer, audio Images and utilities


## HW

- buy header pins (plenty for the connectors)

## AVR

- see if we can run at 5MHz and still talk to neopixel
- I2C master enumeration works, but read register does not - is this true still? 
- PWR_INT is now to read the battery level (vcc really) via 220k and 330k voltage divider with 10 or 100nf capacitor from the ADC pin to ground, fix the ADC readout section

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

# PCB

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
