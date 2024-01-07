# RCKid SDK

## Resources Used

- DMA channel for the display
- 2 DMA channels for the SD card (when mounted)
- DMA_IRQ_0 in shared mode for both the display and the DMA
- single SM in PIO0 for the display driver

## USB Virtual COM port

For debugging purposes, RCKid supports serial port over the USB connection that can be used for sending debug messages to the attached PC. For direct use, call the `writeToUSBSerial()` function directly, or use the `LOG` macro for convenience.

> This should be pretty straightforward on Linux, on modern Windows, the legacy harware ports should be enabled in the _Turn windows features on/off_ menu. A putty or a minicom can then be used to inspect the output. 

## Configuration

`RCKID_SPLASHSCREEN_OFF` 

Disables the initial logo splashscreen (blank screen will be showed instead), conserves a bit of flash memory. 

`RCKID_AUDIO_DEBUG` 

Enables the audio output on cartridge available pins 14 & 15 for further processing by the cartridge or easier debugging via the cartridge port. 
