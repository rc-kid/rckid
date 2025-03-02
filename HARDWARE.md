# Hardware Design

> This is hardware description of RCKid mk III (version 19/1/25 and above). For brief history and to clear any confusion, mk I was much larger, hand soldered version powered by Raspberry Pi Zero 2W and mk II was slightly slimmer version powered by RP2040 with 8bit parallel screen interface.

RCKid is based on the [RP2030B](datasheets/rp2350-datasheet.pdf) (80 pin QFN) and uses its requirement for external program flash chip as a feature by allowing swappable cartridges. This is the overall architecture of the system:
          
                                              +-----------------+
                            + --------+       | Cartridge       |
                            | microSD |       | (flash, extras) |                                    
                            +---------+       +-----------------+                            +-------------+
                                   \\\\       ////                                           | TPA2006D    |
                          <SPI/SDIO>\\\\     ////<10pin/HSTX,QSPI>                           | (Mono AmpD) |--(spkr)
                                     \\\\   ////                                             +-------------+
                                     ||||   ||||                                            /
       +-----------+  <16bitMCU>   +-------------+     <I2S>       +---------------+       / (analog mono)
       |  ST7789V  |===============|   RP2350B   |=================| TLV320AIC3204 |-------
       | (Display) |               +-------------+                 | (audio codec) |_______   
       +-----------+                     ||                   //===+---------------+       \ (analog stereo)
              |                          ||                  //       |                     \         +------------+
              |                          ||                 //        | (analog stereo)      \--------| 3.5mm jack |
              |                    <I2C> ||================//         |                      /        +------------+
              |                          ||                      +------------+             / (FM atnenna)
              |                          ||======================|   Si4705   |------------/
              |                          ||                      | (FM Radio) |--- (embedded antenna)
              | <PWM>                    ||                      +------------+
              | (Backlight)              ||
              |                   +--------------+
              |                   |   TS5A23159  |
              |               ----| (analog mux) |====(gnd) when powered off
               \              |   +--------------+
                \             |          ||
                 \            |          || <I2C>     
                  \           |          ||           //======(buttons, 3x4 mtrix)
                   \          |   +-----------------+//
                    \         ----|    ATTiny3217   |
                     \------------| (IO, time, pwr) |----------(RGB)
                             /----+-----------------+
                            /            ||              +---------+
             (rumbler)-----/       <I2C> ||==============| BQ25895 |----(battery)
                          <PWM>          ||              | (PMIC)  | 
                                        //\\             +---------+
                                       //  \\
                                      //    \\
                             +---------+     +--------+
                             | BMI160  |     | LTR390 |
                             | (Accel) |     |  (ALS) |
                             +---------+     +--------+

Next to the control & logic elements above, there are also various power rails, their short description is below:

- `VUSB` is the +5V coming from the USB port, active when USB is connected, used for charging
- `VBATT` is the battery terminal 
- `VCC` is the system output from the PMIC chip. This is regulated to never fall below 3.5V and is processed by the 3 DC-DC converters below:
- `3V3` is step-down converter from `VCC` and is always on. This powers the ATTiny3217 and sensors (accel & als) and the I2C multiplexer
- `5V` is a charge pump to 5V required by the RGB lights (controlled by AVR)
- `IOVDD` is a step-down converter (identical to `3V3`) that provides power to RP2350, cartridge, display and audio systems (controlled by AVR) 

## I2C Bus

Most of the perihperals are connected to a single I2C bus (accesible on `TP6` and `TP7`). This poses a problem as when the device is turned off (`IOVDD` disabled), the pullups on `3V3` for the SDA and SCL lines would leak power to the RP2350 & friends. To solve this, an analog multiplexer ([TS5A23159](datasheets/ts5a23159.pdf)) on the I2C bus is used so that when power is turned off, the RP2350 end of the I2C is switched to ground pull-downs. When `IOVDD` is powered on, the I2C is switched to the valid bus. This also means that while the device is off, AVR is the master of the I2C bus and can talk to the always on peripherals (PMIC, sensors). 

When powered on, the RP2350 is the sole master on the I2C bus and other devices have means of signalling changes via interrupt lines. There is an interrupt line from the audio codec (headphone detection, heaset button press, etc.), one from the FM radio (frequency scan) and one from the AVR.

The AVR interrupt is used when either AVR wishes to signal (I/O, etc.), or when any of the interrupts connected to the AVR itself fire (Accelerometer, PMIC). 

### Sensors

Onboard accelerometer [BMI160](datasheets/BMI160.pdf) and ambient light & UV detector [LTR390](datasheets/LTR-390UV.pdf) are present. The accelerometer has its interrupt connected to AVR so that we can intercept taps, steps, etc. The sensors are always on, which does drain the battery a bit more, but allows the accel to work as pedometer as well.

> TODO is pull-up/down on the accelerometer interrupt pin required?

## Power Management

The PMIC used is [BQ25895](datasheets/bq25895.pdf). This is a fairly advanced IC that can charge battery really fast, provide 5V for USB OTG, has integrated ADC so that we can use it as battery monitor as well freeing some AVR pins and quite extensive I2C configuration & capabilities. This makes the circuit a bit complex:

- `VBUS` requires 1uF capacitor as close to the device as possible
- `D+` and `D-` are used to determine what type of charger has been connected. But as we do not use this functionality, they are shorted together which makes the device think it is dealing with USB_DCP (over 3A current capability) - from [reference design](datasheets/battery-charger-app-note.pdf) - available from [TI](https://www.ti.com/lit/an/sluaam6/sluaam6.pdf?ts=1736634296011&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FBQ25895)
- I2C is used for communication, pulled up by 3V3 (always on), interrupt line is also available to AVR. The interrupt has to be pulled-up and the datasheet recommends 10k pull-up value. AVR pull-ups are between 20-50k so they might not work and we need a dedicated external resistor.
- `STAT` pin is not used (LED indicator, can be left floating)
- `OTG` pin is connected to GND via 10k because OTG functionality is disabled
- `CE` is connected to GND (active low) because charging is enabled (and can in theory be disabled by SW)
- `ILIM` is used for HW limit of max input current (see below)) 
- `TS` is battery thermisor, which we do not have, this left at between REGN & GND
- `QON` is the shipping mode switch
- `BAT` goes to battery
- `SYS` goes to system output (with 20uF capacitors)
- `PGND` is GND
- `SW` is the switching node, goes to the inductor and 47nF bootstrap capacitor
- `BTST` is for the cap above to SW
- `REGN` needs 4.7uF (10V) to AGND (close to IC) 
- `PMID` is the output of 5V OTG, needs at least 8.1uF capacitance, or 40uF for at most 2.4V output. We have 2x22uF in case
- `DSEL` is used for USB data selection (PMID/MCU) to negotiate charging, can be left floated when not used

Inductor used is 2.2uH as per the datasheet, this time Vishay, another reputable brand is used with saturation current of 3A, which again should be well within the needs of the device.

The `ILIM` pin is used for HW setting of maximum input current. The way to calculate its value is taken from the datasheet, page 26, where: 

    Iinmax = Kilim / Rilim

For Kilim of max 390, the 270R value translates to maximum input current of 1.44A, which is within the specs for 0.5A charging current (see power consumption below). 

### 3.3V Step-Down converters

The design uses two identical step down converters - [TPS62825](datasheets/tps62825.pdf) (small 2x2 QFN). They should provide efficiency above 95% for loads over 10mA, and should work down to 100uA, which hopefully is enough for the sleeping AVR & sensors. They provide output current of 2A, which is well withing the requirements (1A max for IOVDD, 100mA for always on 3V3). They are adjustable versions and their output is regulated using the following equation (from datasheet page 12, eq. 4)

             R1 = R2 * (Vout / 0.6 - 1)
        R1 / R2 = VOut / 0.6 - 1

For VOut of 3.3V and R2 being the recommended 100k, we get the following:

    R1 / 100 = 3.3 & 0.6 - 1
    R1 / 100 = 5.5 - 1
          R1 = 450

Thus 450k and 100k resistors are used with 120pF feedforward capacitor for 100k resistors as suggested by the datasheet. The capacitors are used as per the datasheet, i.e. 4.7uF on input and 22uF on output with inductor of [470nH](https://jlcpcb.com/partdetail/Tdk-TFM201610ALCR47MTAA/C2045043), saturation current at 5.1 well above the switching frequency, from TDK, a respectable brand in hopes of avoiding coil whine).

The EN pin must be high for the converter to work, so for `3V3` rail we short it to input voltage. For `IOVDD` it is controlled by the AVR, which runs at 3.3V but this is ok as the datasheet specifies that voltages above 1V on the EN pin are logic high (page 5).

> There is an unpopulated resistor R128 of 33k that when fitted ensures 100uA current drain on the 3v3 rail. Its pad can also serve as a testpoint.

> TODO Do we need a resistor from 3v3 to ground (100k) to ensure regulator would work 

### 5V Charge Pump

We are using [HX4002](datasheets/hx4002.pdf) charge pump, same as in mk II. The pump is capable of delivering in excess of 240mA for 3.6V input voltage (it runs from VCC, which is stepped up above 3.3v). It only powers the RGB leds, which take at most 12mA per channel, which gives us 6.6 RGBs at full power.

WS2812 compatible, but smaller & less powerful LEDs are used. As they require 5V and the AVR that controls them runs on 3.3V we use a converter.

> TODO should the single LED in DPAD be replaced so that a proper DPAD with tilting can be used? If we use multiple LEDs we will be exceeding the power generator output, but it is likely fine, as currently the LEDs run at 12.5% of full power and seem to be fine.

### Running on Batteries

The idea is that when running on batteries (3x AAA for 4.5V), the whole PMIC circuitry will be left unpopulated, while the bottom side of the board will be populated with a VUSB / VBATT switch as was in the mkII and a battery voltage voltage divider connected to the PWR_INT pin (which is available on ADC0). 

The voltage divider should use 300k and 570k resistors to transform 5V to 3.296V. 

> TODO those registers are really large and maybe the ADC on ATTiny won't be able to read the voltage - should check. 

Since we do not have the PMIC to ensure VSYS is never below 3.3V, the step down converters will stop operating when battery voltage drops below 3.3V (or they will output lower voltage, which might be fine for a while). Anyways, for NiMH this should not be much of a problem as the discharge curve is extremely flat. 

NOTE when running on batteries, the PWR_INT pull-up resistor must not be fitted!

> TODO For alkaline batteries, this could pose a problem as they discharge all the way down to 0V, but this can be solved by extra board with boost converter I presume. 

### Power Consumption

At worst case we expect relatively negligible mA on 3V3 and close to 1A on IOVDD, which for the sake of simplicity is 1A of 3V3 + 240mA on 5V for the neopixels. Assuming neopixel charge pump efficiency of 80% and DC-DC converter efficiency of 90%, we get:

    MAX = 1000 * 3.3 / 0.9 + 240 * 5.0 / 0.8 = 5166

However, in reality, we expect about 300mA for the system (IOVDD) and 50mA for the 5V charge pump, i.e:

    EXP = 300 * 3.3 / 0.9 + 50 * 5.0 / 0.8 = 1412

This leads to the following current draw for different voltages:

    Input Voltage | Imax   | Iexp   |Comments
    --------------|--------|--------|---------------
    3.0           | 1700mA | 470mA  | Battery depleted (worst case before shutdown)
    3.7           | 1400mA | 381mA  | Battery nominal
    4.2           | 1230mA | 336mA  | Battery fully charged
    5.0           | 1033mA | 282mA  | USB attached

> NOTE that while this is well withing the switching capabilities of the power components used, it does exceed the 1C discharge rate for the battery so the worst case might be a problem. If we assume 2C, then everything is allright. Also note that this does not take into account the boost converter employed by the PMIC chip which will also increase the battery drain at lowest settings. 

## Audio

[TLV320aic3204](datasheets/tlv320aic3204.pdf) is used, which is yet another fairly complex chip. It is controlled via I2C and RESET line (the RESET line is necessary), interrupt line and 4 more pins for I2S playback and recording (BCLK, WCLK, DIN and DOUT). It provides integrated headphone amplifier, line out and three analog stereo inputs (or 3 floating inputs). The chip also contains integrated LDO to power the core & analog parts that require smaller and cleaner voltages. Furthermore the chip is used for headphone detection and speaker on/off via a GPIO. Pinout:

- `MCLK` master clock is not used (TP3)
- `BCLK`, `WCLK`, `DIN` and `DOUT` -> I2S connection to RP2350B
- `IOVDD` is IO voltage power supply, 3.3V with 1uF to GND (from typical app, page 40)
- `IOVSS`, `AVSS`, `DVSS` is ground
- `MFP3` is headphone detect input (see below)
- `SCL`, `SDA` for I2C communication
- `MFP4` is AUDIO_INT to RP2350 (can be routed to either INT1 or INT2)
- `SPI_SELECT` goes to ground (to disable SPI, enable I2C)
- `IN1L` and `IN1R` are connected to the embedded microphone (differential)
- `IN2L` and `IN2R` are connected to the FM radio (single ended)
- `REF` is connected to ground via 10uF (from typical app, page 48)
- `MICBIAS` is microphone bias used for headset mic, headset detection and embedded mic
- `IN3R` is connected to headset microphone
- `IN3L` is unused, connected to GND via 0.1uF capacitor (datasheet page 42)
- `LOL`, `LOR` is line output, which is connected to speaker amplifier (see below)
- `AVDD` filtered analog voltage (using internal LDO), 10uF to GND
- `HPL` and `HPR` go to headphones via 220uF capacitors. For headphone impedance of 32Ohm, this forms a high pass filter with cutoff frequency at 22Hz
- `LDOIN` is input voltage for the internal linear regulator, connected to 3.3V and heavily filtered (0.1, 1 and 10 uF)
- `DVDD` is digital (core) voltage, supplied by the internal LDO, here only filtered with 10uF to ground
- `LDO_SELECT` enables the internal LDO so that we can run from single power source, connected to 3.3V
- `RESET` is chip reset, active low, connected to RP2350
- `GPIO` is used as GPIO pin that turns the speaker amplifier on and off (controlled by the RP2350)

> TODO de-pop circuit (?) see page 45
> TODO Typical app uses only 47uF capacitors on the headphone lines, but that is cutoff at 100Hz, isn't it too high?
> TODO current limiting resistors on I2S lane, see page 45
> TODO audio reset can perhaps be accomplished by radio GPIO as this is not very useful function.

> TODO capacitors are bit different between RCKid and app note: (https://www.ti.com/lit/ug/slau266a/slau266a.pdf?ts=1740243769370&ref_url=https%253A%252F%252Fwww.ti.com%252Ftool%252FTLV320AIC3204EVM-K)

AVDD - 0.1 & 22 app note, I have 10uF
HPVDD 0.1 & 22 app note, I have 0.1 1 10
DVDD 01 & 22 app note, I have 10 uF
IOVDD - 0.1 and 10, I have 1uF
REF - 01 and 10, I have 10uF

#### Microphone

As per the typical application, the microphone is connected differentially to `IN1L` and `IN1R` with 1k from MICBIAS to positive mic, 0.1uF capacitor on the positive output to right channel and 1k to ground on the negative channel, which also goes to the left input via 0.1uF capacitor.

> Is the micbias resistor correct?

#### Headset detection & Microphone

> TODO

### Speaker Amplifier

We use [TPA2006](datasheets/tpa2006.pdf), which is a mono class-D amplifier with 1.45W output. This is fine as the speaker used is only rated for 0.7 amps, so we are well within the specification. As per the datasheet (page 15) no ferrite beads or other filtering is necessary on the output for speaker lines below 10cm. 

The input resistors set the gain of the amplifier (page 20). 

> TODO what gain should be used?
> TODO ensure that audio codec can output mono differentially on the line output

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

> TODO do we need pull-up on INT line? TP4 can be used to test, and internal pull-up on RP2350 can be used if needed
> TODO do we need an LDO for 3v3 for the VA of the radio or can single source be used at all? The example module uses ferrite bead and capacitors instead? 

### Headset Antenna

The headset antenna is described in the antenna datasheet, page 19. Headset ground is connected to gnd via 270nH inductor as per the datasheet and to the FMI pin via a 100pF capacitor.

> TODO ESD protection diode
> TODO the module has additional 6.8nH inductor from the gnd to the pin as well as a larger, 1nF capacitor. Why? 

### Embedded antenna

As per the antenna application note, page 31, the embedded antenna is connected directly to the LPI pin win 120nH inductor to the ground. The inductor is 0805 so that it's hand-solderable. The ESD protection and current limiting resistor are not necessary as there is no exposed connector to the embedded antenna. 

> TODO the module above also uses a 100pF capacitor between the pin and the antenna connector, why? 

### External Crystal

We are using the same crystal as AtTiny3217. The load capacitance is 12.5 pF, which is roughly in the middle of the rangle per datasheet. The 22pF capacitors are used in the datasheet, but also correspond to a relatively small stray trace capacitance ([online calculator](https://ecsxtal.com/crystal-load-capacitance-calculator/?form=MG0AV3))

> TODO the external crystal is the same as for ATTiny3217. Is that ok? Should the capacitors have different values?

### Output to the audio codec

The audio output is single ended stereo channels have a Vrms of max 90mV (datasheet page 14). They are connected through 10k resistors to ground and via a 100nF capacitors to the analog input of the audio codec.

> TODO this is likely wrong and seems to be taken from the analog output information instead. Must be calculated properly for the audio codec & used gain, etc.
> TODO what is the DC bias? (1/2 VCC?)

## AVR (ATTiny3217)

[ATTiny3217](datasheets/ATTiny3217.pdf) is used in QFN package and is responsible for power management, input & output (buttons, rumbler and display PWM) and some sensors (RTC clock, temperature sensor). All pins are used, with serial TX being routed to the debug port so that serial output (or simple pin) can be observed (unlike mkII where debugging the AVR was pain). The chip also runs from 3.3V as well, so it can't use VCC measurement to check battery levels (instead this is provided by the PMIC chip). Fixed pinout is as follows:

- display backlight PWM on `PC0` (TCB0 WO alt)
- rumbler PWM on `PA3` (TCB1 WO)
- I2C SDA and SCL on `PB0` and `PB1` (default)
- AVR_TX on `PA1` (UART TxD alt)
- 32.768kHz ext oscillator at `PB2` and `PB3`
- PWR_INT at `PA2` (which is AIN2 for ADC0 in AAA battery mode)

Remaining functionality is interchangeable as they are used simple digital pins, namely:
- BTN_1, BTN_2, BTN_3 and BTN_4 for button matrix rows
- BTN_DPAD, BTN_ABXY, BTN_CTRL for button matrix cols
- ACCEL_INT (input from accel)
- AVR_INT (output to RP2350)
- VDD_ON and 5V_ON for IOVDD and 5V power rails
- RGB for neopixel control
- QSPI_SS, connected to RP2350 QSPI_SS pin to boot into bootloader and/or sense cartridge presence

### Buttons

Due to low number of pins available, a classic button matrix is used. Buttons are grouped in three columns: DPAD, ABXY (A B Sel and Start, historically) and CTRL (home, vol up & vol down). When the device is power off, the CTRL is the only active group so that Home button interrupt works.

> TODO this is different from mk II - check that home button in the matrix & wakeup works reliably)

### RTC

In mkII the internal oscillator was used for RTC which led to inaccuracies of up to 10 minutes per day. This time we are using external 32.768 kHz oscillator for better accuracy. The crystal load capacitance per the datasheet (page 511) is max 12.5pF which is the value used, so the same circuit as for the radio should work. 

> TODO verify cryctal and capacitors

### Rumbler

Rumbler is connected to IOVDD and triggered by AVR PWM. Protective diode is used as well. 

> TODO it's on the opposite side of the RP IOVDD, and will draw large amounts of power when used - perhaps it could run off from the 3V3 rail instead, which uses much less mA for better system stability? Or add some capacitors?

## RP2350B

[RP2350B](datasheets/rp2350-datasheet.pdf) - 2 Cortex M33 core @150Mhz, 520KB RAM, PIO and so on. Arguably, there are chips that are more powerful (high-end STM, ESP32 would provide WiFi for "free"), but none I have tried beats the fun of programming RP2350:) So I am sticking with it. 

All pins are used (48), but in mkIII with FPC display connector, there is theoretical room for some savings as we are no longer bound to 16bit MCU interface:

- 1 second QSPI SS on cartridge, also analog in (pin 47)
- 6 pins for SD card (SPI & SDIO)
- 5 pins for display control (write *and* read, tearing effect)
- 16 pins for display data
- 8 pins for cartridge (also HSTX, pins 19 - 12)
- 1 pin for cartridge (pin 11)
- 6 pins for audio (reset, int, I2S in & out)
- 2 pins for radio (reset, int)
- 2 pins for I2C
- 1 pin for AVR IRQ

> TODO Pin 11 is not special, we might exchange it for say pin 46 and move the rest to provide 2 ADC pins on the cartridge, but it simplifies the wiring - but might be worth it.
> TODO The VCC and GND should be organized as GND VCC GND VCC GND so that only 2 pins (1 gnd and 1 vcc) can be used for the smallest cartridges
> TODO the QSPI SS pull-up can be part of the cartridge to enable the AVR to check that cartridge is present (there can be a 0805 DNF resistor on the board to enable both ways?)

Further details of how the HW requirements for the chip should look can be found in [HW Design Notes](datasheets/hardware-design-with-rp2350.pdf).

### SD Card

> TODO card presence check

### Display

[ST7789v](datasheets/ST7789V.pdf) in 16bit parallel MCU interface is used with FPC connector for easier assembly. 

> TODO figure out the IM settings from the datasheet to enable the correct interface. This is also change from mk II where we used 8bit MCU. Technically, 8bit MCU can also be used on the FPC display version, but the 16 bit interface should fill the screen faster leaving more time to draw.
> TODO should the backlight voltage be larger than 3.3V? There is a space on the board for a charge pump or boost

## Test Points

Top side:

- `TP2` - VUSB

Bottom side:

- `TP1` - QON pin of PMIC chip (connect to ground for battery reset & exit from the shipping mode)
- `TP3` - MCLK for audio codec
- `TP4` - RADIO_INT
- `TP5` - GPO1 for radio chip (GPIO, protocol select between I2C and SPI)
- `TP6` - I2C_SDA
- `TP7` - I2C_SCL


# Development Board

The development board is a 10x10cm 4 layer board that mimics the basic layout and connections of the rckid mk3 device, but with simpler components (buttons, RGB lights, etc.), larger passives so that they can be manually tweaked to determine correct values and far more test points. 

- [X] accel interrupt pullup?
- [X] ilim resistor for PMIC (270 atm)
- [X] 100kOhm R130 upopulated (ensure 3v3 even when off load)
- [ ] R3 100k to ground converter why?
- [X] pwr-int resistor can we do without it? 
- [X] audio de-pop circuit
- [X] headphone capacitors 47 uF for typical app?
- [X] gain resistors for speaker amplifier
- [X] pull-ups on radio int? 
- [X] 3v3 input for the audio
- [X] radio & avr external crystal capacitors
- [X] the whole radio to audio input circuit is likely wrong and has to be thought out
- [X] micbias resistor? 

Extras:

- [X] solderable display (on separate PCB)
- [X] get ready for analog vcc being from an ldo
- [x] 2C pull-ups
- [X] SD card detection
- [X] radio GPIOs 
- [X] use [this](https://cz.mouser.com/datasheet/2/1628/sj_43504_smt_tr-3510743.pdf) for the headphone jack
- [ ] the slots don't work, update the pcb with holes
- [ ] add top & bottom GND planes and connect the AGND and GND so that it can be cut on the bottom to leave only one 

Extra PCBs:

- [ ] figure out the hold in place mechanism for the cartridge if the friction of the connectors is not enough
- [ ] make the PCB larger to fit the entire case, but make sure no components, or useful stuff is above 10cm so that we can always skip it -- this means moving the rpi 1.5mm down(!)


# Things to check on the prototype

- can the radio extra GPIO be used for audio reset? (this saves one pin on RP2350)
- this can then become SDcard detection
- avrTX pin can be used as cartridge detect of needs be


