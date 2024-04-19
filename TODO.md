## Shopping List

- displays from aliexpress (~27x, IPS 2.8")
- buttons (https://cz.mouser.com/ProductDetail/Omron-Electronics/B3U-3000P-B?qs=AO7BQMcsEu4JAdtnbsGArA%3D%3D)
- rumbler (https://cz.mouser.com/ProductDetail/DFRobot/FIT0774?qs=ljCeji4nMDmZzIiC0gR9iA%3D%3D)
- battery (https://www.tme.eu/cz/details/accu-lp503759_cl/akumulatory/cellevia-batteries/l503759/)
- precision headers rckid ()
- precision headers cartridges ()
- screw inserts (https://www.prusa3d.com/product/threaded-inserts-m2-short-100-pcs/)

## TODO

- headphones work, speaker seems not to (always off) - seems bad GND connectiom for PAM8302
- check rumbler
- check audio quality wrt previous version 

- sensors are not read properly (need non-blocking reads so that we can yield)
- carousel won't be happy with empty items (icons, etc.)
- no clearing of items 

- write optimized draw bitmap functions

- make app system apps use 160x120x8bpp for minimal VRAM footprint for system
- determine carousel with smaller footprint too

- steps counter - might need battery to verify
- accelerometer reported temperature

- check that we can launch apps from the carousel

- add menu font
- implement png's transparent color in png load & drawing

- calculate longest distance between yields
- check sleep mode power consumption for pedometer

- microphone tested to work - sample code https://github.com/ArmDeveloperEcosystem/microphone-library-for-pico/tree/main 
- needs integration and lots of amplification it seems 

- create simpler version with 3 AAA batteries for power (https://www.gme.cz/v/1506937/bh431-1a-drzak-baterie-3xaaa). Check if USB can be still used in this mode!!!

## Case

- holes in the top panel for rgb and sensor are on the wrong side
- print the top holder for painting in a bright color so that the paint opacity can be checked
- see if the cartridge insertion mechanism can be made reliable (looks ok)
- check if the slim buttons are actually working reasonably well
- there is still warping when screwed together (Anton suggested heating up a bit and let the tensions equalize, also try larger PCB for v2 to improve rigidity of the frame around it)

## Audio Woes

- 8bit PCM audio is very very noisy - this is probably ok for some synthesized tunes & stuff, but sounds horrible for music. This is not HW problem, but rather quantization error and some such
- 12bit audio seems to be better, works reliably well with 250MHz overclock, but won't work for 8kHz, a resampling and PDM output via PIO would be much better
- use new circuit? - https://datasheets.raspberrypi.com/rp2040/hardware-design-with-rp2040.pdf
- for 12bit sound, no need for extra timer on PWM
- headphone volume is too high, speaker volume is too low - is that still? 
- check https://github.com/TuriSc/RP2040-PWM-DMA-Audio/tree/main and https://github.com/TuriSc/Dodepan for hints perhaps? 

## PCB

- make sure PAM8302 GND connection is strong 
- add extra cartridge risers to the PCB because the cheap castellation process does not always work
- enlarge holes so that they are more solderable
- the cartridge riser mountholes should not connect to gnd for easier solderability
- how to make the oscillator better? The capacitors are already ok-ish for 16.5 pF, but maybe the capcacitance of the traces is wrong? 
- add inrush current limiting resistors to all mosfets!!! (around 1k)
- add ground pads to side buttons (not in EasyEDA library yet, can be added manually after the buttons arrive)

- maybe change charge_en to go to AVR directly thtough the RProg. That way to disable charging we can just let the pin float
- can even be replaced with headphones so that we can use single ADC as the pin is no longer analog required

- charging fluctuates when there is no battery present (not an issue IMO)

## RP

- ensure VRAM is own bank
- disable file & line information for fatal errors

## AVR

- LED speed is too great
- looks like we get WDT reset every now & then with reset & bootloader delays, check watchdog timing
- I2C on RP2040 hangs sometimes, could be bus errors? 
- add some better non-linear interpolation for the breathe effect
- check basic rumbler
- measure the shunt resistor if the calculations are right (?)
- can talk to INA, voltage reads ok, current is wrong
- I2C master can hang up, which is bad (wdt saves us)
- check that setting out high when already high does not glitch

## SDK

> Audio:

- mp3
- opus
- proper tone & music (sine, different audio sample rates, mixer), ADSR tone

> Graphics

- fonts with alpha
- more stuff to the framebuffer
- tiling engine 
- jpeg, Bitmap::loadImage to support jpeg too
> Apps

- some selector app that cycles through options, could this be carousel? 
- cleanup the app stack and make it more natural not relying on calling super for basic events

> FAT32

- do own async FAT32 driver with DMA

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
