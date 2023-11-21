# ATTiny1616 Code

The ATTiny is always-on, responsible for power, user input and data retention. Communication is done via the I2C bus and the AVR is in strictly slave mode, polled by the RP2040 in regular intervals. 

> See the `src/avr.cpp` file for more details. 

## Pinout

                       -- VDD             GND --
                 5V_ON -- (00) PA4   PA3 (16) -- PWM_RUMBLER
         PWM_BACKLIGHT -- (01) PA5   PA2 (15) -- BTN_DPAD
            HEADPHONES -- (02) PA6   PA1 (14) -- BTN_4
                   RGB -- (03) PA7   PA0 (17) -- UPDI
                 BTN_3 -- (04) PB5   PC3 (13) -- 3V3_ON
                 BTN_2 -- (05) PB4   PC2 (12) -- QSPI_SS
              BTN_ABXY -- (06) PB3   PC1 (11) -- CHARGING
                 BTN_1 -- (07) PB2   PC0 (10) -- BTN_CTRL
             SDA (I2C) -- (08) PB1   PB0 (09) -- SCL (I2C)

## User Input

When powered down, the BTN_CTRL pin is held high and 

## Bootloader






