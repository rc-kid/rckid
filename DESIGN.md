# PCB Design Notes

## USB-C

Pull-down resistors are needed on the `CC1` and `CC2` lines in order for the device to be recognized and 5V above 500mA was supplied to it. 

Since we are using the same 1mm board as the reference HW design, we can keep their values, i.e. 22R termination resistors (RPi Pico datasheet says 27R but 22R is the closest value available in basic parts and internet seems to say its ok) close to the chip and 0.8mm wide tracks with 0.15mm space between should get us close enough to the 90R characteristic impedance required by the USB standard. 

## Charger

2k2 resistor gives us 450mA charging. 


## Crystal

We are using crystal with capacitance of 20pF and ESR of 80R max. From the RP2040 HW design, the capacitors connected to it should be roughly 20 * 2 - 5, which at 33pF gives us 21pF, which is close enough. The resistor is set to 1k as everywhere else and hopefully all will be ok. 

## USB


