## TODO

- 256 color framebuffer is very slow

- add menu font
- implement png's transparent color in png load & drawing

- calculate longest distance between yields
- check sleep mode power consumption for pedometer
- where to put rumbler? 

- can start working on GPU Benchmarks? 
- framebuffer clear with DMA right after drawing is done, or in parallel with drawing if requested

- microphone tested to work - sample code https://github.com/ArmDeveloperEcosystem/microphone-library-for-pico/tree/main 
- needs integration and lots of amplification it seems 

## Case

- see if the cartridge insertion mechanism can be made reliable (looks ok)
- check if the slim buttons are actually working reasonably well
- Select & Start buttons need repositioning to accomodate the speaker and the SD card
- top midframe does not fit around UPDI programmer and other headers, and the speaker
- and there is still warping when screwed together
- enlarge speaker hole

## Audio Woes

- the new version on breadboard has markedly better audio quality (although not perfect) in headphones
- try with 44.1 kHz
- better audio? 


- sound quality is rather poor - is it the board? does it change if AVR leaves headphones open?
- is the circuit wrong? - https://datasheets.raspberrypi.com/rp2040/hardware-design-with-rp2040.pdf
- add big cap to vclean and see if it changes stuff 
- seem to be easily driven from PWM timer being the desired frequency
- can test different circuits on the cartridge boards (PWM through the cartridge)
- verify that this is not due to some SW issue, because basic tone seems ok-ish
- particularly the big resistors in parallel with headphones are something quite different from what I currently have
- headphone volume is too high, speaker volume is too low
- can we do PDM instead? 

- check https://github.com/TuriSc/RP2040-PWM-DMA-Audio/tree/main and https://github.com/TuriSc/Dodepan for hints perhaps? 

## PCB

- Q: the level translator seems not to be working allright. The rising edge up takes a *lot* of time from the vlow
- Q: when no headphones are present, the headphones line only has about 2 volts, not 3volts it is supposed to have. Is this an issue? 
- Q: the audio is really very noisy. What could be improved?
- Q: does a LC low pass filter before LDO help in any way?

- try the backlight current limiting to be 3.7Ohm for ~90mA max current
- increase pads for the bottom side button for easier hand solderablity and maybe use the buttons with hole in the middle for better grip
- pull-up resistors & termination resistors for the SD card (the sd card library has some info)
- AB buttons are not ceneterd right, fix
- Select & Start buttons need to go a bit up
- add labels to PGM and other headers on the bottom side of the PCB
- where to put the rumbler? Can't be next to the speaker (two magnets)
- add holes for the display legs for lower profile

??? - add headers to the bottom side of the PCB for the solderable wires

## RP

- document ST7789
- disable file & line information for fatal errors
- print fatal error to USB serial

## AVR

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

Tile = bitmap with statically given dimensions (really only a storage format)
Bitmap = bitmap with dynamically known dimensions 
Canvas = bitmap with extra state

Support 16,12,8,4,2 and 1 bitdepth

- pngdec does not use malloc
- libhelix does (but can be patched imo)

