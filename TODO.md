## Shopping List

- displays from aliexpress (~27x, IPS 2.8")
- buttons (https://cz.mouser.com/ProductDetail/Omron-Electronics/B3U-3000P-B?qs=AO7BQMcsEu4JAdtnbsGArA%3D%3D)
- rumbler (https://cz.mouser.com/ProductDetail/DFRobot/FIT0774?qs=ljCeji4nMDmZzIiC0gR9iA%3D%3D)
- battery (https://www.tme.eu/cz/details/accu-lp503759_cl/akumulatory/cellevia-batteries/l503759/)
- precision headers rckid ()
- precision headers cartridges ()
- screw inserts (https://www.prusa3d.com/product/threaded-inserts-m2-short-100-pcs/)

## TODO

- make canvas similar api wrt bitmap
- same for framebuffer
- write optimized draw bitmap functions

- make app system apps use 160x120x8bpp for minimal VRAM footprint for system
- determine carousel with smaller footprint too

- check UV light detector

- display does not work when overclocked (rest works ok)

- steps counter - might need battery to verify
- accelerometer reported temperature

- check that we can launch apps from the carousel

- add menu font
- implement png's transparent color in png load & drawing

- calculate longest distance between yields
- check sleep mode power consumption for pedometer
- framebuffer clear with DMA right after drawing is done, or in parallel with drawing if requested

- microphone tested to work - sample code https://github.com/ArmDeveloperEcosystem/microphone-library-for-pico/tree/main 
- needs integration and lots of amplification it seems 

- create simpler version with 3 AAA batteries for power (https://www.gme.cz/v/1506937/bh431-1a-drzak-baterie-3xaaa). Check if USB can be still used in this mode!!!

## Case

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

- verify everything before ordering new versions

## RP

- document ST7789
- disable file & line information for fatal errors
- print fatal error to USB serial
- SWAP MIC & PWM and mic CLK and DATA !!!!!

## AVR

- changed to ATTiny3217, new pinout and options

- on AVR ABXY and BTN2 pins are swapped
- DPad buttons are swapped
- on wakeup sequence, check that start & sel are pressed too to enter debug mode(s)
- add battery critical power off
- add neopixel & rumbler effects
- add I2C commands

## SDK

> Audio:

- mp3
- opus
- proper tone & music (sine, different audio sample rates, mixer), ADSR tone

> Graphics

- fonts with alpha
- more stuff to the framebuffer
- tiling engine 

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

> Make the 320x240x8 the default framebuffer for menu & stuff. Support partial screen updates? Would be nice for things like menu so that we can keep the partial framebuffer of the game or what not visible-ish, but hard to do with menu in the bottom & header in the top (can also refresh header only when necessary at the expense of some tearing, etc. ) 

> the app hierarchy does no recycle renderers, might keep the allocated VRAM, or maybe use static regions for the VRAM? later - but how to size VRAM? The cartridge can choose for sure

> can put the extra buffers & palette and friends to the scrach memories 

Tile = bitmap with statically given dimensions (really only a storage format)
Bitmap = bitmap with dynamically known dimensions 
Canvas = bitmap with extra state

Support 16,8,4,2 and 1 bitdepth

- pngdec does not use malloc
- libhelix does (but can be patched imo)

- 40mA rpi on + avr (6mA)
- 56mA rpi overclocked?
- 5ohm for the backlight (4R7?)
