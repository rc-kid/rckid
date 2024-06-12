# RCKid SDK

The RCKid SDK provides basic access to the peripherals available on RCKid. Notably those include the graphics system (capable of 320x240x16bpp graphics in either bitmap, or tiles & sprites mode), audio (mono/stereo output and mono microphone, mixing & tone generation), storage (SD card and cartridge flash storage), other hardware (accelerometer, step counter) and cartridge-specific features (most notably radio). This document presents an overview of the system and their features with links to code that contains more detailed information. 

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

## Graphics

Two basic graphic modes are supported - _bitmap_ and _tile_ oriented. The _bitmap_ mode utilizes bitmaps, canvases and framebuffers that support either 8 or 16 bpp and 