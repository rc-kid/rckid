## TODO

- can start working on GPU Benchmarks? 
- framebuffer clear with DMA right after drawing is done, or in parallel with drawing if requested

- should we ditch other color modes than 565? 

- sound quality is rather poor - is it the board? does it change if AVR leaves headphones open?
- is the circuit wrong?

## Case

- see if the cartridge insertion mechanism can be made reliable
- check if the slim buttons are actually working reasonably well

## PCB

- Q: the level translator seems not to be working allright. The rising edge up takes a *lot* of time from the vlow
- Q: when no headphones are present, the headphones line only has about 2 volts, not 3volts it is supposed to have. Is this an issue? 

- where to put the rumbler? maybe used encolosed speaker and put rumbler underneath it 
- try the backlight current limiting to be 3.7Ohm for ~90mA max current
- increase pads for the bottom side button for easier hand solderablity
- display pins 11-15 maybe should be grounded (unused data pins)
- pull-up resistors & termination resistors for the SD card (the sd card library has some info)

- add pin help text to the GPIO and the programming interface
- the logo on the back is a bit ugly & not very informative

- add big cap to vclean and see if it changes stuff 

## RP

- document ST7789
- add PIO driver unload option
- make I2C non-blocking to avoid blocking main thread

## AVR

- on wakeup sequence, check that start & sel are pressed too to enter debug mode(s)
- add battery critical power off
- add neopixel & rumbler effects
- add I2C commands

## Picosystem

- clearing the screen does not work (perhaps overclocking issue)
- IRQ does not work
- add audio & friends and AVR integration
- rgba is not binary compatible with picosystem, fix - picosystem has its driver color-wise working now
- the waiting for updating not working on picosystem
- true picosystem is BGR

## Shopping List

- buttons (https://www.tme.eu/cz/details/b3u-3000p/mikrospinace-tact/omron-ocb/b3u3000p/)
- battery (https://www.tme.eu/cz/details/accu-lp503759_cl/akumulatory/cellevia-batteries/l503759/)
- rumbler (maybe this - https://www.tme.eu/cz/details/oky3170/dc-motory/okystar/)
- precission headers ()
- screw inserts (https://www.prusa3d.com/product/threaded-inserts-m2-short-100-pcs/)

## Extra Cartridges

- simple cartridge (only FLASH)
- add SRAM (maybe https://cz.mouser.com/ProductDetail/ISSI/IS66WVS2M8BLL-104NLI?qs=doiCPypUmgFx786bHGqGiQ%3D%3D)


