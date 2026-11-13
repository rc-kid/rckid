# LEGO Remote Brick Hardware Specification

> This document describes the third iteration of the remote brick that uses Puya PY32F030 (the same MCU that powers the IO of kidDo itself) as the brick MCU. The goal of this revision is to cut costs, simplify manufacturing and to focus on what is essential for the remote brick.

## Power Domain

The hub is powered by 18650 li-ion batteries. One cell or two cells in parallel can be used. The cells are connected to [IP5306](), which is a very cheap chip intended for power banks that provides the following:

- fast charging of the battery (1S or 2P)
- step-up regulator from the battery to 5V
- LED indicators of the battery power (4 LEDs)
- button (long press on, long press off)

Drawbacks are no programmability, the charging is set to 2.1A (which is less than 1C even for a single battery) and the discharge limit is 2.1A as well. The device will also *not* power the MCU when it is charging, a compromise that I believe makes sense for the hub. Power circuit is taken straight from the datasheet.

Furthermore, the boost converter will turn itself off under light load, i.e. if the current draw is below 45mA for 32 seconds continuously. To combat this there is a dedicated low side mosfet connected to two 300Ohm 0805 resistors from 5V rail (33mA). This ensures that together with the neopixel (12x3mA on full power white), which is almost 70mA combined and should be enough for a quick pulse every 20 seconds to ensure the device will not be turned off. 

Internally the device power is distributed into the following networks:

- 3V3 via an LDO (XC6206P332MR-G) for the NRF24L01P
- 5V IO voltage that goes as power to the 4 IO connectors, which is connected via a load switch (TPS22919), which allows programmatic disconnects, slow start for capacitive loads and short circuit protection
- the left & right motors are powered through the main 5V line via H bridges (see below) so that these outputs can be selectively turned off as well. 

> TODO talk about the polyfuse

### Battery Sense

In order for the hub to be able to report the status of the battery to the kidDo, the raw battery line is connected to the ADC input of the MCU. However, to prevent the battery to backfeed the device when powered off, the battery sense line is guarded by a MOSFET that turns on only when the 5V rail is active.

This is done by a P high-side mosfet switching the BATT_SENSE with an N low side mosfet connecting the P side to a ground, controllable by the 5V rail.

### Main Current Sense

The charger 5V boost feeds directly to INA180 with 4mOhm sense resistor used to measure the total power consumption by the hub. 

> TODO INA sensitivity

### Power Envelope

> TODO calculations on how much what can take

## Motor Controllers

The hub supports left and right motor controls with identical circuits. Each motor is driver by DRV8837 H bridge and monitored by a dedicated INA180 with 4mOhm sense resistor so that the hub knows current consumption for selective control. Both motors support only 5V and are connected via a reversible 4 pin connector:

    -  +
    +  -

Note that this actually allows 90 degrees rotation, but this is safe and 90 degrees rotation only changes the moto direction, which is a feature so that kids do not have to change programs that often.

TODO INA sensitivity

TODO H bridge drive details

## IO Connections

The hub contains 4 independent (but powered by same rail) multi-purpose IO connectors. Each connector consists of GND, 5V and a IO line that is fed directly to the MCU. The connectors are reversible:

    5V   IO
    GND GND
    IO   5V

The IO connections can be configured to function as one of the following:

- digital input 
- digital output
- analog input (via built-in ADC)
- analog output (via built-in PWM)
- neopixel RGB (probably bit banging)

In the future, more configurations might be possible, including JACDAC, etc.

## Radio

The hub uses NRF24L01P module, then NF-03 module from AI Thinker, which in fact is the Si clone, which for the purposes of the hub is quite enough. The module is 3v3 powered with 5V tolerant pins and is powered by an LDO (XC6206P332MR-G) connected to the 5V rail. 

100uF buffer capacitor is placed next to the power pin for better power supply. 

## MCU

The MCU is PY32F030, ARM Cortex M0 with 63Kb FLASH and 4kB RAM, identical to the chip used as IO controller in the kidDo device itself. This keeps the ecosystem simple as all used MCUs are ARM and the same SWD programming interface can be used. The chip runs off the 5V rail coming directly from the boost powerbank chip.

- IO Connections go to `PA6`, `PA7`, `PB0` and `PB1`, which corresponds to ADC6-9 and channels 1-4 for TIM3
- Motor connectios go to `PA8`-`PA11` corresponding to TIM1 channels 1-4
- motor current sense goes to `PA0` and `PA3`, which is ADC0 and ADC3
- total current sense goes to `PA1` (ADC1)
- battery voltage sense goes to `PA2` (ADC2)
- NRF24 radio goes to `PB3`-`PB7` and `PA15`, corresponding to SPI1 SCK (B3), MISO (B4), MOSI (B5), CS, CE and IRQ pins

> TODO connect the "heater"





### RGB light

### Buzzer




