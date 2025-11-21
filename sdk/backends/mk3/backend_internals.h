#pragma once

#define RCKID_I2C_SPEED 400000

#define RCKID_AVR_I2C_ADDRESS 0x43
#define RCKID_PMIC_I2C_ADDRESS 0x6b
#define RCKID_ACCEL_I2C_ADDRESS 0x19
#define RCKID_ALS_I2C_ADDRESS 0x29
#define RCKID_AUDIO_CODEC_I2C_ADDRESS 0x1a
#define RCKID_FM_RADIO_I2C_ADDRESS 0x11

#define RCKID_VUSB_THRESHOLD 460 
#define RCKID_LOW_BATTERY_THRESHOLD 330 
#define RCKID_POWER_ON_THRESHOLD 300

/** Speed of the communication with ST7789 display when in PIO mode (for fast display data transfers). Defaults to 15MHz which is just within the display datasheet's capabilities.
 */
#define RCKID_ST7789_SPEED 15000000
#define RCKID_ST7789_PIO pio0


#define RCKID_SERIAL_SPEED 115200

#define RCKID_SD_SPI_INIT_SPEED 200000
//#define RCKID_SD_SPI_SPEED 12000000
#define RCKID_SD_SPI_SPEED 20000000
#define RCKID_SD_PIO pio0




/** RP2350 Pinout
 */
#define RP_PIN_I2C_SDA 0
#define RP_PIN_I2C_SCL 1
#define RP_PIN_RADIO_INT 2
#define RP_PIN_AVR_INT 3
#define RP_PIN_I2S_MCLK 4
#define RP_PIN_I2S_DAC 5
#define RP_PIN_I2S_ADC 6
#define RP_PIN_I2S_BCLK 7
#define RP_PIN_I2S_LRCK 8
#define RP_PIN_HEADSET_DETECT 9
#define RP_PIN_SD_CD 10
#define RP_PIN_DISP_RDX 11
#define RP_PIN_GPIO_12 12
#define RP_PIN_GPIO_13 13
#define RP_PIN_GPIO_14 14
#define RP_PIN_GPIO_15 15
#define RP_PIN_GPIO_16 16
#define RP_PIN_GPIO_17 17
#define RP_PIN_GPIO_18 18
#define RP_PIN_GPIO_19 19
#define RP_PIN_DISP_DB15 20
#define RP_PIN_DISP_DB14 21
#define RP_PIN_DISP_DB13 22
#define RP_PIN_DISP_DB12 23
#define RP_PIN_DISP_DB11 24
#define RP_PIN_DISP_DB10 25
#define RP_PIN_DISP_DB9 26
#define RP_PIN_DISP_DB8 27
#define RP_PIN_DISP_DB7 28
#define RP_PIN_DISP_DB6 29
#define RP_PIN_DISP_DB5 30
#define RP_PIN_DISP_DB4 31
#define RP_PIN_DISP_DB3 32
#define RP_PIN_DISP_DB2 33
#define RP_PIN_DISP_DB1 34
#define RP_PIN_DISP_DB0 35
#define RP_PIN_DISP_WRX 36
#define RP_PIN_DISP_DCX 37
#define RP_PIN_DISP_CSX 38
#define RP_PIN_DISP_TE 39
#define RP_PIN_SD_SCK 40
#define RP_PIN_SD_TX 41
#define RP_PIN_SD_RX 42
#define RP_PIN_SD_SDIO_1 43
#define RP_PIN_SD_SDIO_2 44
#define RP_PIN_SD_CSN 45
#define RP_PIN_GPIO_46 46
#define RP_PIN_GPIO_47 47

/** AVR Pinout 
 
    - I2C is routed to B0 and B1, their default position. 
    - PWM pins (rumbler and backlight) are routed to TCB0 (alternate) and TCB1 respectively.
    - button matrix pins are default digital pins. We need iterrupt on the home button (BTN_1 of BTN_CTRL group) to wake up when powered on
    - AVR_TX is alternate position of serial TxD and can be used for debugging the firmware. Its is also the only free pin.
    - 

    Powered On:
    
    - monitor buttons
    - update RGB LEDs
    - update rumbler
    - respond to I2C commands
    - control RP reboot & bootloading

    - everything is digital, so we only need the ADC for temperature...
    - use system tick of 5ms, this gives us ability to read all buttons every frame 

    NOTE: There is an errata for attiny1616 and smaller chips that states HW bug where turning off RTC turns off PIT as well, which means those chips will *not* work with RCKid as we use RTC for the system tick and PIT for the timekeeping.
 */

#define AVR_PIN_AVR_INT         gpio::A1
#define AVR_PIN_VCC_SENSE       gpio::A2
#define AVR_PIN_PWM_RUMBLER     gpio::A3
#define AVR_PIN_5V_EN           gpio::A4
#define AVR_PIN_CHARGING        gpio::A5
#define AVR_PIN_BTN_1           gpio::A6
#define AVR_PIN_BTN_3           gpio::A7
#define AVR_PIN_I2C_SCL         gpio::B0
#define AVR_PIN_I2C_SDA         gpio::B1
#define AVR_PIN_ACCEL_INT       gpio::B4
#define AVR_PIN_BTN_CTRL        gpio::B5
#define AVR_PIN_BTN_ABXY        gpio::B6
#define AVR_PIN_BTN_2           gpio::B7
#define AVR_PIN_PWM_BACKLIGHT   gpio::C0
#define AVR_PIN_IOVDD_EN        gpio::C1
#define AVR_PIN_BTN_4           gpio::C2
#define AVR_PIN_RGB             gpio::C3
#define AVR_PIN_BTN_DPAD        gpio::C4
#define AVR_PIN_QSPI_SS         gpio::C5

#define NUM_RGB_LEDS 8
