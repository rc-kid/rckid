# Oscilloscope & Function Generator

> Uses the two ADC pins for 2 channel oscilloscope and some resistor ladder or PWM for custom function generator. Should work with 0..24V.

# Multimeter

> Ideally simple multimeter with GND, V, A jacks that can measure voltage, current and resistance. Maybe also add somehow an option that can measure *voltage* and *current* at the same time.

# Digital Analyzer

> A cartridge that can be used for various digital functions. Should support a simple logic analyzer (at least 4 bits), should also be able to communicate explicitly with devices via UART, SPI, I2C, etc. Should be able to pass through the UARTs into CDC USB device. The inputs would be 5V tolerant, but output will be 3v3.

# RCKid Development Cartridge

> A special cartridge that will have extra PCB on it that would allow it to connect to the debug port on another rckid. It could then be used as UPDI programmer, serial reader for the AVR and RP2350 (or passthrough for USB) and SWD debugger via USB. The connector can be on cable and we can have this as a simple GPIO out cartridge and special board with the updi stuff on it. 

# WiFi & BT

> Uses RM2 from Raspberry Pi for BT & WiFi access. 

# Radio Cartridge

> The below is comments from version 3.1 which had the Si4705 chip onboard. This was later changed and radio can move to its own cartridge if needed. 

[Si4705](datasheets/si4705.pdf) is the chip used. It is FM only receiver with ability to use both embedded and headphone integrated antenna. The FM chip also comes with [antenna guidelines](datasheets/si4705-antenna.pdf) which contains layout and typical application information. It communicated withj RP2350 via I2C and has two extra lanes - RESET and INT. We leave GPO1 unconnected so that I2C communication mode will always be selected. Finally there is the [programming manual](datasheets/si4705-programming.pdf) for the I2C setup.

I've also found an [Si4705 module](https://media.elv.com/file/140984_fm_rm1_schaltplan.pdf) whose schematic can be of use. 

The rest of the pins are used as follows:

- `FMI` is connected to headphones antenna
- `RFGND` goes to ground
- `LPI` is the embedded antenna
- `RSTB` is the RADIO_RESET to RP2350
- `SENB` goes to ground (I2C address selection - `0b0010001`)
- `SCLK` and `SDIO` are the I2C SCL and SDA lines to RP2350 respectively
- `RCLK` and `GPO3` are connected to 32.768kHz crystal
- `VD` and `VA` both go to `IOVDD`. VD has 100nF capacitor and VA has 22nF capacitor (from the app in antenna datasheet)
- `GND` is ground
- `ROUT` and `LOUT` are analog outputs. They go to the audio codec via 10k to ground and 100nF capacitor
- `DOUT`, `DFS` and `DCLK/GPO3` are not used and left floating (digital audio out)
- `GPO2` is connected to the RADIO_INT and goes to RP2350, pull-up does not seem to be necessary
- `GPO1` is not used

### Headset Antenna

The headset antenna is described in the antenna datasheet, page 19. Headset ground is connected to gnd via 270nH inductor as per the datasheet and to the FMI pin via a 100pF capacitor. To insulate the ground antenna, ferrite beads are installed on both left and right audio outputs, as well as the headphone detection line. 

### Embedded antenna

As per the antenna application note, page 31, the embedded antenna is connected directly to the LPI pin win 120nH inductor to the ground. The inductor is 0805 so that it's hand-solderable. The ESD protection and current limiting resistor are not necessary as there is no exposed connector to the embedded antenna. 

### External Crystal

We are using the same crystal as AtTiny3217. The load capacitance is 12.5 pF, which is roughly in the middle of the rangle per datasheet. While the dev-board had 22pF capacitors, the crystal looked like it is not swinging fully, so we are trying 15pF for the device, which would correspond to 5pF stray capacitance. This is under the assumption that lower than ideal capacitance is better than higher than ideal, but needs to be verified. Internet says this should not degrade the crystal accuracy too much as only a few seconds per week max. 
