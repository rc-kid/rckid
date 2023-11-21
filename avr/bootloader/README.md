# AVR Bootloader using the I2C protocol

> Unlike the previous version, the I2C communication between the AVR and RP2040 lacks the AVR_IRQ pin, so there is no callback available. This also means that the only way to detect a bootloader has been activated is via the EEPROM memory (this is allright as the EEPROM has order of magniture better rewrite rating than the flash). 

> TODO: If there is some memory left, it would be good to make the bootloader understand also the emergency RP2040 bootloader mode entry, alternatively UPDI can be used. There is probably plenty of space, because the AVR's programming itself will be quite small. 

Ideally, the rckid's basic library should allow for a bootloader to be present by checking the contents of the flash card and if it 