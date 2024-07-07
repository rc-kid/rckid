# RCKid SDK Library

- `avr` contains code for the AVR chip inside RCKid that handles power management and user inputs mostly
- `cartridges` contains simple static libraries providing the cartridge intialization and yield routines. One of these libraries has to be linked together with the librckid in each executable (cartridge)
- `librckid` contains the SDK itself, which handles all the common hardware (display, audio, SD card, etc.)
- `platform` contains platform override for RCKid (i.e. RP2040 plus extra RCKid features)
