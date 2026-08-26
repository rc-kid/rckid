# LEGO Remote Brick Hardware Specification

> This document describes the second iteration of the remote brick that uses RP2040 as the brick MCU, which gives much nicer programming interface than the original ATTiny3217. 

## Power Domain

Battery goes via a polyfuse to the battery charger's battery port as well as to the global current sense resistor towards the motors 5V regulators. The VSYS regulated path of the battery charger goes to the 3v3 regulator used for system rail. This is to ensure that system is always on even when battery is charged, while the firmware must ensure that when battery is being charged, the 5V regulators as well as motors are powered off. 

### Battery Charger

We use Monolithic MP2672A which is an integrated 2S li-ion charger in standalone mode so that any RP2040 firmware glitches cannot interfere with the charging:
- `CV` pin pulls to GND with `33k` resistor to select 8.4V battery chemistry (the safest)
- `ILIM` pin pulls to GND with `12k` resistor for 1A charging
- `VLIM` is set to 4.5V minimal charging voltage by having RH 33k and RL 12k.

### Left and right 5V

Two buck converters to 5V, 2A each are provided for the left and right side connectors (2 general and 1 motor per side). 

### 3v3 System Power

### Current Sensing

INA219 is used three times, one global for total consumption of the entire brick, and 2 for left and right motors respectively. INA219 senses both voltage *and* current so we also know the battery level and the selected motor voltages. 

### Power Envelope

> TODO

## Radio

> TODO

## Motors

Left and right motor drivers are available both with voltage selection (directly from the battery, or 5V from DC-DC converter), INA219 for current sensing and a full H bridge for motor control. 6 row reversible hedaer connector allows the motor itself to determine the voltage by routing it from full/5V to VSEL which then goes to the current sense and H bridge motor power.

## General Pins

2 left and 2 right general pins are available. They provide 5V, GND and DATA in a reversible 3 row connector. The DATA pin goes directlyto the RP2040 and as such is expected to be between 0..3.3V. The use of general pins is completely up to the firmware, but we ensure the connections go to the ADC pins so that analog input is possible.
