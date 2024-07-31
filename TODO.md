## Shopping List

- displays from aliexpress (~27x, IPS 2.8", https://www.aliexpress.com/item/1005004635814413.html?spm=a2g0o.order_list.order_list_main.5.10891802IbL7I1)
- buttons (https://cz.mouser.com/ProductDetail/Omron-Electronics/B3U-3000P-B?qs=AO7BQMcsEu4JAdtnbsGArA%3D%3D)
- rumbler (https://cz.mouser.com/ProductDetail/DFRobot/FIT0774?qs=ljCeji4nMDmZzIiC0gR9iA%3D%3D)
- battery (https://www.tme.eu/cz/details/accu-lp503759_cl/akumulatory/cellevia-batteries/l503759/)
- precision headers rckid (gme824-021)
- precision headers cartridges (gme 832-443)
- screw inserts (https://www.prusa3d.com/product/threaded-inserts-m2-short-100-pcs/)

## Release 0.9

> First release, should meet the minimal usability, i.e. messaging app via telegram (ideally radio) and a few games. HW should be finished.

- basic settings UI (brightness, volume, etc.)
- basic file access on the SD card
- order displays
- order hardware revision (5x or 10x ?)
- order batteries (5x-10x ?)

- app that sets keyboard lights, etc. 

- extra games can be hangman, battleships, calculator (?)

## TODO

- I2C woes are related to the fact that AVR slept (some errors with timers still even now that sleep is disabled in normal modes)
- I2C often fails on timeouts *and* fails on timeouts (not sure what that is any more)
- and then in the IRQ fails on 0x40 cause (TX abort), from that point SDA is low and the stuff hangs
- often hangs on startup when the W43 is what keeps the line low
- the data sent seems to be often mangled, i.e. status bytes are sent, but offset is wrong, i.e.:

AVR: 000011ea74ff8000
AVR: 0000ffffffffffff
AVR: 000031ea756d8000
AVR: 000031ea756d8000
AVR: 000031e875ff8000
AVR: 000031e875ff8000
AVR: e875ff8000010000
AVR: 000031e8776d8000
AVR: 6d80000100000001
AVR: 000011e7786d8000
AVR: 000011e67c6e8000
AVR: 0000ffffffffffff
AVR: 000031e67eff8000
AVR: 000031e67dff8000
AVR: 000031e67dff8000
AVR: 000031e67dff8000
AVR: 000031e67dff8000
AVR: 000031e67dff8000
AVR: 000031e67dff8000
AVR: 000031e67cff8000
AVR: 000031e67cff8000
AVR: 000031e57cff8000
AVR: 000031e47dff8000
AVR: 000031e37dff8000
AVR: 000011ffffffffff
AVR: 000031e17d6d8000
AVR: 000031e17d6d8000
AVR: 000031e17dff8000
AVR: 000011e17dff8000
AVR: 000011e17dff8000
AVR: 000011e0ffffffff
AVR: 000031e17e6d8000
AVR: 000031e17d6d8000
AVR: 7d6d800001000000
AVR: 000031df7f6d8000
AVR: 000011dd7f6d8000
AVR: 000011dd7f6d8000
AVR: 000031dd816d8000
AVR: 000011dd806d8000
AVR: 000011dd806d8000
AVR: 000031dd806d8000
AVR: 000031dd80ff8000
AVR: 000031dd7fff8000
AVR: 7fff800001000000
AVR: 000031db80ff8000
AVR: 000011db7fff8000
AVR: 000011db7f6c8000
AVR: 000031db806d8000
AVR: 000031d980ff8000
AVR: 000031d97fff8000
AVR: 000031d97fff8000
AVR: 000031d97fff8000
AVR: 000031d77fff8000
AVR: 000031d77eff8000
AVR: d77dff8000010000
AVR: 000011d77dff8000
AVR: 000011d77cff8000
AVR: 000031d77cff8000
AVR: 000031d77bff8000
AVR: 000031d77bff8000
AVR: 7bff800001000000
ERR:40

- somehow the disableCharging is called too often (now disabled in code)

- add buffer for writing system messages (1kb might be sufficient) - this will help with I2C and stuff

- test HTTP and HTTPS clients
- add internet connections
- make the cartridge silent wrt log


- the double radio methods should not be a problem
- move controller & radio & stuff to the sdk? likely yes

- add cartridge settings app

- add commands, such as the GET, etc. then make the curl stuff work-ish? 

- there seems to be some NRF issue with timing (0.5Mhz for SPI communication mostly does not work, while 10Mhz seems to work reliably)
- could it be that I am using the single byte transfer?
- make sure that multiple connection requests for the same connection will be ignored if the connection is already listed as open (and fake accept message will be returned) - because messages can cross
- have even and odd message sends so that the same data sent multiple times can be deduplicated when ACK is lost

- connections API is in progress, how to send messages? via buffer and clear them on IRQ? retransmits? Maybe a buffer, if won't fit in the buffer, not that much harm done - can always retry at later time
- add timeouts to to connections
- seems I can use ESB for the NRF24L01p, at least between base station & rckid devices and rckid device themselves (for walkie talkie, we are still just flooding)

- do header icons & information 

- do stream from memory that can be used by PNG and other things as well so that all works on streams

- update how the transparency is handled in sprite drawing 
- static sprites

- when dc power plugged in (and maybe charging enabled) weird button presses are registered

- better stabilization for the voltage gauge (avg from N measurements + hysteresis maybe?)

- add message & alarm to symbol glyphs
- dynamic menu (for say file listing, etc.)

- make malloc work in mock
- make audio work in mock

- what is the allocation during startup - see how I can debug and if it poses a problem - https://raspberry-projects.com/pi/microcontrollers/programming-debugging-devices/debugging-using-another-pico

- 2x 256 framebuffer does not render correctly and likely the code is wrong

- write optimized draw bitmap functions - rewrite the bitmap to use drawing.h as much as possible. Also rewrite the asm functions to be in line with routines.cpp & rckid.h

- steps counter - might need battery to verify
- accelerometer reported temperature

- implement png's transparent color in png load & drawing

- text UI should use only 8 color in tiles shading wise and use the rest for effects. Add effects and colors to it 

- microphone tested to work - sample code https://github.com/ArmDeveloperEcosystem/microphone-library-for-pico/tree/main 
- needs integration and lots of amplification it seems 

- create simpler version with 3 AAA batteries for power (https://www.gme.cz/v/1506937/bh431-1a-drzak-baterie-3xaaa). Check if USB can be still used in this mode!!!

## Case

- fix the rumbler hole position in bottom and midframe
- check that cable guides for battery are big enough
- holes in the top panel for rgb and sensor are on the wrong side
- there is still warping when screwed together (Anton suggested heating up a bit and let the tensions equalize, also try larger PCB for v2 to improve rigidity of the frame around it)

## Audio Woes

- have basic tone & envelope check, the sound quality is not stellar, but actually quite ok
- need programmable amplitude & ADSR in milliseconds updated by sample rate
- sample rate other than 44100 is not that great (because of PWM?)
- sigma-delta DAC might be better? - at the cost of extra CPU though
- the tone generator can be much optimized - interpolator?
- add audio mixer
- microphone works-ish but is super noisy - maybe better PDM to PCM filter than the simple sum? - https://tomverbeure.github.io/2020/10/04/PDM-Microphones-and-Sigma-Delta-Conversion.html, https://github.com/ArmDeveloperEcosystem/microphone-library-for-pico

- 8bit PCM audio is very very noisy - this is probably ok for some synthesized tunes & stuff, but sounds horrible for music. This is not HW problem, but rather quantization error and some such
- 12bit audio seems to be better, works reliably well with 250MHz overclock, but won't work for 8kHz, a resampling and PDM output via PIO would be much better
- use new circuit? - https://datasheets.raspberrypi.com/rp2040/hardware-design-with-rp2040.pdf
- for 12bit sound, no need for extra timer on PWM
- headphone volume is just right
- speaker volume too low - AIN- should be connected to GND, figure out resistor divider between AIN+ and AIN-
- check https://github.com/TuriSc/RP2040-PWM-DMA-Audio/tree/main and https://github.com/TuriSc/Dodepan for hints perhaps? 

## SD Card 

- usb msc works, but is terribly slow, could it be that we don't do event checking when redrawing much? 
- no DMA & IRQ mode and how to deal with busy ? 
- the code needs to be much polished

## PCB

- can do color pictures on the new cartridge boards maybe
- ESP cartridge has pin 0 & 2 swapped
- also move to ESP8285, which is much cheaper and a bit smaller
- the freed SPI pins can be used for extra things such as torchlight and TV remote? 
- still have 2 pins left

- how to make the oscillator better? The capacitors are already ok-ish for 16.5 pF, but maybe the capcacitance of the traces is wrong? 

## RP

- USB host cannot be run twice because tud init can only be executed once. See comment in USBMassStorage.cpp, might need to patch the library, or maybe use in host & device mode so that we actually have weak pulldowns and not pullups on data lines? (is this true?)


## NRF Cartridge

- 0.5MHz does not seem to work, 10MHz SPI communication seems to be better
- no clue why, generally first message works, then it breaks
- all NRF modules I have but the green ones seem to be ESB compatible

## ESP8266 Cartridge



## AVR

- the I2C master (or something) keeps WDT timeout on AVR, onl when RPI is on
- verify the Headphones/Charge EN pin swap and charge en circuit
- AVR EEPROM can store basic info such as username, etc and a password to lock the device
- turn off if INA senses too much of a current draw
- LED speed is too great -- reduced to 30fps, maybe still too great? Add delay to effect? -- is it still? 
- add some better non-linear interpolation for the breathe effect
- write I2C bootloader (entered via writing EEPROM, because EEPROM will last loner than the flash)

## SDK

> Audio:

- mp3
- opus
- proper tone & music (sine, different audio sample rates, mixer), ADSR tone

> Graphics

- more stuff to the framebuffer
- tiling engine 
- jpeg, Bitmap::loadImage to support jpeg too

## Shopping List

- buttons (https://www.tme.eu/cz/details/b3u-3000p/mikrospinace-tact/omron-ocb/b3u3000p/)
- battery (https://www.tme.eu/cz/details/accu-lp503759_cl/akumulatory/cellevia-batteries/l503759/)
- rumbler (maybe this - https://www.tme.eu/cz/details/oky3170/dc-motory/okystar/)
- precission headers ()
- screw inserts (https://www.prusa3d.com/product/threaded-inserts-m2-short-100-pcs/)

## Extra Cartridges

- simple cartridge (only FLASH)
- add SRAM (maybe https://cz.mouser.com/ProductDetail/ISSI/IS66WVS2M8BLL-104NLI?qs=doiCPypUmgFx786bHGqGiQ%3D%3D)

# Graphics

- have 16bpp, 8bpp, 4bpp and 2bpp for font, sprites, tiles and framebuffers


- 320x240x8 framebuffer with full resolution (~80Kb with palette & transfer buffers)
- 320x240x16 framebuffer can be selected too (~153.6Kb)
- 8x8 tiling engine 16bit depth (~40Kb)
- 8x8 tiling engine 8bit depth (~20Kb)
- 160x120x16 half resolution framebuffer (~38.4Kb)

Tile = bitmap with statically given dimensions (really only a storage format)
Bitmap = bitmap with dynamically known dimensions 
Canvas = bitmap with extra state
Sprite = bitmap with position & stuff


Support 16,8,4,2 and 1 bitdepth

- pngdec does not use malloc
- libhelix does (but can be patched imo)

- 40mA rpi on + avr (6mA)
- 56mA rpi overclocked?
- 5ohm for the backlight (4R7?)

- not quite because palette indices need *2 and uses one too many registers 

~ 12500 cycles is max budget for 40 fps
- 6250 is max budget for 60 fps

- do first layer = read tile, write tile ~ 240 cycles
- do second layer = worst case : read 1, test, branch, load, write

    - tilerow index
    - no of tiles
    - 



; 36 cycles worst case -- 39 41 
; 20 cycles best case

    2 LDW L3 ; background
    2 LDW L2 ; mid
    2 LDW L1 ; foreground  -- 6

    1 MOVS mask, 0xff  -- 7 
    1 AND x, L1, mask
    1 BNZ bit2 ; if fg is not zero, we are done
    1 AND x, L2, mask
    1 BNZ bit1set ; if middle is not zero we will use that
    1 AND x, L3, mask
bit1set:
    1 OR L1, L1, x

bit2:
    1 LSL mask, mask, 8
    1 AND x, L1, mask
    1 BNZ bit3
    1 AND x, L2, mask
    1 BNZ bit2set
    1 AND x, L3, mask
bit2set:
    1 OR L1, L1, x

bit3:
    1 LSL mask, mask, 8
    1 AND x, L1, mask
    1 BNZ bit4
    1 AND x, L2, mask
    1 BNZ bit3set
    1 AND x, L3, mask
bit3set:
    1 OR L1, L1, x
  
bit4:
    1 LSL mask, mask, 8
    1 AND x, L1, mask
    1 BNZ store
    1 AND x, L2, mask
    1 BNZ bit4set
    1 AND x, L3, mask
bit4set:
    1 OR L1, L1, x

store:
    2 STM out!, L1


Worst case: 58 cycles for 4 pixels
Best case: 34 cycles for 4 pixels

2 LDRB x [l3 + N]
1 TST x
1 BNZ ready
2 LDRB x [l2 + N]
1 TST x
1 BNZ ready
2 LDRB x, [l1 + N]
  ready:
1 lsls x, 1
2 LDRW x [palette + x]
1 LSL x, x, 16

2 LDRB y [l3 + N]
1 TST y
1 BNZ ready
2 LDRB y [l2 + N]
1 TST y
2 BNZ ready
2 LDRB y, [l1 + N]
  ready:
1 lsls y, 1
2 LDRW y [palette + y]
1 AND x, x, y
2 STM out! {x}

