# Hardware Design

## Power Stage

We are using BQ25895(https://www.ti.com/product/BQ25895) for charger and power path manager as it supports I2C configuration, has integrated ADC which frees AVR pins and has integrated boost converter that ensures the VCC voltage will stay above 3.3V threshold which allows the 3V3 converters to be simple buck ones. 

We are not using the USB source detection because we are not interested in high speed charging, instead the D+ and D- pins are shorted together as per (https://www.ti.com/lit/an/sluaam6/sluaam6.pdf?ts=1736634296011&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FBQ25895) so that the adapter is always identified as USB_DCP (over 3A current capability, which we do *not* use). 

The PMID pin requires at least 8.5uF when not used and 40uF when used, so we are adding 40uF in case it will ever be used. 

> In theory this might allow the RP2350 to power external keyboard, or so, which *might* be useful. 

CE is pulled low to enable charging (inverted). 
