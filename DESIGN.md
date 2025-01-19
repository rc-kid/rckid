# Hardware Design

> This is hardware description of RCKid mk III (version 19/1/25 and above). For brief history and to clear any confusion, mk I was much larger, hand soldered version powered by Raspberry Pi Zero 2W and mk II was slightly slimmer version powered by RP2040 with 8bit parallel screen interface.

RCKid is based on the [RP2030B](datasheets/rp2350-datasheet.pdf) (80 pin QFN) and uses its requirement for external program flash chip as a feature by allowing swappable cartridges. This is the overall architecture of the system:
          
                    microSD    Cartridge
                         \\    //                  / -------- TPA2006D ------ Speaker
                          \\  //                  /           (Mono AmpD)
                           \\//                  /
       ST7789S ========== RP2350 ==== TLV320AIC3204 --------- 3.5mm Jack
       (Display)            ||          (AudioCodec)
               \            ||        /       |   \
                \           ||-------/        |    \--------- Microphone
                PWM         ||                |
              (Backlight)   ||------------ Si4705
                   \        ||           (FM Radio)
                    \   TS5A23159
                     \  (I2C mux)    WS2812B (RGB)
                      \     ||  |   /
                       \    ||  |  /
                        \   ||  | /
           Buttons ==== ATTiny3217  ========= BQ25895 ------ 3.7 LiPo
        (3x4 matrix)    (IO, Power)           (PMIC)
                         //   |  \\
                        //    |   \\
                       //     |    \\
                  BMI160      |    LTR390
            (Accelerometer)   |    (ALS)
                              |
                             PWM
                           (Rumbler)

Next to the control & logic elements above, there are also various power rails, their short description is below:

- `VUSB` is the +5V coming from the USB port, active when USB is connected, used for charging
- `VBATT` is the battery terminal 
- `VCC` is the system output from the PMIC chip. This is regulated to never fall below 3.5V and is processed by the 3 DC-DC converters below:
- `3V3` is step-down converter from `VCC` and is always on. This powers the ATTiny3217 and sensors (accel & als) and the I2C multiplexer
- `5V` is a charge pump to 5V required by the RGB lights (controlled by AVR)
- `IOVDD` is a step-down converter (identical to `3V3`) that provides power to RP2350, cartridge, display and audio systems (controlled by AVR) 

## I2C Bus

Most of the perihperals are connected to a single I2C bus. This poses a problem as when the device is turned off (`IOVDD` disabled), the pullups on `3V3` for the SDA and SCL lines would leak power to the RP2350 & friends. To solve this, an analog multiplexer ([TS5A23159](datasheets/ts5a23159.pdf)) on the I2C bus is used so that when power is turned off, the RP2350 end of the I2C is switched to ground pull-downs. When `IOVDD` is powered on, the I2C is switched to the valid bus. This also means that while the device is off, AVR is the master of the I2C bus and can talk to the always on peripherals (PMIC, sensors). 

When powered on, the RP2350 is the sole master on the I2C bus and other devices have means of signalling changes via interrupt lines. There is an interrupt line from the audio codec (headphone detection, heaset button press, etc.), one from the FM radio (frequency scan) and one from the AVR.

The AVR interrupt is used when either AVR wishes to signal (I/O, etc.), or when any of the interrupts connected to the AVR itself fire (Accelerometer, PMIC). 

## Buttons

Due to low number of pins available, a classic button matrix is used. Buttons are grouped in three columns: DPAD, ABXY (A B Sel and Start, historically) and CTRL (home, vol up & vol down). When the device is power off, the CTRL is the only active group so that Home button interrupt works.

> TODO this is different from mk II - check that home button in the matrix & wakeup works reliably)

## Display

[ST7789v](datasheets/ST7789V.pdf) in 16bit parallel MCU interface is used with FPC connector for easier assembly. 

> TODO figure out the IM settings from the datasheet to enable the correct interface. This is also change from mk II where we used 8bit MCU. Technically, 8bit MCU can also be used on the FPC display version, but the 16 bit interface should fill the screen faster leaving more time to draw.
> TODO should the backlight voltage be larger than 3.3V? There is a space on the board for a charge pump or boost

## Sensors

Onboard accelerometer [BMI160](datasheets/BMI160.pdf) and ambient light & UV detector [LTR390](datasheets/LTR-390UV.pdf) are present. The accelerometer has its interrupt connected to AVR so that we can intercept taps, steps, etc. The sensors are always on, which does drain the battery a bit more, but allows the accel to work as pedometer as well.

> TODO is pull-up/down on the accelerometer interrupt pin required?

## Rumbler

Rumbler is connected to IOVDD and triggered by AVR PWM. Protective diode is used as well. 

> TODO it's on the opposite side of the RP IOVDD, and will draw large amounts of power when used - perhaps it could run off from the 3V3 rail instead, which uses much less mA for better system stability? Or add some capacitors?

## 3.3V DC-DC converters

The design uses two identical step down converters - [TPS62825](datasheets/tps62825.pdf) (small 2x2 QFN). They should provide efficiency above 95% for loads over 10mA, and should work down to 100uA, which hopefully is enough for the sleeping AVR & sensors. They provide output current of 2A, which is well withing the requirements (1A max for IOVDD, 100mA for always on 3V3). They are adjustable versions and their output is regulated using the following equation (from datasheet page 12, eq. 4)

             R1 = R2 * (Vout / 0.6 - 1)
        R1 / R2 = VOut / 0.6 - 1

For VOut of 3.3V and R2 being the recommended 100k, we get the following:

    R1 / 100 = 3.3 & 0.6 - 1
    R1 / 100 = 5.5 - 1
          R1 = 450

Thus 450k and 100k resistors are used with 120pF feedforward capacitor for 100k resistors as suggested by the datasheet. The capacitors are used as per the datasheet, i.e. 4.7uF on input and 22uF on output with inductor of [470nH](https://jlcpcb.com/partdetail/Tdk-TFM201610ALCR47MTAA/C2045043), saturation current at 5.1 well above the switching frequency, from TDK, a respectable brand in hopes of avoiding coil whine).

The EN pin must be high for the converter to work, so for `3V3` rail we short it to input voltage. For `IOVDD` it is controlled by the AVR, which runs at 3.3V but this is ok as the datasheet specifies that voltages above 1V on the EN pin are logic high (page 5).

> TODO add an unpopulated resistor that can ensure we use at least 100uA on the 3V3 converter
> TODO there is R3, 100k resistor to ground on the converter input. Why? 

## 5V Charge Pump

We are using [HX4002](datasheets/hx4002.pdf) charge pump, same as in mk II. The pump is capable of delivering in excess of 240mA for 3.6V input voltage (it runs from VCC, which is stepped up above 3.3v). It only powers the RGB leds, which take at most 12mA per channel, which gives us 6.6 RGBs at full power.

WS2812 compatible, but smaller & less powerful LEDs are used. As they require 5V and the AVR that controls them runs on 3.3V we use a converter.

> TODO should the single LED in DPAD be replaced so that a proper DPAD with tilting can be used? If we use multiple LEDs we will be exceeding the power generator output, but it is likely fine, as currently the LEDs run at 12.5% of full power and seem to be fine.

## PMIC (BQ25895)

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

### Running on Batteries

The idea is that when running on batteries (3x AAA for 4.5V), the whole PMIC circuitry will be left unpopulated, while the bottom side of the board will be populated with a VUSB / VBATT switch as was in the mkII and a battery voltage voltage divider connected to the PWR_INT pin (which is available on ADC0). 

The voltage divider should use 300k and 570k resistors to transform 5V to 3.296V. 

> TODO those registers are really large and maybe the ADC on ATTiny won't be able to read the voltage - should check. 

Since we do not have the PMIC to ensure VSYS is never below 3.3V, the step down converters will stop operating when battery voltage drops below 3.3V (or they will output lower voltage, which might be fine for a while). Anyways, for NiMH this should not be much of a problem as the discharge curve is extremely flat. 

NOTE when running on batteries, the PWR_INT pull-up resistor must not be fitted!

> TODO For alkaline batteries, this could pose a problem as they discharge all the way down to 0V, but this can be solved by extra board with boost converter I presume. 

## AVR (ATTiny3217)

[ATTiny3217](datasheets/ATTiny3217.pdf) is used in QFN package.

## Audio Codec

[TLV320aic3204](datasheets/tlv320aic3204.pdf) is used.

## Speaker Amplifier

> TODO

## FM Radio

[Si4705](datasheets/si4705.pdf) is the chip used. It is FM only receiver with ability to use both embedded and headphone integrated antenna. 

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

> Pin 11 is not special, we might exchange it for say pin 46 and move the rest to provide 2 ADC pins on the cartridge, but it simplifies the wiring - but might be worth it.

Further details of how the HW requirements for the chip should look can be found in [HW Design Notes](datasheets/hardware-design-with-rp2350.pdf).

## Power Consumption

At worst case we expect relatively negligible mA on 3V3 and close to 1A on IOVDD, which for the sake of simplicity is 1A of 3V3 + 240mA on 5V for the neopixels. Assuming neopixel charge pump efficiency of 80% and DC-DC converter efficiency of 90%, we get:

    MAX = 1000 * 3.3 / 0.9 + 240 * 5.0 / 0.8 = 5166

However, in reality, we expect about 300mA for the system (IOVDD) and 50mA for the 5V charge pump, i.e:

    EXP = 300 * 3.3 / 0.9 + 50 * 5.0 / 0.8 = 1412

This leads to the following current draw for different voltages:

    Input Voltage | Imax   | Iexp     |Comments
    --------------|--------|---------
    3.0           | 1700mA | 470mA  | Battery depleted (worst case before shutdown)
    3.7           | 1400mA | 381mA  | Battery nominal
    4.2           | 1230mA | 336mA  | Battery fully charged
    5.0           | 1033mA | 282mA  |USB attached

> NOTE that while this is well withing the switching capabilities of the power components used, it does exceed the 1C discharge rate for the battery so the worst case might be a problem. If we assume 2C, then everything is allright. Also note that this does not take into account the boost converter employed by the PMIC chip which will also increase the battery drain at lowest settings. 


## Test Points

Top side:

- `TP2` - VUSB

Bottom side:

- `TP1` - QON pin of PMIC chip (connect to ground for battery reset & exit from the shipping mode)












## Pinouts

This is just a summary of devices connected to the 2 MCUs (AVR and RP) to ensure all fits and give a simple overview:

### RP2350

The RP2350 has 48 GPIO available in total. The mandatory (i.e. those required for even the basic functionality of the device) are the following (41):

- 16 for display data
- 4 for display control
- 4 for SD card in SPI interface
- 8 for HSTX pins on cartridge
- 2 for extra pins on cartridge (one of them ADC)
- 2 for I2C
- 5 for audio codec (reset, bclk, lrclk, di, do)

This leaves us with 1

- 2 pins for the SD card to allow SDIO interface
- 1 pin for AVR int
- 1 pin for reading the display memory as well
- 1 pin for audio codec int
- 2 pins for radio (reset & int)


### AVR

- accel int
- pmic int
- int to RP
- i2c switch -- this can actually be the same as 3v3 on it would seem, just the 3V3 on might need to be more controlled to ensure that we do not bleed voltage? 

## Power Path Stage



## Audio

TLV320AIC3204
- might need LDO for the codec's voltage (3V is ok, 150mA)

- seems I can get rid of MCLK

- verify AVR & Radio crystal & capacitors