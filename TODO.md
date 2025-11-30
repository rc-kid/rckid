# DevBoard Checklist

> X means done, - means cannot be done

- [-] check accelerometer works (using different accelerometer)
- [ ] check SD card insertion detection
- [X] check powered from real battery works
- [-] check USB detection works when powered from battery
- [-] check DataSync app
- [-] check the above still works if we add USB ESD protection
- [ ] disable debug mode on by default on AVR (end of initialize)
- [ ] check that tilting dpad is better 
- [X] mk3 idle and keepalive timers are too high, move lower again
- [ ] add way to clear memory when necessary (e.g. remove background, etc.)
- [ ] draw LoRA cartridge
- [ ] add board for debug connectors

# MkIII Checklist

- [X] verify rumbler
- [X] verify battery operation works 
- [X] add missing AVR features (charging detection, etc.)
- [ ] determine the actual speed & how to set it to 8MHz? Or even 5? 
- [X] buy torx M2x10mm (https://www.nerezka.cz/sroub-m-2-x-10-din-965tx-a2) 
- [X] buy threaded inserts for thermosets (https://www.tme.eu/cz/en/details/b2_bn1054/threaded-insertions/bossard/1386727/)
- [X] buy LEDs with proper forward voltage (https://www.tme.eu/cz/details/l128-4080ca3500001/vykonove-diody-led-emiter/lumileds/)
- [X] order extra batteries (TME)
- [X] order extra enclosures
- [X] order headphone jacks (mouser)
- [X] order AP2502 extras (mouser)

# XMas delivery checklist

- [X] audio player keeps button presses - fix
- [X] repeat function
- [X] automatic next function
- [X] header
  - [X] show budget
  - [X] show time
  - [X] show volume
  - [X] allow showing header even if not part of the renderable area
- [X] fm radio app fixes (display frequency)
- [X] rm radio rds, 
- [X] fm radio signal quality, stereo
- [X] time apps
  - [X] clock settings
  - [X] stopwatch
- [X] improve text input dialog
- [X] screen lock (double home tap)
- [X] preserve volume settings
- [ ] low battery error and device won't power on
- [X] AVR debouncing
- [X] multiple saves for apps

Nice extras would include:

- [X] LED flashlight
- [X] make the LED flashlight more user friendly
- [X] make start negate the state regardless of its value
- [X] shuffle function in audio playback
- [X] birthdays & PIM
- [X] select icon for contacts
- [X] larger font in contact details
- [X] rumbler effects on button presses
- [X] allow background playback
- [X] pin locking
- [ ] parents mode
- [ ] piggy bank
- [ ] owner info
- [X] theme style settings (colors & background) 
- [ ] extra settings (colors, etc. for system & gbcemu)
- [X] settings menu for button lights
- [ ] accelerometer controller background & background selection
- [X] fm radio rds time setting
- [ ] fm radio rds in background
- [X] fm radio embedded antenna (tested)
- [X] fm radio presets
- [X] data sync app
- [ ] RGB dimming that preserves colors (? is possible)
- [ ] timer app
- [ ] alarm app
- [ ] port more games (15 puzzle at least)
- [ ] add high scores
- [ ] app for creating icons (64x64) so that kids can design their own. Eventually this can go larger and larger
- [ ] in header, show volume value, not just bar when changing (?)
- [ ] some way app can show which buttons it reacts to (faint glow for key press, only use the effect on valid keys otherwise)

Must-have before wider release

- [ ] AVR bootloader so that AVR firmware can be updated too
- [ ] telegram messenger
-   [ ] notification
-   [ ] run in background
-   [X] add telegram ids to users
- [ ] webpage, releases, updates, etc.

# TODO

> !!! It does look like the new batteries do *not* have protection circuits in them. To compensate, I can add battery protection circuit to the protection PCB. This could be from BQ2970 and CSD16406, both available from jlcpcb.

- pngenc: https://github.com/bitbank2/PNGenc
- when new conversaton is added and its the first conversation, it is not displayed
- widget does not work well - children owned & not owned mixing is not good
- maybe ditch the ownership? 

- a more polished and reasonable palette for the header
- callback to wifi status
- https://github.com/raspberrypi/pico-examples/blob/master/pico_w/wifi/http_client/example_http_client_util.c

- piggy bank topup-monthly, etc.
- piggy bank data load & save

- parent mode lock
- add parent mode password instead
- parent mode cartridge

- bootloader cartridge (this will only flash the memory)

- move brightness and audio settings from avr status

- for the flashlight, somehow at the beginning there is a faint glow even when turned off, this seems to disappear after a while

- for rumbler in AVR, ignore the values that are below some threshold that is observable on mk3
- icons for rgb light, key and rainbow kay, breathe & solid effects
- allow breathing to start at specified offset in the animation

- make ini writing use writers interface instead of the strings
- would be nice to have some simple way of specifying ini file parsing...

- clean-up the code around display initialization
- document host file system

- maybe have the icon editor use 32x32 and double sized pixels? Will be easier to use and simpler, for now... or wait till we have assets editor proper? 

- add some onPowerOff event that will be called when device decides to power off *and* when avr decides to power the device off as well (so that we can save state, etc)

- would be nice to have menu-like transitions between apps (i.e. multimain menu stuff)

- bg tasks have to be properly closed when some app says it is not compatible with them

- the color settings should be stored, perhaps per game?  (gameboy)
- and when colors are updated, different colors should be update-able at the same time

- some avr commands are longer than 16 bytes, which means it cannot be stored via the I2C async commands

- platform string utils should not be used, instead everything should run off rckid::String
- verify other std::string uses

- add counters, namely how long the redraw of the screen takes, from the beginning of render to the end of render

- critical battery error
- add RGB signalization to the avr mkIII
- avr int as serial tx mabe add 0r resistor on the line to avoid bleeding into rp2350

- improve audio fidelity for GBCEmu

- file browswer can also use the new carouselMenu?

- TODO would be good if image could work immediately with static in memory pictures
- do we need bitmap in graphics now? its more like bitmap is now image really

- allow saving external ram to cartridge when gbc game exits? 

- for messages, add widgets for the various message types that can be viewed

- how to blitting & stuff? (bitmap is multi bpp, while surfaces such as canvas are fixed bpp), this makes blitting harder a bit

- when stack protection fails, it will fail in the error code as well forever, update stack on device, do new thread on rckid? 
- perfect fit strategy seems to be less wasteful, but will have to add fallback
- merging chunks/splitting large ones? 
- add comments to memory 

- ColorRGB is weird, should be colorRGBA and then have Color565 as a version that wraps around 15bpp? Then change uint16_t everywhere in palettes

- comment PNG loading stuff

- add version of background wher the background just bounces so that the background can be some actual image

- fix/check device fatal eror & stack protection
- improve memory allocator (heap)

- add events for getting icon for files & on file selected for file browser
- add actions for file browser such as rename, copy, mkdir, etc.  

## HW

- RM2 cartridges do not connect flash to 3v3(!!)
- side buttons are too thin
- pull-up for the headphone detect is too weak, try sth like 40kOhm? 
- home button can be centered in the hole so that it is a bit higher up
- connector pcb should move the pins as close to edge as possible for better contact (or make cartridges a bit taller, and meybe both)
- is there a way how to make the headphones work with headphones that have microphone as well? maybe by connecting tip with some large resistor to 0 (68k or so) and then connecting the tip mate via even higher resistor to VCC as a pull up. Then it will read close to 0 when not inserted and VCC when inserted. But will this upset the audio? It actually might work and I can ignore the second sleeve and it would work with all headphones! (can I make it work with current audio setup by rewiring?)
- might get super pretty front panels from here: https://www.hopesens-glass.com/

## AVR

- see if we can run at 5MHz and still talk to neopixel

## UI

- proper palette rendering, do offsets
- palettes in the UI stuff (header & dialogs)
- when updating multiple attributes of a widget the recalculate after each one of them is not necessary
- also maybe change the resize to change and make it general method for ui change stuff
- review dialogs & how they are configured and used (custom title, context menu for file dialog, etc.)

## Audio

- add square and white noise waveforms

## Graphics

- should colorRGB be 3 bytes? Or should it be true to the 565 representation? 
- visit loadImage to determine if the setAt is good enough (conversition from int16 to Color to int16 seems inefficient)
- image can have pixel arrays form ROM as well - just ensure on the fantasy that we are not deleting the memory if it comes form "ROM" - sth like lazybitmap? 

- optimize surface functions for common cases
- add specialization for 16 bpp bitmap renderColumn that simply does memcopy

## Apps

### AudioPlayer

- label scrolling
- image picture

### GBCEmu

- tearing is kinda ugly, can be fixed by framebuffer, will cost around 23k, but then scaling & rendering can be done by other core

## Others

- memory leaks via reusing large parts for small items, this way we eventually run out of memory
- waiting for display update done could make the cpu sleep
- serialize & deserialize vs load & save

# PCB Things To Fix

# PCB

> This is updated TODO list for mkIII. It's split into 

- 2.8" 320x240 with fpc: https://www.aliexpress.com/item/1005004629215040.html?pdp_npi=4%40dis%21CZK%21CZK%208.90%21CZK%208.90%21%21%210.36%210.36%21%402103891017375828457015857e954c%2112000035688288567%21sh%21CZ%213305825785%21X&spm=a2g0o.store_pc_allItems_or_groupList.new_all_items_2007508297226.1005004629215040

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
