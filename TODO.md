# Version 3.2 Checklist

First add a board that can verify the last missing features and DFM improvements, namely:

- [ ] microphone sensing
- [ ] headphone detection via switches, *not* extra ground link
- [ ] speaker without housing connected via spring contacts
- [ ] SMT soldered vibration motor
- [ ] battery connectors for nokia-like batteries, etc.
- [ ] smaller board size for cheaper assembly

Software features:



# DevBoard Checklist

> X means done, - means cannot be done

- [ ] check SD card insertion detection
- [ ] draw LoRA cartridge
- [ ] add board for debug connectors

# MkIII Checklist

Nice extras would include:

- [ ] timer app
- [ ] port more games (15 puzzle at least)
- [ ] add high scores

Must-have before wider release

- [ ] AVR bootloader so that AVR firmware can be updated too
- [X] telegram messenger
-   [X] notification
-   [X] preserve unread notification across runs
-   [X] show unread message icon with chat menu
-   [X] delete chats
-   [X] chat names & icons 
-   [X] add contacts
-   [ ] run in background
-   [X] add telegram ids to users
- [ ] webpage, releases, updates, etc.

Polish
- [ ] RGB dimming that preserves colors (? is possible)
- [ ] in header, show volume value, not just bar when changing (?)
- [ ] some way app can show which buttons it reacts to (faint glow for key press, only use the effect on valid keys otherwise)
- [X] accelerometer controller background & background selection

# TODO

- first home button press in heartbeat mode ignored, second one works though
- home menu is hit immediately when powering on from heartbeat menu
- when powered on from heartbeat menu, the automated power off for heartbeat is not cleared it seems

- display power on routine - do no turn brightness in AVR unless debug mode, do not turn brightness in RP in bootloader mode

- likely there is an issue with timing & drawing of the lines on GBCemu we have VBLANK first then start drawing line, also songbird does not work well during transitions...

- or maybe add circular scrolling to labels?
- make label scrolling nicer to configure so that inside audio player when refresh rate is 1s we can still do nice things, or can we increase refresh rate? 

- add steps to budget seconds conversion

- make level app draw circle and look prettier, maybe draw angles even
- reset step count every day (once we have midnight reset stuff)

- PNG decoder should change to use heap again as there is more chance to find the space in heap

- rumblerOff does not actually turn the rumbler off
- images for games
- and use unpacked icons with no heap allocs for the system icons

- drawing preserves state like games
- allow messages to send images

- rapid fire not working
- main menu generation not that well modular with the game menu being extended - add wrappers instead? 

- keep the offset so that we can go back (in messages)
- check wifi available, etc. 
- make more robust
- Friends::ContactViewer should be own dialog, not part of friends ideally

- add airplane mode & sleep function (also uncomment the options in home menu)

- a more polished and reasonable palette for the header
- callback to wifi status
- https://github.com/raspberrypi/pico-examples/blob/master/pico_w/wifi/http_client/example_http_client_util.c

- allow messages to save to existing contact as opposed to always a new one

- add confirmation dialog to many deletions
- why popup menu takes pointer and not a reference? 

- move brightness and audio settings from avr status
- update & processEvents are not really honoured in the ui 

- scrollview scrolling when focused
- update scrollview to be able to scroll horizontally as well, check its update method for scrolling handling, cleanup the code

- for the flashlight, somehow at the beginning there is a faint glow even when turned off, this seems to disappear after a while

- for rumbler in AVR, ignore the values that are below some threshold that is observable on mk3
- allow breathing to start at specified offset in the animation

- make ini writing use writers interface instead of the strings
- would be nice to have some simple way of specifying ini file parsing...

- clean-up the code around display initialization
- document host file system

- would be nice to have menu-like transitions between apps (i.e. multimain menu stuff)

- the color settings should be stored, perhaps per game?  (gameboy)
- and when colors are updated, different colors should be update-able at the same time

- some avr commands are longer than 16 bytes, which means it cannot be stored via the I2C async commands

- platform string utils should not be used, instead everything should run off rckid::String
- verify other std::string uses

- file browswer can also use the new carouselMenu?

- TODO would be good if image could work immediately with static in memory pictures
- do we need bitmap in graphics now? its more like bitmap is now image really

- how to blitting & stuff? (bitmap is multi bpp, while surfaces such as canvas are fixed bpp), this makes blitting harder a bit

- when stack protection fails, it will fail in the error code as well forever, update stack on device, do new thread on rckid? 

- ColorRGB is weird, should be colorRGBA and then have Color565 as a version that wraps around 15bpp? Then change uint16_t everywhere in palettes

- comment PNG loading stuff

- fix/check device fatal eror & stack protection

- add events for getting icon for files & on file selected for file browser
- add actions for file browser such as rename, copy, mkdir, etc.  

## HW

- maybe microphone is reversed, in which case this is easy to check by detecting if brass case is gnd or not
- also can try different microphones (https://jlcpcb.com/partdetail/SAMZO-MIC_4013_GG00/C42371093)

- RM2 cartridges do not connect flash to 3v3(!!)
- side buttons are too thin
- pull-up for the headphone detect is too weak, try sth like 40kOhm? 
- home button can be centered in the hole so that it is a bit higher up
- connector pcb should move the pins as close to edge as possible for better contact (or make cartridges a bit taller, and meybe both)
- is there a way how to make the headphones work with headphones that have microphone as well? maybe by connecting tip with some large resistor to 0 (68k or so) and then connecting the tip mate via even higher resistor to VCC as a pull up. Then it will read close to 0 when not inserted and VCC when inserted. But will this upset the audio? It actually might work and I can ignore the second sleeve and it would work with all headphones! (can I make it work with current audio setup by rewiring?)
- might get super pretty front panels from here: https://www.hopesens-glass.com/
- IR led needs pull-up, not pull-down

## AVR

- see if we can run at 5MHz and still talk to neopixel
- avr int as serial tx mabe add 0r resistor on the line to avoid bleeding into rp2350

## UI

- proper palette rendering, do offsets
- palettes in the UI stuff (header & dialogs)
- when updating multiple attributes of a widget the recalculate after each one of them is not necessary
- also maybe change the resize to change and make it general method for ui change stuff
- review dialogs & how they are configured and used (custom title, context menu for file dialog, etc.)
- isBorder from drawing editors & palette should be part of some common widget (same as border drawing) 

## Audio

- add square and white noise waveforms

## Graphics

- should colorRGB be 3 bytes? Or should it be true to the 565 representation? 
- visit loadImage to determine if the setAt is good enough (conversition from int16 to Color to int16 seems inefficient)
- image can have pixel arrays form ROM as well - just ensure on the fantasy that we are not deleting the memory if it comes form "ROM" - sth like lazybitmap? 

- optimize surface functions for common cases
- add specialization for 16 bpp bitmap renderColumn that simply does memcopy

## Known performance issues

- when clearing drawing buffers with bg color, this is very inefficient and can be done with DMA right after screen is updated

## Apps

### AudioPlayer

- image picture

### GBCEmu

- RTC is not implemented and will not read values well, nor can it write them
- tearing is kinda ugly, can be fixed by framebuffer, will cost around 23k, but then scaling & rendering can be done by other core
- extra settings (colors, etc. for system & gbcemu)
- improve audio fidelity for GBCEmu
- figure out how debug mode for gbcemu should work now

## Others

- waiting for display update done could make the cpu sleep
- serialize & deserialize vs load & save

# PCB Things To Fix

# PCB

> MK III ideas
- can use DMA & control blocks to do the 256 palette lookup with FB w/o involving the CPU
- when using NiMH batteries, due to very flat discharge curve, the battery voltage reading might not be really useful

> After XMAS, but can be done with RP2040:

- the SD card initialization routine does not work for SDHC cards, only for SDXC (not a blocker for xmas)

- hall of fame and others can be apps of their own that just reuse the canvas from previous run, but delegate the controls? How though

- maybe not use joystick, but keep it as an extra file so that each app can configure it properly, including things like position. But how to integrate with settings? 

- maybe even RGBA colors? 
- rumbler & LED effect done event

- recording in fantasy

- libopus decoder & encoder library
- jpeg decoding library

# Power Measurements

> Those are from mk2, keepin them for reference, should verify with mkIII

- 50% brightness, radio on 80mA @ 5V
- speaker at default volume 100mA @ 5V
- with extra power, we get at 120mA @5V
- headphones are small, ~82mA
