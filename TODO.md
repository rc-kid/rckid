## Checklist

## TODO 

- the level translator seems not to be working allright. The rising edge up takes a *lot* of time from the vlow
- when no headphones are present, the headphones line only has about 2 volts, not 3volts it is supposed to have. Is this an issue? 

- use PIO for pixel doubling on vline


## Case

- see if the cartridge insertion mechanism can be made reliable
- check if the slim buttons are actually working reasonably well

## PCB

- try the backlight current limiting to be 3.7Ohm for ~90mA max current
- increase pads for the bottom side button for easier hand solderablity
- add pin help text to the GPIO and the programming interface
- the logo on the back is a bit ugly & not very informative
- display pins 11-15 maybe should be grounded (unused data pins)
- pull-up resistors & termination resistors for the SD card (the sd card library has some info)

## Extra Cartridges

- simple cartridge (only FLASH)
- add SRAM (maybe https://cz.mouser.com/ProductDetail/ISSI/IS66WVS2M8BLL-104NLI?qs=doiCPypUmgFx786bHGqGiQ%3D%3D)


- add big cap to vclean and see if it changes stuff 


## AVR

- on wakeup sequence, check that start & sel are pressed too to enter debug mode(s)
- add & test battery charging detection
- add battery critical power off
- add neopixel & rumbler effects
- add I2C commands


## Shopping List

- buttons (https://www.tme.eu/cz/details/b3u-3000p/mikrospinace-tact/omron-ocb/b3u3000p/)
- battery (https://www.tme.eu/cz/details/accu-lp503759_cl/akumulatory/cellevia-batteries/l503759/)
- rumbler (maybe this - https://www.tme.eu/cz/details/oky3170/dc-motory/okystar/)
- precission headers ()
- screw inserts (https://www.prusa3d.com/product/threaded-inserts-m2-short-100-pcs/)
