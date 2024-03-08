# PCB Design Notes

## USB-C

Pull-down resistors are needed on the `CC1` and `CC2` lines in order for the device to be recognized and 5V above 500mA was supplied to it. 

Since we are using the same 1mm board as the reference HW design, we can keep their values, i.e. 22R termination resistors (RPi Pico datasheet says 27R but 22R is the closest value available in basic parts and internet seems to say its ok) close to the chip and 0.8mm wide tracks with 0.15mm space between should get us close enough to the 90R characteristic impedance required by the USB standard. 

## 3v3 Switching Regulator

Uses `TPS63001` fixed at 3v3, therefore equations (1) and (2) for the divider and feed forward capacitor are moot. Boost duty cycle at 3V, which is the lowest operating voltage (under 3V the AVR's voltages will be low enough for the 3v3 I2C rail to cause problems) is thus calculated at as (3):

    D = (Vout - Vin)/Vout 
    D = (3.3 - 3.0)/3.3
    D = 0.09

Inductor peak current (4) is:

    Ipeak = Iout / (efficiency * (1 -D)) + Vin * D / (2 * f * L)

where efficiency is `0.8`, switching frequency is `2.5MHz` and inductor value is `0.7uH` (using 1uH inductor with 30% tolerance, this is the worst case), for max `Iout` of `1A` (this gives 20% margin over the maximum rated current of 800mA):

    Ipeak = 1 / (0.8 * 0.91) + 3 * 0.09 / (2 * 2.5MHz * 0.7uH)
    Ipeak = 1.37 + 0.077
    IPeak = 1.45 A

> OLD:

Uses `TPS63060`, minimum input voltage is 3V, output voltage is 3.3V. From the application design procedure in the datasheet, duty cycle (1) is:

    D = (Vout - Vin)/Vout 
    D = (3.3 - 3.0)/3.3
    D = 0.09

Maximum Output current boost (2) is:

    Iout = 0.8 * Isw * (1 - D)
    Iout = 0.8 * Isw * 0.91 = Isw * 0.728
    Iout = 2 * 0.728 = 1.45A

Using these values, the `Ipeak` for the inductor from (5) is:

    Ipeak = Iout/(0.728) + (Vin * 0.09)/(2 * 2.4Mhz * 1uH)
    Ipeak = 2 + 0.3 / (2 * 2.4 * 1000000 * 1 / 1000000)
    Ipeak = 2 + 0.06

Setting the voltage is done by (6):

    R1 = R2 * (Vout / Vfb - 1)
    R1 = R2 * 5.6

R2 should be lower than 500kOhm, so the values of `R1 = 390kOhm` and `R2 = 68kOhm` gives output value of:

    Vout = (R1 + R2) / 2R2
    Vout = 3.367V

## Charger

2k2 resistor gives us 450mA charging. 

## Current Consumption

INA219 is used to measure the current. Based on the datasheet, Current LSB (2)

    CurrentLSB = 2 / 2^15


## Crystal

We are using crystal with capacitance of 20pF and ESR of 80R max. From the RP2040 HW design, the capacitors connected to it should be roughly 20 * 2 - 5, which at 33pF gives us 21pF, which is close enough. The resistor is set to 1k as everywhere else and hopefully all will be ok. 

## USB




## Sensors

An RC filter is added to provide a bit cleaner voltage to the sensors in the top bar. The current consumption is as follows:

- 1mA for BMI160
- 20mA for BMM150
- 1mA for LTR390UV
- 10mA for HDC1080

In total this gives us 32mA, say 40mA, which gives us 40mV voltage drop for 1Ohm resistor, that combined with 2x47uF capacitors makes a RC filter with cutoff frequency around 1.6kHz

> If this proves not that great, the 1Ohm resistor is hand-solderable 0805 and can be replaced with 0Ohm.