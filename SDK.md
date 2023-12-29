# RCKid SDK

## Resources Used

- DMA channel for the display
- 2 DMA channels for the SD card (when mounted)
- DMA_IRQ_0 in shared mode for both the display and the DMA
- single SM in PIO0 for the display driver

## Configuration

`RCKID_SPLASHSCREEN_OFF` 

Disables the initial logo splashscreen (blank screen will be showed instead), conserves a bit of flash memory. 

`RCKID_AUDIO_DEBUG` 

Enables the audio output on cartridge available pins 14 & 15 for further processing by the cartridge or easier debugging via the cartridge port. 
