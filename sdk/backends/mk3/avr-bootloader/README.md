# I2C Bootloader

This is a simple I2C bootloader than can be used to flash the AVR firmware directly from the RP2350 cartridges. As the bootloader 
The bootloader returns avr status as normal firmware would, but indicates that its in the bootloader mode. When in bootloader mode, special commands exist  

Flashing the AVR is done via a dedicated cartridge that when inserted will:

- put the AVR into bootloader mode
- read the memory to determine the stored version
- if the version is different than the one embedded in the bootloader, it will flash it
- informs user all is done




# Old - (from mkI)

A simple I2C bootloader that fits in 512 bytes. Both the bootloader and the programmer are pretty specific for the `rckid`, but minimal changes should make them generic. 

## Commands

Each I2C command is a single byte, optionally followed by an argument. The commands are designed in such way that they allow page-by-page writing and reading the memory so that the programmer can write and verify the memory contents with minimal AVR code required. 

`0x00 Reserved`

> Not used for anything. 

`0x01 RESET`

> Resets the chip.

`0x02 INFO`

> Updates the `State` object and sets the buffer address to it so that next reading from the device will return the state object itself. 

`0x03 WRITE_BUFFER`

> Writes data to the buffer. The command is only expected to be used after first setting the buffer address via `SET_ADDRESS` command. Using the flexible set address and the xmega core on Series-X ATTiny chips the same command can be used for writing both flash and eeprom. 

`0x04 WRITE_PAGE`

> Commits the page previously written to the appropriate memory (based on its mapped address). Sets the last error in `Status` info. 

`0x05 SET_ADDRESS`

> The command is followed by a 16bit unsigned which will be the new address for the buffer operations (write/read). By setting the address to the appropriate memory mapped locations for either flash or EEPROM memory the bootloader can simply write both. The same mechanism is also used to retrieve the `Status` object. 

## Bootloader

__Note that the UPDI programmer does not check if the chip is correct. If the chip specified in platformio and the actual chip do not match, the memory writes might not work even if everything else would (different memory will be written due to different page sizes) !!.__ 

The bootloader checks the status of the `AVR_IRQ` pin and if low, enters the bootloader mode. When high (i.e. pull-up enabled on the RPi side) starts the application. The bootloader is protected by a watchdog so that inactivity longer than a second will trigger reboot of the chip. The watchdog is reset after a command receive. 

When the bootloader is busy with a command it signals it by pulling the `AVR_IRQ` low, returning to input state when the command is executed, which allows fast programming times. 

To make programming observable, the bootloader also sets the display brightness to maximum. 

## Programmer

A simple i2c programmer is provided as well. Usage:

    i2c-programmer write HEX_FILE

`HEX_FILE` is path to the Intel HEX file that should be flashed to the chip. By default the programmer must be run by `root` to have access to the I2C and GPIO it requires to communicate with the AVR. The programmer can either time the commands by using the `AVR_IRQ` pin, or it can add a timeout to the commands when the `AVR_IRQ` connection is not available for whatever reason so that the bootloader can be used over I2C entirely. 

> Apart from basic flashing the programmer can do a bit more. Use the `--help` command for list of options. 

## Resources

- https://docs.platformio.org/en/latest/platforms/atmelavr.html#bootloader-programming
- https://www.microchip.com/en-us/application-notes/an2634
