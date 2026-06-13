# Immediate Roadmap

- once I have the prototypes, finish the v1.0 AVR and start experimenting with what I have on the device
- this might be better button: https://jlcpcb.com/partdetail/ALPSALPINE-SKRABCE010/C19724057
- figure out the side buttons if there are ones that can be soldered by jlcpcb and be better centered
- this might be better speaker: https://cz.mouser.com/ProductDetail/Same-Sky/CMS-160903-18S-X8
- buttons can be better printed with circular infill on the top layer & filament change

# Ladder

The plan is to start with a simple game - cat chase - there is a mouse and a cat, mouse moves randomly, cat chases it, controlled by the player. The aim is to program this game at the C++ level to bring all the pieces together and make sure the c++ code looks decent and easy. Once I have this I can start writing the dynamic wrapper for the layers below (blocks & visual) and asset editors. 

So what I need:
- spriteset asset
- sound asset
- sprite object
- sound object
- basic game loop & input, collisions

- very basic descriptors might be working, the code needs commenting, testing.
- descriptor/game object infrastructure for matching arguments of events should be added 

# SDK 1.0

- how to detect we are done playing music? in the DMA

- update the config.h to have backend config and general config to which we put things from rckid.cpp now (key repeat and other UI settings)

- when app is not started and shows error diealog instead, we do double background bump on leaving

- add telemetry (will be useful for the pilots, record time & app start, or end)

- improve value parsing in Reader (bool, ints, etc.)
- how to do heartbeat? (static initializers?)

- for game saves use different bookmark icon (red maybe?)

- avr firmware wakeup timing

- make UNREACHABLE & friends part of platform? This should make it write between mkIII, fantasy and ATTiny stuff

- add wave keyboard effect for rgb
- does the start then drop immediately effect for keyboard press work? 
- what should go to settings & what should go to style?

Wakeups
- alarm clock
- steps daily reset
- piggy bank daily allowance top up check

Settings
- pim (pin, parent mode, etc.)

Basic util apps:
- level
- data sync (mk3 overhaul, sharing only the FAT partition, not the whole MBR)

Games (port from mkII)
- sound in tetris
- sokoban
- space invaders
- pong
- sliding puzzle

- canvas app can be rendered in single call technically

- show header even when not full screen is rendered (home menu & friends)

- deal with wakeup interrupts

- budget reset & the whole wakeup business

- add extra tiles for all volume levels

- mk3 code not tested
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

- make level app draw circle and look prettier, maybe draw angles even

- PNG decoder should change to use heap again as there is more chance to find the space in heap

- rumblerOff does not actually turn the rumbler off
- images for games

- drawing preserves state like games
- allow messages to send images

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
- make the cartridge pcb 1.6mm standard thickness? Basically it would only simplify the jacdac stuff, otherwise unnecessary

- is there a way how to make the headphones work with headphones that have microphone as well? maybe by connecting tip with some large resistor to 0 (68k or so) and then connecting the tip mate via even higher resistor to VCC as a pull up. Then it will read close to 0 when not inserted and VCC when inserted. But will this upset the audio? It actually might work and I can ignore the second sleeve and it would work with all headphones! (can I make it work with current audio setup by rewiring?)
- might get super pretty front panels from here: https://www.hopesens-glass.com/
- need to re-route NRF and loRa boards (pull-up for IR control at least, schematics is correct)

## AVR

- see if we can run at 5MHz and still talk to neopixel

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



## Memory Management

Currently, I have unique_ptr, which is just std::unique_ptr alias. I also have immutable_ptr, which is either pointer to flash memory, or unique ptr to non-flash memory that will get released properly. Only allows immutable access because flash memory. I also have mutable_ptr, which can be either immutable ptr, or unique ptr and can lazily move from immutable to mutable by copying the immutable data from flash to ram. Thus it must know the size of the area it points to.

I am proposing to remove the mutable ptr altogether. Things either explicitly allow mutability via unique_ptr, or they stay immutable forever. The transition from immutability to mutability must happen explicitly and the outside (user) must at this point provide the size. I hope it will be simpler. 

Then I have images, bitmaps, canvases and ImageSources. 

ImageSource is a class that can be used to identify an image. This can be done via (a) path to a file, or (b) memory buffer that contains the image data itself. Both files and memory buffers can be in different formats, namely:

QOI, PNG, JPG, Raw. 

Raw is interesting especially with the immutable_ptr. It is a format that first has the raw data as they would be unpacked, and this is followed by width & height. This allows no allocation for images stored in flash in the raw format. At the moment the raw format only uses RGB565 color representation. I am proposing to add third - last byte specifies the bpp/format and it can be RGB565, or indices (256, 16). Makes sense? 

Finally I have canvas. Canvas at the moment only supports RGB565, but I am thinking maybe it should support multiple formats too. 
