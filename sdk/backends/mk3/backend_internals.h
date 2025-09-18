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


#define RCKID_SERIAL_SPEED 115200




/** RP2350 Pinout
 */
#define RP_PIN_I2C_SDA 0
#define RP_PIN_I2C_SCL 1
#define RP_PIN_RADIO_RESET 2
#define RP_PIN_AVR_INT 3
#define RP_PIN_RADIO_INT 4
#define RP_PIN_AUDIO_INT 5
//#define RP_PIN_I2S_DOUT 6
//#define RP_PIN_I2S_DIN 7
//#define RP_PIN_I2S_LRCK 8
//#define RP_PIN_I2S_BCLK 9
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
#define RP_PIN_SD_SDIO_2 40
#define RP_PIN_SD_SDIO_1 41
#define RP_PIN_SD_SCK 42
#define RP_PIN_SD_TX 43
#define RP_PIN_SD_RX 44
#define RP_PIN_SD_CSN 45
#define RP_PIN_GPIO_46 46
#define RP_PIN_GPIO_47 47

#define RP_SD_SPI spi1




// TODO remove those (for devboard only)
#define RP_PIN_I2S_MCLK 19
#define RP_PIN_I2S_BCLK 18
#define RP_PIN_I2S_LRCK 17
#define RP_PIN_I2S_DOUT 16
#define RP_PIN_I2S_DIN 15


