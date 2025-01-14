# Hardware Design

## Power Path Stage

We are using BQ25895(https://www.ti.com/product/BQ25895) for charger and power path manager as it supports I2C configuration, has integrated ADC which frees AVR pins and has integrated boost converter that ensures the VCC voltage will stay above 3.3V threshold which allows the 3V3 converters to be simple buck ones. 

We are not using the USB source detection because we are not interested in high speed charging, instead the D+ and D- pins are shorted together as per (https://www.ti.com/lit/an/sluaam6/sluaam6.pdf?ts=1736634296011&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FBQ25895) so that the adapter is always identified as USB_DCP (over 3A current capability, which we do *not* use). 

The PMID pin requires at least 8.5uF when not used and 40uF when used, so we are adding 40uF in case it will ever be used. OTG is pulled low to disable the 5V output. 

> TODO reserve pin forthe OTG and figure out how to enable the OTG so that in theory RCkid can for instance use mouse or keyboard?  

CE is pulled low to enable charging (inverted). 

Inductor is 2.2uH as per the datasheet, going for coilcraft with hopes that coil whine will be elliminated when using respected brand. 

ILIM resistor is set to 270Ohm for (KLIM of 390/270, see datasheet page 26) 1.44A max input current, which should be enough for 500mA cartridge drain, system and 500mA battery charging current. 

The only communication between the PMIC chip and AVR is the two I2C lines and the interrupt. 

## 3V3 Voltage Converters

The design emplys two identical 3V3 converters, one is always on and powers the AVR, rumbler and sensors, while the other can be enabled/disabled and powers the device proper (i.e. RP2350B, cartridge and audio). Using TLV62569 has the additional benefit of being able to use the same inductor as the power path stage. 

The feedback resistors are set



## 5V voltage converter

To simplify the design, we use a charge pump 

> Can we use VBUS and USB OTG for this with the PMIC? Since we do not negotiate higher USB voltage, this should be possible? 

## Audio

TLV320AIC3204