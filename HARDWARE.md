# Hardware Design

> This is hardware description of RCKid mk IV (version 17/10/26 and above). For brief history and to clear any confusion, mk I was much larger, hand soldered version powered by Raspberry Pi Zero 2W and mk II was slightly slimmer version powered by RP2040 with 8bit parallel screen interface. MkIII used RP2350 for the CPU, but kept using the AtTiny3217 for the secondary always on IO MCU. 

RCKid is based on the [RP2350B](datasheets/rp2350-datasheet.pdf) (80 pin QFN) and uses its requirement for external program flash chip as a feature by allowing swappable cartridges. This is the overall architecture of the system:
          
                                              +-----------------+
                            + --------+       | Cartridge       |
                            | microSD |       | (flash, extras) |                              
                            +---------+       +-----------------+      
                                   \\\\       ////                   
                          <SPI/SDIO>\\\\     ////<8pin/HSTX,2/ADC,QSPI>
                                     \\\\   ////                                    /- speaker
                                     ||||   ||||                                   /   (analog mono)
       +-----------+  <16bitMCU>   +-------------+  <I2S>  +---------------+      /
       |  ST7789V  |===============|   RP2350B   |=========| NAU88C22GY    |------
       | (Display) |---------------+-------------+         | (audio codec) |_____   
       +-----------+ <PWM backlight>     ||           //===+---------------+     \   (analog stereo)
                                         ||          //        \                  \  +------------+
                                         ||         //          \                  \-| 3.5mm jack |
                                         ||========//            \--(mic)            +------------+
                                         ||
                                         || <I2C>     
                                         ||
                                  +-----------------+
                                  |    PY32F030     |
                  (rumbler)-------| (IO, time, pwr) |======(buttons, 3x4 matrix)
                                  +-----------------+
                                    |||     |   \  \--- (RGB LED)
                              <SPI> |||     |    \
                                    |||     |     \---- (LED, sense)
                                    |||     |    
                            +----------+   +-----------+ 
                            | SC7A20   |   | MCP73832  |---(battery)
                            | (Accel)  |   | (charger) |
                            +----------+   +-----------+

Next to the control & logic elements above, there are also various power rails, their short description is below:

- `VUSB` is the +5V coming from the USB port, active when USB is connected, used for charging
- `VBATT` is the battery terminal 
- `VCC` is the main system rail. This comes either from the battery, or from the USB when connected. Valid voltages are from 3v to 5v. This rail is measured by the IO MCU to determine battery status / USB presence and is fed to a DC-DC converter below:
- `3V3` is the main system voltage, always on, 3.3V from DC-DC converter
- `IOVDD` is 3V3 after a switch that powers the RP2350, Display, Cartridge, SD card and the audio system (i.e. the device)

## I2C Bus

The RP2350 uses I2C bus to communicate with the IO MCU and the audio codec chip. The I2C bus is powered from the `IOVDD` and so only works when the device is powered on. The IO MCU pulls the lines to ground when turning device off to prevent leakage.

### Sensors

The only extra digital sensor in mkIV is the onboard acceleromeneter [SC7A20](). This is a very basic cost effective accelerometer with no additional functionality. It is always on, powered from the `3V3` rail as the IO MCU. It communicates with it using SPI to stay clear of the I2C bus that does not work when the device is turned off.

Furthermore several of the devices connected to the I2C bus also supoort temperature measurements (including the PY32F030 itself).

> The ambient light detector from previous versions has been replaced with a reverse connected single LED controlled by the IO MCU. 

## Power Management

Power management consists of a standalone Li-Ion battery charger [MCP73832](datasheets/MCP73832.pdf) and a power path via a MOSFET and diode that enables DC power when connected, and battery otherwise. 

> MCP73832 *must* be used as the IO MCU runs on 3.3V while the charger runs at 5V so outputting charger high when charge is complete as the 73831 does could destroy the chip. There is no such danger with MCP73832 as the STAT pin behavior is HiZ when not charging and low when charging.

The charger pinout is as follows:

- `VDD` is connected to the VUSB (5v)
- `STAT` is the charger state output (low when charging, floating otherwise)
- `VBAT` goes to the battery
- `PROG` is connected to `2k2` resistor for `???` max charge current

The IO MCU monitors the `VCC` rail (after power path), which enables it to determine battery voltage when powered from battery (value <= `4.2V`) and if USB is connected (VCC above `4.2V`). This is done through a `100k/200k` voltage divider that is always on. This seeps negligible battery power, and while being rather hard impedance for ADC input, the voltage signal is very slow changing so we should be fine. Charger status is monitored via the `STAT` pin that is connected to a `100k/200k` voltage divider on the `VUSB` (so that it is pulled high at `3v3` when USB connected, `0` otherwise). As per the datasheet, the `STAT` pin is floating when not charging and goes to `0` while charging. This gives the following power scenarios:

- `VCC<=4.2V` battery is connected, `STAT` should be 0 (but can really be ignored)
- `VCC>4.2V` and `STAT` is low means USB is connected and the device is charging (pulled down by the charger IC)
- `VCC>4.2V` and `STAT` is high means USB is connected, but charging is done (via the ladder)

> TODO add 0.1uF cap from VCC sense midpointto gnd to smooth the measurements.

### Li-Po Battery

By default the device runs from a LiPo battery such as [503759](https://www.aliexpress.com/item/1005007102975858.html). Important is the JST-PH2 connector and cable length which make it simple to use the battery with the PCB and case dimensions. Those batteries come at different capacities from 1200 to 1400mAh, we are assuming the lowest - 1200mAh for the rest of the document.

The battery supports continuous discharging at 1C. 

### Power Envelope

Below is the expected power envelope for the system:

- 100mA for the RP2350B (overclocked 250MHz, both cores)
- 100mA for the display (full brightness)
- 150mA for the audio (speaker at full reasonable volume)
- 72mA for all white RGB leds powered at 25%
- 500mA for the cartridge (budget)

This gives total 1A per the device with some slack. 500mA for the device itself and 500mA for the cartridge. That said, the normal budget is a *lot* lower. Bench testing shows draw at the 3.7V to be around 130mA for normal use without fancy cartridges.

> TODO verify this number with the newest prototype.

### 3.3V Buck-Boost converter

Single DC-DC buck-boost converter is used - [TPS631000](datasheets/tps631000.pdf). As per the datasheet, the converter is capable of 1.5A of 3v3 output from voltages as low as 2.7V, which is above the 1A we need from the power budget. At this level the efficiency is at 90%, we overshoot the 1C discharge limit by 2%:

    1000 / (3.0 / 3.3) * 0.9 = 
    1000 / 0.818181818 =
    1222

This is acceptable as the absolute worst case scenario since the budget very generous.

The converter is variable output resistor configuration straight from the datasheet is used for `3v3` output. So is the inductor and the recommended layout. The converter is always on as it powers the IO MCU, accelerometer and other always on circuitry. 

### IOVDD Switch

To enable the device, the IO MCU must power on the `IOVDD` rail that powers the RP2350, cartridge, display and audio subsystems. Since this powers also the cartridge, which is external to the device, this is done by the [TPS2553D](datasheets/tps2553d.pdf) USB switch, which is rated up to 1.7A. 

The switch also functions as a current limiter, set to the following value (page 15):

    Inorm = 22980 / ( Rlim ^ 0.977)
    Inorm = 22980 / ( 27 ^ 0.977)
    Inorm = 22980 / 25.03
    Inorm = 918 [mA]

Which is around the limit expected from the cartridge, display, RP2350 and audio. This ensures that cartridge shorts are protected as well as large current inrush on poweron for large capacitance cartridges.

The switch is connected to the IO MCU via two pins, one for control and one for reading a fault state so the IO MCU can determine cartridge problem and act accordingly.

### Neopixels

The device uses 8 neopixel RGB LEDs [WS2812B](datasheets/WS2812B.pdf), which offer three significant benefits over the most common WS2812 parts:

- they support running from 3.3V, which removes the need for the 5V charge pump previous used
- standby current is only 1uA which removes the need for switchable power rail
- max power draw is 36mA per LED, 12 per channel

## Audio

[NAU88C22GY](datasheet/nau88c22gy.pdf) chip is used, which is relatively simple, yet it offers all the features we need - 16bit audio playback & record, line-in for the radio, microphone input, integrated headphone and mono speaker (BTL) drivers. While the chip features audio jack detection and can automatically enable/disable headphones based on the jack status, it does not offer any way to report the jack status, hence the jack detection pin is shared with RP2350 which can read it. The chip also provides no interrupt (no events available) and provides one simple digital GPIO that can be set high or low (not used). In terms of I2S, the chip requires MCLK to be provided, but it does not have to be of the exact frequency needed as the internal PLL can adjust it - but we are not using this feature. Pinout:

- `VDDA`, `VDDB`, `VDDC` and `VDDSPK` are all connected to the `IOVDD` 3v3 rail for simplicity, decoupled with `4u7` caps
- `VSSA`, `VSSD` and `VSSSPK` are all connected to `GND`
- `SCLK` and `SDIO` are connected to `SCL` and `SDA` respectively via a `220` resistor and `33pF` capacitor, as per the datasheet to add filtering to the lines
- `FS`, `BCLK`, `ADCOUT` and `DACIN` are connected to the `I2S` bus
- `MCLK` is connected to the master clock generated from the RP2350
- `LHP` and `RHP` are connected to headphones via large `220uF` capacitors
- `LSPKOUT` and `RSPKOUT` are connected to directly to the speaker in BTL mode
- `LMICP` and `LMICN` go to positive and negative microphone terminals via `1uF` capacitors (as per the datasheet)
- `MICBIAS` is used as a power source for the MEMS microphone (so no biasing resistor is needed)
- `RMICP`, `RMICN`, `AUXOUT1`, `AUXOUT2`, `RAUXIN` and `LAUXIN` are left floating (as per the datasheet for unused analog inputs/outputs)
- `GPIO2` goes to headset detection
- `GPIO3` is tied to ground as per the dataset for unused gpio
- `MODE` is tied to ground for I2C selection

> Note that I2C communication with the chip did not work when MCLK was being generated on the breadboard. This was fixed by adding the RC filter to I2C lines as per the datasheet (220R, 33pF). 

> TODO GPIO1 

#### Microphone

Since we are using MEMS microphone, not electret, the `MICBIAS` is used simply as a power line to the microphone. The microiphone is connected between `GND` (connected toi `LMICN`) and `LMICP` via `1uF` capacitors. 0.1uF capacitor is next to the microphone for smoothness.

> TODO Verify the microphone works

#### Headset detection

Headset detection is via a `10k` pull-up resistor (to `IOVDD`) connected to the second ground terminal of the headphone connectr. This will read high when headphones are not detected and will go to `0` when headphones are inserted as the ground will be connected to both terminals. The headset detection line is then fed to the audio codec for automatic speaker/headhone switching and to the RP2350 as well to determine the headphone status. 

We use two switch, 3 contact [headset connector](datasheets/headphone%20jack.pdf), half sinking for better profile. 

## IO MCU

We are using the [PY32F030]() which is much cheaper than the originally used AtTiny3217, while providing nearly the same functionality. The chip has a rather complex pinout, with analog & special lines added first and the digital rest filled later to simplify trace widths. This gives us the following pin assignment:

- I2C communication with RP2350 on `PB6` and `PB7`
- SPI communication with the accelerometer on `PB5`, `PB4`, `PB3` and `PA15`
- rumbler PWM via `PA11`, which corresponds to `TIM1_CH4`
- RGB led via `PA2`, which corresponds to `TIM3_CH1`
- VCC sense on `PB0`, i.e. `ADC_IN8`
- LED for light sensing via `PF0` and `PA0` (`ADC_IN0`)
- external crystal for RTC (`PA9` and `PA10`)
- debug & sys BOOT0, NRST, TX, SWD, SWCLK (`PF4`,`PF2`,`PF3`,`PA14`,`PA13`), where the TX corresponds to `USART1_TX`

Remaining functionality (simple digital pins):

- button matrix rows BTN_1 - BTN_4 (`PA4`, `PA1`, `PA12` and `PA8`)
- button matrox columns BTN_DPAD, BTN_CTRL, BTN_ABXY (`PA6`, `PB2` and `PA7`)
- accelerometer interrupt at `PB8`
- CHARGING detection (low when not charging, high when charging) at `PB1`
- IOVDD_EN at `PF1` and IOVDD_FAULT (input) at `PA5`
- QSPI_SS, connected to RP2350 QSPI_SS pin to boot into bootloader at `PA3`

### Buttons

Due to low number of pins available, a classic button matrix is used. Buttons are grouped in three columns: DPAD, ABXY (A B Sel and Start, historically) and CTRL (home, vol up & vol down). When the device is power off, the CTRL is the only active group so that Home button interrupt works.

### RTC

In mkII the internal oscillator was used for RTC which led to inaccuracies of up to 10 minutes per day. This time we are using external 32.768 kHz oscillator for better accuracy. 

> TODO Determinal crystal load as per datasheet and calculate

### Light sensor

> TODO this is done by a reverse diode that is sampled via internal ADC

### Rumbler

Rumbler is connected to 3V3 and triggered by IO MCU PWM. Protective diode is used as well. 

## RP2350B

[RP2350B](datasheets/rp2350-datasheet.pdf) - 2 Cortex M33 core @150Mhz, 520KB RAM, PIO and so on. Arguably, there are chips that are more powerful (high-end STM, ESP32 would provide WiFi for "free"), but none I have tried beats the fun of programming RP2350:) So I am sticking with it. Further details of how the HW requirements for the chip should look can be found in [HW Design Notes](datasheets/hardware-design-with-rp2350.pdf). Basic pinout is below, for more details see the desciption of subsystems below

- I2C SDA & SCL (gpio `0` and `1`)
- RP_TX at `10`, which is `UART1_TX` (via F11) to stand clear of UART0 which is available on the cartridge pins
- PWM_BACKLIGHT at `2` (`PWM1`)
- I2S MCLK at `4` (its own pio sm)
- I2S DIN, DOUT, BCLK and LRCLK (`5`, `6`, `7`, `8`) (record & playback sms)
- HEADSET_DETECT (`3`)
- SD_CD (card detect) `9`
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

The SD card is protected with ESD diodes and has pull-ups on `RX`, `TX`, `CSN`, `SDIO_1` and `SDIO_2` as per the specification (10k). 

### Display

[ST7789v](datasheets/ST7789V.pdf) in 16bit parallel MCU interface is used with FPC connector for easier assembly. The display uses the `IOVDD` power rail for both logic and backlight (which is controlled by the RP2350). The display uses 16bit parallel interface to speed up the data transfer process, which is extremely useful especially in framebuffer applications. The RDX line is also available so that the CPU can read from the display as well (can be used in the future for visua effects). 

For backlight, we use [AP2502](datasheets/AP2502), which can sink up to 80mA of current in total of 4 channels. This is less bright than the max of 100mA recommended by the display datasheet, but is still comfortably high. 

## Cartridge Connector

> TODO describe

## Programming & Debugging

Two standardized SWD debug ports are provided, one for the RP2350 and one for the PY32F030. They contain the ARM SWD (clock & data), GND, UART TX for debug prints and RST signal. They can be either populated with connectors (precision milled headers), for which there is room left in the enclosure, or used with pogo pins. 

Both connectors have identical layout.

## Test Points

- `SDA` and `SCL` lines for I2C

# Appendix A - Running from AAA batteries

It is technically possible to run the device from 3 AAA batteries instead of the li-po cell. This requires changing the IO MCU firmware to accomodate for the lack of charging behavior and different voltage ranges as 3 AAA batteries provide anywhere from 4.5 to a bit below 3.0V on their output. The li-ion charger must not be soldered. 

As we keep the power path switch between USB and batteries, if the batteries are replaced while the USB is powered there should be no power loss. 

Mechanically I was thinking of running two small pcbs with clips perpendicular to the bottom of the PCB with battery contacts being soldered to the current battery terminals. 