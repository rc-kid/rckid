# Platform Framework

This is a very simplified framework for low level basic functions for the various embedded and not so embedded devices I often work with. Each platform provides basic gpio, cpu, i2c and spi abstractions so that the code for very basic peripheral control can be shared between them. Certain platforms also provide specialized functionality beyond the common basics. 

## Suppoprted Platfoms

- `attiny_series_1` for the so named AT tiny chips. Does not require arduiono to be presents and talks directly to the HW. Suitable for chips like ATTiny3217, ATTiny1616, etc. 
- `rp2040` for Raspberry Pi's RP2040 microcontroller and the RPi Pico board as well
- `rpi` for RaspberryPi linux computers, runing Raspbian OS. There are multiple libraries that can be used to deal with the gpio from C/C++. The [`pigpio`](http://abyz.me.uk/rpi/pigpio/) is fairly decent, but thanks to the constant polling of the gpio pins uses non-trivial amounts of CPU. The [`wiringPi`](http://wiringpi.com/) seems to be much older and less robust, but its gpio implementation does not use the CPU. The library does not provide as nice to use I2C and SPI functions, so the rpi platform implements those from scratch using linux character devices.  
- `arduino` a fallback solution for any arduino compatible device, provides wrappers around teh arduino functions
- `esp8266` which is mostly arduino at this point
- `mock` a mock platform where the functions exist, but don't really do anything for basic functionality testing on dev boxes, etc. Useful for local testing of the `rpi` and `rp2040` platforms where no HW communications is required

## Useful tools

### Serial Monitor

First ensure that you have the necessary permissions, e.g. by running:

    sudo usermod -a -G dialout YOUR_USER_NAME

Then run the following:

    picocom -b BAUDRATE /dev/ttyUSB0

To terminate, run `C-a C-x`.     