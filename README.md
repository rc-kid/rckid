# RCKid mk II

RCKid mk II is an RP2040 (the chip that powers RPi Pico) handheld device that can be used to play own games, emulate some very old consoles, play music and video files and act as a walkie-talkie or remote controller with the RCKid mk I protocol.

To resemble the handheld game consoles of the past, RCKid mk II uses swappable cartridges that can extend its functionality. At minimum, the cartridge must provide the flash chip for the RP2040 inside, but extra power, I2C, SPI and 4 GPIO pins are provided for HW expansion. 

## Device

The device consts of the RP2040 itself, a 2.8" 8bit 8080 display, audio in & out, accelerometer and other sensors.  

## Cartridges

The default cartridge 

## WiFi Beacon





This is an initial repository for my new project, RP2040 based smaller RCKid with actual cartridges for real nostalgia feel. The project should be relatively cheap, easy to build and its parts should be plentiful. The emphasis this time is not on emulation, but on creating own games and getting smart with the cartridges like in the olden days.

> !! Can preorder parts at JLCPCB to ensure they are available to me when I want to assemble them 

## Device

The device itself will contain the basic necessary hardware, including:

- 2.8" screen
- RP2040
- SD card for memory
- audio outputs (headphones & mono speaker)
- microphone
- battery & charger (https://www.tme.eu/cz/details/accu-lp503759_cl/akumulatory/cellevia-batteries/l503759/)
- sensors (accel, photo, ...)
- rumbler

### Stackup

> total stackup is 11mm

- 1mm plexiglass cover
- 3mm display & buttons & room for top 
- 1mm main PCB
- 5mm battery
- 1mm bottom 


### Display

> https://www.aliexpress.com/item/1005004635814413.html?spm=a2g0o.store_pc_groupList.8148356.36.28a0cb5fia4tAE&pdp_npi=3%40dis%21CZK%21CZK%20146.38%21CZK%20146.38%21%21%21%21%21%4021038edf16929925175593334e3f07%2112000029921223776%21sh%21CZ%213305825785#nav-review 

This should be a nice IPS screen with smaller margins for minimal footprint. Also seems to have all the required, including the tearing effect synchronization. Alternatively any non-IPS ILI9341 or ST7789 screen with the same pinout should be compatible as well at the expense of the display's active area not being centered perfectly 

Driven in 8080 8bit mode by the RP2040. 

### SD Card

Attached to the RP2040, uses the SPI interface. Contains the 





### RP2040 (30 pins)

- 12 8080 8bit display
- 4 SD card SPI
- 1 microphone
- 2 audio out

- 2 I2C (cartridge connector)
- 4 SPI (cartridge connector & SRAM)
- 4 GPIO (cartridge connector)

- free: 1 pin (used as CS for the SRAM chip)

### ATTiny1616 (17 pins)

> Using the smaller xx16 part as they are more readily available

- 2 I2C
- 7 (3x4 matrix) buttons (11 buttons - 4 dpad, A, B, Sel, Start, VolUp, VolDown, HOME) - 1 free
- 1 backlight
- 3 power, VBATT sense, charging sense
- 2 neopixel power + neopixel
- 2 headphones, speaker on

## Cartridges

Cartridges and the device connect via a single row of 20 precision (SIP connector, single row) pins with 2.54mm spacing. The connector will provide the following interfaces:

- QSPI for flash memory (6 pins)
- SPI (4)
- I2C (2)
- GPIO (4)
- GND (2)
- VBATT (1)
- 3V3 (1)

At a minimum, the cartridge must provide the QSPI flash for the RP2040. The design should allow for the inclusion of NRF24L01P and LoRa modules and IR sensors/diodes as well so that different cartridges can be created by populating different components:

- base cartridge (flash only)
- connected cartridge (flash + NRF/LoRA)
- IR remote (flash + IR)

Optional cartridges in next generation would include: 

- camera (SPI models only with buffer)
- WiFi connected cartridge (ESP8266 or some such)

## Lessons learned

- many screws necessary for the 3D printed parts to fit
- order plexiglass covers (https://plasticexpress.cz/), fairly cheap





# Things to try

- don't use battery sense pin on AVR (either VCC if not charging, or show charging when charging) (saves 1pin)
- BQ25895 or when used this, we can read voltage & charging current for better visualization (saves 2 pins)
- rename the pins in the cartridge connector
- INA3221 can tell me charging, VBUS and VSYS all at once. Maybe worth a try? 
- can then use smaller I2C charger w/o the status 