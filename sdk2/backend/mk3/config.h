#pragma once

/** Firmware version of the AVR chip. 
 
    NOTE this number must be bumped for every feature update to the AVR firmware that changes the communication protocol between the RP2350 and the AVR. This is used by the RP2350 firmware to determine if the AVR firmware is compatible with it or not.
 */
#define RCKID_AVR_FIRMWARE_VERSION 1

/** Default brightess. This is also the brightness that will be set by debug mode.
 */
#define RCKID_DEFAULT_BRIGHTNESS 128

/** Frames per long press of the home button (at 60 fps). The home button long press is automatically detected by the AVR and will either turn the device on when powered off, or will force it to shutdown mode if powered on. 
 */
#define RCKID_HOME_BUTTON_LONG_PRESS_FPS 120

/** Timeout for the RP2350 to acknowledge the power interrupt request.  
 */
#define RCKID_POWEROFF_ACK_TIMEOUT_FPS 10

#define RCKID_POWEROFF_TIMEOUT_FPS (10 * 60)

/** Number of ticks (roughly 1/60th of second) for button debounce (all buttons), i.e. changes in button state are ignored (delayed if permanent) for this many ticks after accepted change.
 */
#define RCKID_BUTTON_DEBOUNCE_TICKS 4

#define RCKID_RGB_TICKS_PER_SECOND 66
#define RCKID_NUM_RGB_LEDS 8
#define RCKID_RGB_BRIGHTNESS 32
#define RCKID_RGB_NOTIFICATION_SPEED 100

#define RCKID_RUMBLER_EFFECT_POWER_ON RumblerEffect{192, 5, 1, 1}

/** When set to 1, the AVR_INT pin is used for serial debugging. 
 */
#define RCKID_AVR_IS_SERIAL_TX 0

/** Serial speed for AVR debugging (when RCKID_AVR_IS_SERIAL_TX is 1). 
 */
#define RCKID_AVR_SERIAL_SPEED 115200

/** Voltage above which USB power is detected. While reading the VUSB presence directly would be more precise, using this threshold is valid too and saves us one pin. This is based on the typical Li-Ion battery voltage cutoff at 4.2 volts, so anything above that must be the 5V from USB.
 */
#define RCKID_VUSB_THRESHOLD 460 

/** Voltage threshold under which the device will power off.
 */
#define RCKID_POWER_ON_THRESHOLD 310

/** Speed of the communication with ST7789 display when in PIO mode (for fast display data transfers). Defaults to 15MHz which is just within the display datasheet's capabilities.
 */
#define RCKID_ST7789_SPEED 15000000
#define RCKID_ST7789_PIO pio0

#define RCKID_SPLASHSCREEN_OFF 1

/** I2C speed for the RP2350 master driver. 
 */
#define RCKID_I2C_SPEED 400000
/** Maximum size of an asynchronous I2C message. 
 */
#define RCKID_I2C_MAX_ASYNC_MSG_SIZE 16

#define RCKID_AVR_I2C_ADDRESS 0x43

/** SD Card Confguration
 */
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



/** AVR pinout
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

#define RGB_LED_DPAD_TOP_LEFT 1
#define RGB_LED_DPAD_TOP_RIGHT 0
#define RGB_LED_DPAD_BOTTOM_LEFT 2
#define RGB_LED_DPAD_BOTTOM_RIGHT 3
#define RGB_LED_BTN_A 7
#define RGB_LED_BTN_B 6
#define RGB_LED_BTN_SELECT 4
#define RGB_LED_BTN_START 5
