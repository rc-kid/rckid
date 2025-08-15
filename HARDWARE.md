# Hardware Design

> This is hardware description of RCKid mk III (version 29/7/25 and above). For brief history and to clear any confusion, mk I was much larger, hand soldered version powered by Raspberry Pi Zero 2W and mk II was slightly slimmer version powered by RP2040 with 8bit parallel screen interface.

RCKid is based on the [RP2030B](datasheets/rp2350-datasheet.pdf) (80 pin QFN) and uses its requirement for external program flash chip as a feature by allowing swappable cartridges. This is the overall architecture of the system:
          
                                              +-----------------+
                            + --------+       | Cartridge       |
                            | microSD |       | (flash, extras) |                                    
                            +---------+       +-----------------+                            
                                   \\\\       ////                                           
                          <SPI/SDIO>\\\\     ////<8pin/HSTX,2/ADC,QSPI>                           
                                     \\\\   ////                                             /---- speaker
                                     ||||   ||||                                            /
       +-----------+  <16bitMCU>   +-------------+     <I2S>       +---------------+       / (analog mono)
       |  ST7789V  |===============|   RP2350B   |=================| NAU88C22GY    |-------
       | (Display) |               +-------------+                 | (audio codec) |_______   
       +-----------+                     ||                   //===+---------------+       \ (analog stereo)
              |                          ||                  //       |                     \         +------------+
              |                          ||                 //        | (analog stereo)      \--------| 3.5mm jack |
              |                    <I2C> ||================//         | (reset)              /        +------------+
              |                          ||                      +------------+             / (FM atnenna)
              |                          ||======================|   Si4705   |------------/
              |                          ||                      | (FM Radio) |--- (embedded antenna)
              | <PWM>                    ||                      +------------+
              | (Backlight)              ||
               \                         ||
                \                        || <I2C>     
                 \                       ||           //======(buttons, 3x4 mtrix)
                  \               +-----------------+//
                   \              |    ATTiny3217   |
                    \-------------| (IO, time, pwr) |----------(RGB)
                             /----+-----------------+
                            /            ||         \     +-----------+
             (rumbler)-----/       <I2C> ||          \----| MCP73832  |----(battery)
                          <PWM>          ||               | (charger) | 
                                    +----------+          +-----------+
                                    | MPU6500  | 
                                    | (Accel)  | 
                                    +----------+

Next to the control & logic elements above, there are also various power rails, their short description is below:

- `VUSB` is the +5V coming from the USB port, active when USB is connected, used for charging
- `VBATT` is the battery terminal 
- `VCC` is the main system rail. This comes either from the battery, or from the USB when connected. Valid voltages are from 3v to 5v. This rail is measured by the AVR to determine battery status / USB presence and is fed to the 3 DC-DC converters below:
- `3V3` and `IOVDD` are identical buck-boost converters to 3v3. The `3V3` rail is always on and powers the AVR, accelerometer, display backlight and rumbler. The `IOVDD` can be enabled by the AVR and powers the RP2350B, cartridge and audio system
- `5V` is a charge pump to 5V required by the RGB lights (controlled by AVR)

## I2C Bus

Most of the perihperals are connected to a single I2C bus (accesible on `TP6` and `TP7`). The pull-ups of the bus go to the `IOVDD` so that the bus is only working when the device is on. To make sure eveything works, before AVR turns `IOVDD` off, it emits `START` condition on the bus, which is followed by `STOP` condition transmitted right after `IOVDD` is enabled back. 

### Sensors

The only extra sensor in mkIII is the onboard acceleromeneter [LSM6DSV](datasheets/lsm6dsv.pdf). Unlike MPU6500 or various TDK alternatives is relatively simple to use (similar to the discontinued BMI160 used in mkII). As it is also capable of working as a pedometer, it is always on, connected to the `3V3` rail. The accelerometer has its interrupt connected to AVR so that we can intercept taps, steps, etc.

Furthermore several of the devices connected to the I2C bus also supoort temperature measurements (including the AVR itself).

## Power Management

Power management consists of a standalone Li-Ion battery charger [MCP73832](datasheets/MCP73832.pdf) and a power path via a MOSFET and diode that enables DC power when connected, and battery otherwise. MCP73832 *must* be used as the AVR runs on 3.3V while the charger runs at 5V so outputting charger high when charge is complete as the 73831 does could destroy the chip. There is no such danger with MCP73832 as the STAT pin behavior is HiZ when not charging and low when charging.

The charger pinout is as follows:

- `VDD` is connected to the VUSB (5v)
- `STAT` is the charger state output (low when charging, floating otherwise)
- `VBAT` goes to the battery
- `PROG` is connected to `2k2` resistor for `???` max charge current

The AVR monitors the `VCC` rail (after power path), which enables it to determine battery voltage when powered from battery (value <= `4.2V`) and if USB is connected (VCC above `4.2V`). This is done through a `100k/200k` voltage divider that is always on. This seeps negligible battery power and is above the `50k` input impedance of the AVR adc, but we compensate for this by longer capture times. Charger status is monitored via the `STAT` pin that is connected to a `100k/200k` voltage divider on the `VUSB` (so that it is pulled high at `3v3` when USB connected, `0` otherwise). As per the datasheet, the `STAT` pin is floating when not charging and goes to `0` while charging. This gives the following power scenarios:

- `VCC<=4.2V` battery is connected, `STAT` should be 0 (but can really be ignored)
- `VCC>4.2V` and `STAT` is low means USB is connected and the device is charged
- `VCC>4.2V` and `STAT` is high means USB is connected, but charging is done

### Li-Po Battery

By default the device runs from a LiPo battery such as [503759](https://www.tme.eu/cz/details/aky-lp503759/akumulatory/akyga-battery/aky0107/).

### Running from AAA batteries

The device also supports a different configuration when it can be powered via 3 `AAA` batteries in series. This gives maximum voltage of `4.5V` (for alkaline, will be around `3.6` for NiMH) and minimum voltage can go *very* low for alkaline, or about `3V` for NiMH. In those settings the cutoff voltage is defined by the minimal input voltage of the DC-DC regulators, and particularly the `5V` charge pump, whose minimal voltage is `2.7V`

To enable the `AAA` mode, the charger chip simply should not be soldered. 

> If the batteries are replaced while the device is powered by USB, there is no data loss wrt timekeeping, or user information. 

> The plan is to use clip-on contacts on the batteries for lowest possible clearance, those will be added to two extra pcbs that will slide to the extended case and connected by wires to the +/- BATT terminals

### 3.3V Buck-Boost converters

The design uses two identical step down converters - [TPS631000](datasheets/tps631000.pdf). This is newer chip than the original designs in mkII and devboard and provides greater efficience and range. As per its datasheet, it is capable of providing `800mA` in boost mode (and `1.2A` in buck), which is within the limits (500mA cartridge + 300mA for RP2350B and audio is the limiting factor here). 

The converter is variable output resistor configuration straight from the datasheet is used for `3v3` output. So is the inductor and the recommended layout. The two DC-DC blocks are almost identical with the exception of the on switch, which for `3v3` is tied to input voltage for always enable, while for the `IOVDD` is pulled down with `100k` resistor for default off behavior and will be pulled high to `3v3` by the AVR when enabled (this is well above the logic high threshold for the chip at `1.2V`. The chip tolerates up to `7V` on the `EN` pin regardless of the input voltage, so even with very low VCC (technically we can go as low as `2.4V` the chip will work).

### 5V Charge Pump

We are using [HX4002](datasheets/hx4002.pdf) charge pump, same as in mk II. The pump is capable of delivering in excess of 240mA for 3.6V input voltage (it runs from VCC, which is stepped up above 3.3v). It only powers the RGB leds, which take at most 12mA per channel, which gives us 6.6 RGBs at full power.

WS2812 compatible, but smaller & less powerful LEDs are used. As they require 5V and the AVR that controls them runs on 3.3V we use a converter.

> TODO should there be 4 LEDs in the DPAD? maybe yes

### Power Consumption

To share the load between the two DC-DC converters, teh display backlight and the rumbler (which can separately be turned off) are connected to the `3V3` rail. The `TPS63001` is very effective at high loads, so we assume `90%` efficiency (will be a bit higher in reality). The calculation for the maximum power that can be drawn from the battery at the lower acceptable level (`3V`) is thus:

    MAX = (800 * 3.3 / 3.0 / 0.9 * 2) + (240 * 5 / 3 / 0.8)
    MAX = 1955 +                        500
    MAX = 2455 [mA] @ 3V

For a more realistic example, let's assume full utilization of the `IOVDD` converter (i.e. 300mA for the RP2350 and 500mA for the cartridge), 110mA utilization for the `3V3` converter (100mA brightness, 10mA for rumbler as mostly off), and 50mA for the RGBs (red at 50%):

    REAL_MAX = (800 * 3.3 / 3 / 0.9) + (110 * 3.3 / 3 / 0.9) + (50 * 5 / 3 / 0.8)
    REAL_MAX = 977                    + 134                   + 104
    REAL_MAX = 1215 [mA] @ 3V

This is within the `1C` discharge rate for the battery, which is good. 

## Audio

[NAU88C22GY](datasheet/nau88c22gy.pdf) chip is used, which is relatively simple, yet it offers all the features we need - 16bit audio playback & record, line-in for the radio, microphone input, integrated headphone and mono speaker (BTL) drivers. While the chip features audio jack detection and can automatically enable/disable headphones based on the jack status, it does not offer any way to report the jack status, hence the jack detection pin is shared with RP2350 which can read it. The chip also provides no interrupt (no events available) and provides one simple digital GPIO that can be set high or low (used for the radio reset line). In terms of I2S, the chip requires MCLK to be provided, but it does not have to be of the exact frequency needed as the internal PLL can adjust it - but we are not using this feature. Pinout:

- `VDDA`, `VDDB`, `VDDC` and `VDDSPK` are all connected to the `IOVDD` 3v3 rail for simplicity, decoupled with `4u7` caps
- `VSSA`, `VSSD` and `VSSSPK` are all connected to `GND`
- `SCLK` and `SDIO` are connected to `SCL` and `SDA` respectively via a `220` resistor and `33pF` capacitor, as per the datasheet to add filtering to the lines
- `FS`, `BCLK`, `ADCOUT` and `DACIN` are connected to the `I2S` bus
- `MCLK` is connected to the master clock generated from the RP2350
- `LHP` and `RHP` are connected to headphones via large `220uF` capacitors
- `LSPKOUT` and `RSPKOUT` are connected to directly to the speaker in BTL mode
- `RAUXIN` and `LAUXIN` are connected via `1uF` capacitors to the radio analog output (as per the datasheet)
- `LMICP` and `LMICN` go to positive and negative microphone terminals via `1uF` capacitors (as per the datasheet)
- `MICBIAS` is connecte to ground via `4u7` capacitor and to positive mic input via `2k2` resistor (as per the datasheet)
- `RMICP`, `RMICN`, `AUXOUT1` and `AUXOUT2` are left floating (as per the datasheet for unused analog inputs/outputs)
- `GPIO1` goes to radio reset line (via `100k` pull-up)
- `GPIO2` goes to headset detection
- `GPIO3` is tied to ground as per the dataset for unused gpio
- `MODE` is tied to ground for I2C selection

#### Microphone

As per the typical application, the microphone is connected differentially to `LMICP` and `LMICN` with `2k2` from MICBIAS to positive mic, 0.1uF capacitor on the positive output to right channel and direct connection to ground on the negative channel, which also goes to the left input via 0.1uF capacitor.

#### Headset detection

Headset detection is via a `100k` resistor to the `IOVDD` line to second ground terminal of the headphone jack. This will read high when headphones are not detected and will go to `0` when headphones are inserted as the ground will be connected to both terminals. The headset detection line is then fed to the audio codec for automatic speaker/headhone switching and to the RP2350 as well to determine the headphone status. 

> TODO verify the pull-up is ok - would this work for headsets with microphones as well? 

### Radio Reset

As the audio codec's `GPIO1` can be used as a general purpose digital output, we use it as a radio reset line to save pins at the RP2350B. The line pulled up by default (no reset). 

### FM Radio

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

## AVR (ATTiny3217)

[ATTiny3217](datasheets/ATTiny3217.pdf) is used in QFN package and is responsible for power management, input & output (buttons, rumbler and display PWM) and some sensors (RTC clock, temperature sensor). All pins are used, the AVR_INT pin plays double duty as AVR interrupt to RP2350 and serial TX from the AVR chip for debugging (on RPI the pin matches alt UART0 RX). The chip also runs from 3.3V as well, so it can't use VCC measurement to check battery levels (instead it uses dedicated 100/200kOhm divider into a pin for voltage measurements. Pinout is as follows:

- display backlight PWM on `PC0` (TCB0 WO alt)
- rumbler PWM on `PA3` (TCB1 WO)
- I2C SCL and SDA on `PB0` and `PB1` (default)
- AVR_INT (and TX) on `PA1` (UART TxD alt)
- 32.768kHz ext oscillator at `PB2` and `PB3`
- VCC_SENSE at `PA2` (which is AIN2 for ADC0 in AAA battery mode)

Remaining functionality is interchangeable as they are used simple digital pins, namely:
- BTN_1, BTN_2, BTN_3 and BTN_4 for button matrix rows (`PA6`, `PB7`, `PA7` and `PC2` respectively)
- BTN_DPAD, BTN_ABXY, BTN_CTRL for button matrix cols (`PC4`, `PB6` and `PB5` respectively)
- ACCEL_INT (input from accel) at `PB4`
- CHARGING detection (low when not charging, high when charging) at `PA5`
- IOVDD_EN and 5V_EN for IOVDD and 5V power rails at `PC1` and `PA4`
- RGB for neopixel control at `PC3`
- QSPI_SS, connected to RP2350 QSPI_SS pin to boot into bootloader at `PC5`

### Buttons

Due to low number of pins available, a classic button matrix is used. Buttons are grouped in three columns: DPAD, ABXY (A B Sel and Start, historically) and CTRL (home, vol up & vol down). When the device is power off, the CTRL is the only active group so that Home button interrupt works.

### RTC

In mkII the internal oscillator was used for RTC which led to inaccuracies of up to 10 minutes per day. This time we are using external 32.768 kHz oscillator for better accuracy. The crystal load capacitance per the datasheet (page 511) is max 12.5pF which is the value used, so the same circuit as for the radio should work. 

### Rumbler

Rumbler is connected to 3V3 and triggered by AVR PWM. Protective diode is used as well. 

## RP2350B

[RP2350B](datasheets/rp2350-datasheet.pdf) - 2 Cortex M33 core @150Mhz, 520KB RAM, PIO and so on. Arguably, there are chips that are more powerful (high-end STM, ESP32 would provide WiFi for "free"), but none I have tried beats the fun of programming RP2350:) So I am sticking with it. Further details of how the HW requirements for the chip should look can be found in [HW Design Notes](datasheets/hardware-design-with-rp2350.pdf). Basic pinout is below, for more details see the desciption of subsystems below

- I2C SDA & SCL (gpio `0` and `1`)
- Radio interrupt (`2`)
- AVR interrupt (and debug UART TX) at `3`
- I2S MCLK at `4` (its own pio sm)
- I2S DIN, DOUT, BCLK and LRCLK (`5`, `6`, `7`, `8`) (record & playback sms)
- HEADSET_DETECT (`9`)
- SD_CD (card detect) `10`
- display RDX at `11`
- cartridge HSTX (`12`-`19`)
- display DB15-DB0 (`20`-`35`)
- display WRX, DCX, CSX (`36`, `37`, `38`)
- display TE (input at `39`)
- SD card SCK, TX, RX, SDIO1, SDIO2 and CSN (`40` - `45`)
- cartridge ADC (`46`, `47`) 

### USB

Using jlcpcb impedance calculator and 1mm thick board with coplanar differential pair (coplanar because it is bordered by GND on the top layer as well), we get trace spacing of 0.2mm, trace to ground spacing of 0.254mm and trace width of 0.156mm for the USB required 90 Ohm impedance.

### I2S

I2S connection to the audio codec is done via LRCLK (FCLK), BCLK and DIN & DOUT pins for playback and recording respectively. Furthermore MCLK has to generate steady clock at 256x the framerate (LRCLK) for the codec to operate. 47Ohm terminating resistors are used for all the I2S lines and MCLK.

### SD Card

Although the SD card supports SPI protocol, in is not connected to any of the HW SPI pins. Instead connection scheme that will work for SDIO (with the data pins being sequential) is used and a dediacated SPI PIO will be used for the SPI comms. This allows device to later support the faster SD interface instead. 

SD card detect works ising internal pull-up on the `SD_CD` line. When no card is present, it will read high, but when card is inserted this is connected to ground and will read low. The internal pull-up is high enough to make the power draw for this negligible. 

### Display

[ST7789v](datasheets/ST7789V.pdf) in 16bit parallel MCU interface is used with FPC connector for easier assembly. The display uses both power supplies, `IOVDD` for the digital interface and `IOVDD` for backlight. This allows for a better load distribution (otherwise the `3v3` always on SMPS has not much to do most of the time with only the AVR connected). The display uses 16bit parallel interface to speed up the data transfer process, which is extremely useful especially in framebuffer applications. The RDX line is also available so that the CPU can read from the display as well (can be used in the future for visua effects). 

> TODO determine backlight resistor value 

## Programming & Debugging

The device provides 2 programming & debugging ports for the AVR and RP2350 chips. The RP2350 uses the standard 3pin SWD interface known from the Pico boards. The AVR chip has a 2 wire UPDI programming interface (UPDI & GND). Both are unpopulated, but as they use standard 2.54" spaced holes, they can be populated with connectors and are located in places where case cutouts can be made for easy access.

> TODO in the future, if we use I2C bootloader for the AVR chip, there should be no need for an user accessing UPDI connector any more, but maybe it is still convenient to have.

## Test Points

- `SDA` and `SCL` lines for I2C
- `INT/TX` for the AVR_INT/TX line to be able to debug the AVR chip
- [ ] all unconnected pins, so that we can do fixes on the board if necessary

## DFM & DFA

At this stage the PCB is still fairly prototype stage as there is a reasonably high amount of hand soldering and careful steps to manufacture & assemble the device:

- the biggest problem is probably the custom cartridge connector which consists of a separate board and has to be hand soldered. For a large run - perhaps a better spring contact can be found that can attach to the bottom of the PCB and can be soldered automatically with 2 sided assembly
- bottom side buttons - will be fixed by 2 sided assembly
- battery connector is again a custom PCB connected via hand soldered wires and connectors. If we get battery with standard connector, this too can be fixed by a bottom side soldered connector (another option would be to use cell phone batteries)
- speaker - requires soldering of the wires, maybe for plarge production can use speaker that has spring contects to the pcb (but how would the sound box work?)
- rumbler - can be had with connector in large quantities


Extras:

- [X] use [this](https://cz.mouser.com/datasheet/2/1628/sj_43504_smt_tr-3510743.pdf) for the headphone jack


