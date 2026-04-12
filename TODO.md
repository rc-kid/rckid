# Ladder

The plan is to start with a simple game - cat chase - there is a mouse and a cat, mouse moves randomly, cat chases it, controlled by the player. The aim is to program this game at the C++ level to bring all the pieces together and make sure the c++ code looks decent and easy. Once I have this I can start writing the dynamic wrapper for the layers below (blocks & visual) and asset editors. 

So what I need:
- spriteset asset
- sound asset
- sprite object
- sound object
- basic game loop & input, collisions

- very basic descriptors might be working, the code needs commenting, testing.

# SDK 1.0

- when app is not started and shows error diealog instead, we do double background bump on leaving

- add telemetry (will be useful for the pilots, record time & app start, or end)

- have some on change handler that gets triggered when widget property changes so that we can react to changes (one is enough AFAIK)

- improve value parsing in Reader (bool, ints, etc.)
- how to do heartbeat? (static initializers?)

- use only absolutely necessary icons in the raw format to avoid extra flash bloat
- for game saves use different bookmark icon (red maybe?)

- apply style to existing widgets (Label, etc.) (...)
- button press repeat (configurable)

- avr firmware wakeup timing

- make UNREACHABLE & friends part of platform? This should make it write between mkIII, fantasy and ATTiny stuff

Widgets
- popup menu
- numpad
- keyboard
- color selector

Settings



Basic util apps:
- level
- piggy bank
- data sync (mk3 overhaul, sharing only the FAT partition, not the whole MBR)
- steps reset (we need wakeups for this)
- alarm in clock (for this we need wakups)

Games (port from mkII)
- sound in tetris
- sokoban
- space invaders
- pong
- sliding puzzle

- show header even when not full screen is rendered (home menu & friends)

- save state of applications on stack when powering off (and other routines)
- deal with wakeup interrupts

- budget reset & the whole wakeup business

- add extra tiles for all volume levels
- pim (pin, parent mode, etc.)

- mk3 code not cleaned up and not working
- heapend on mk3 should return current sp or something like that to verify that we are not growing over


- add support for overclocking, and in the missing adjustment for display pio in the hal layer:

        static void adjustSpeed() {
            pio_sm_set_clock_speed(RCKID_ST7789_PIO, sm_, RCKID_ST7789_SPEED * 4); // 2 cycles per pixel
        }

### Code Cleanup
- should ini reader and writer own the stream? Might simplify things a bit in the API



## Link Capability

- direct connection to other device
- via usb cable/wifi/nrf or other means, when activated user can select which link implementation to use if multiple are possible so that the API is the same and does not care with implementation

# Version 3.2 Checklist

First add a board that can verify the last missing features and DFM improvements, namely:

- [ ] SMT soldered vibration motor
- [ ] ditch radio from main board
- [ ] radio IRQ pin can become RPI TX for debugging
> This means debugging connector is 1 GND, 2 ARM debugging, 1 UPDI, 1 RP tx, 1 AVR tx (6)

These will be tested with a simple testboard:

- [ ] smaller board size for cheaper assembly
- [ ] microphone sensing
- [ ] headphone detection via switches, *not* extra ground link
- [ ] speaker without housing connected via spring contacts
- [ ] battery connectors for nokia-like batteries, etc.
- [ ] rubber dome buttons

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

- turn brightness *on* in avr forever and then try it looks like only problem when not DC connected
- and only every second turn on is a problem

- likely there is an issue with timing & drawing of the lines on GBCemu we have VBLANK first then start drawing line, also songbird does not work well during transitions...

- or maybe add circular scrolling to labels?
- make label scrolling nicer to configure so that inside audio player when refresh rate is 1s we can still do nice things, or can we increase refresh rate? 

- add steps to budget seconds conversion

- make level app draw circle and look prettier, maybe draw angles even

- PNG decoder should change to use heap again as there is more chance to find the space in heap

- rumblerOff does not actually turn the rumbler off
- images for games

- drawing preserves state like games
- allow messages to send images

- rapid fire not working

- keep the offset so that we can go back (in messages)
- check wifi available, etc. 
- Friends::ContactViewer should be own dialog, not part of friends ideally

- add airplane mode & sleep function (also uncomment the options in home menu)

- callback to wifi status
- https://github.com/raspberrypi/pico-examples/blob/master/pico_w/wifi/http_client/example_http_client_util.c

- allow messages to save to existing contact as opposed to always a new one

- add confirmation dialog to many deletions
- why popup menu takes pointer and not a reference? 

- update & processEvents are not really honoured in the ui 

- scrollview scrolling when focused
- update scrollview to be able to scroll horizontally as well, check its update method for scrolling handling, cleanup the code

- for the flashlight, somehow at the beginning there is a faint glow even when turned off, this seems to disappear after a while

- for rumbler in AVR, ignore the values that are below some threshold that is observable on mk3
- allow breathing to start at specified offset in the animation

- clean-up the code around display initialization

- the color settings should be stored, perhaps per game?  (gameboy)
- and when colors are updated, different colors should be update-able at the same time

- some avr commands are longer than 16 bytes, which means it cannot be stored via the I2C async commands

- when stack protection fails, it will fail in the error code as well forever, update stack on device, do new thread on rckid? 

- fix/check device fatal eror & stack protection

- add events for getting icon for files & on file selected for file browser
- add actions for file browser such as rename, copy, mkdir, etc.  

## HW

- https://jlcpcb.com/partdetail/JST-S2B_PH_KL_LF_SN/C160237 (use this for battery connector)
- JST-PH2 batteries from https://www.aliexpress.com/item/1005007102975858.html
- alternatively maybe a daughterboard with pogo pins that will connect to the main board might be the simplest solution
- raise the nylon case 1mm above the glass and round it, maybe 1mm diameter or slightly less? 
- make cartridge cover tiny bit slimmer for that larger pcb
- make the cartridge pcb 1.6mm standard thickness? Basically it would only simplify the jacdac stuff, otherwise unnecessary

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


- actually here: https://www.aliexpress.com/item/1005001515210980.html
- silicone rubber dome to test: https://www.aliexpress.com/item/1005008583229786.html?spm=a2g0o.productlist.main.17.5598382eMHHANJ&algo_pvid=4a0a441a-0334-4e4e-918b-2aa777b6d8ae&algo_exp_id=4a0a441a-0334-4e4e-918b-2aa777b6d8ae-16&pdp_ext_f=%7B%22order%22%3A%222%22%2C%22eval%22%3A%221%22%2C%22fromPage%22%3A%22search%22%7D&pdp_npi=6%40dis%21CZK%21397.26%21397.26%21%21%2118.55%2118.55%21%402103892f17687548884142661e9e70%2112000045830733759%21sea%21CZ%210%21ABX%211%210%21n_tag%3A-29910%3Bd%3A3793dd73%3Bm03_new_user%3A-29895&curPageLogUid=cINl4yvQZdTP&utparam-url=scene%3Asearch%7Cquery_from%3A%7Cx_object_id%3A1005008583229786%7C_p_origin_prod%3A#nav-specification
