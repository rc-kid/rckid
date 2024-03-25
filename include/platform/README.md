# Platform Framework

This is a very simplified framework for low level basic functions for the various embedded and not so embedded devices I often work with. Each platform provides basic gpio, cpu, i2c and spi abstractions so that the code for very basic peripheral control can be shared between them. Certain platforms also provide specialized functionality beyond the common basics. 

## Suppoprted Platfoms

- `attiny_series_1` for the so named AT tiny chips. Does not require arduiono to be presents and talks directly to the HW. Suitable for chips like ATTiny3217, ATTiny1616, etc. 
- `rp2040` for Raspberry Pi's RP2040 microcontroller and the RPi Pico board as well
- `rpi` for RaspberryPi linux computers, runing Raspbian OS. 
- `arduino` a fallback solution for any arduino compatible device, provides wrappers around teh arduino functions
- `mock` a mock platform where the functions exist, but don't really do anything for basic functionality testing on dev boxes, etc. Useful for local testing of the `rpi` and `rp2040` platforms where no HW communications is required