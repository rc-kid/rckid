#pragma once

#define RCKID_ENABLE_STACK_PROTECTION

// ================================================================================================

/** Backend configuration - hardware settings. 
 
    The following macros determine the hardware characteristics of the RCKid the firmware is intended for. 

    BE EXTREMELY CAREFUL WHEN CHANGING THOSE VALUES AS INVALID SETTINGS MAY DESTROY THE DEVICE. 
 */

/** If defined, the RCKid is powered by a LiPo battery and when DC power is inserted, the battery will charge and must be monitored. 
 
    THIS MUST BE SET TO FALSE FOR THE AAA POWERED VERSION. 
  */
#define RCKID_HAS_LIPO_CHARGER




#define RCKID_SPLASHSCREEN_OFF false


/** Select this for PCB version 2.2 where charging was not connected directly to an AVR pin, but went to ground with a swicth (i.e. to enable charging the pin was to be set to float and not disturb the mosfet and to disable, the pin was drivern ) 
*/

#define RCKID_VERSION_2_2___

// ================================================================================================

/** User Interface Specifics
 
    Those values determine the user experience, and can be set freely. Especially if your build does not follow the exact materials used by RCKid proper, you way want to tweak those. 
 */


/** Idle timer in seconds, after which the device powers off. 
 */
#define IDLE_TIMEOUT 300
//#define IDLE_TIMEOUT 3000000

/** Determines home button's long press duration in system ticks (1ms)
 */
#define BTN_HOME_LONG_PRESS_THRESHOLD 1000

/** Default brigthness of the RGB LEDs for all effects. Can be anything between 1 and 255 - the value depends on the opacity of the keys. 
 */
#define RGB_LED_DEFAULT_BRIGHTNESS 32


#define RUMBLER_DEFAULT_STRENGTH 128
#define RUMBLER_OK_DURATION 5
#define RUMBLER_FAIL_DURATION 3
#define RUMBLER_ATTENTION_DURATION 10
#define RUMBLER_NUDGE_DURATION 2

// ================================================================================================

/** Hardware thresholds. 
 
    Those are hardware thresholds used by the firmware. There should be no need to change those past the initial development and calibration, unless you are doing a heavily modified build. 
 */

/** Critical voltage at which the device will not allow itself to be started (3v3 power rail). To prevent spurious measurements when the device is powering on or off, a configurable amount of consecutive vcc measurements must be below the threshold top trigger the reaction. 
 
    TODO The limit here is the fact that AVR runs off the VCC rail while communicating with RP over I2C (i.e. SDA and SCL will be 3v3. If we went lower, we could get past AVR's tolerance). In version 3 this (and the warning numbers can be lowered to extend battery life a bit). 
 */
#define VOLTAGE_CRITICAL_THRESHOLD 330

/** When the VCC is below this threshold, the notification LED will flash red. 
 */
#define VOLTAGE_WARNING_THRESHOLD 340

/** Nominal voltage of fully charged battery.
 */
#define VOLTAGE_BATTERY_FULL_THRESHOLD 420

/** If the VCC measured by the chip is over this value (max of battery voltage + margin), we expect we are running from DC power. 
 */
#define VOLTAGE_DC_POWER_THRESHOLD 430

/** If the VBatt will be reported at over 4.35V during charging, we'll detect overcharge failure and cut the charger off. 
 */
#define VOLTAGE_BATTERY_OVERCHARGE_THRESHOLD 435

/** If the device's temperature measured by teh AVR will be over 40 degrees, we'll detect battery overheat and turn charging off.
 */
#define TEMPERATURE_BATTERY_OVERHEAT_THRESHOLD 400

/** Headphones detection threshold (when no headphones are inserted, the line is pulled up to 3V, i.e. some 930. 
 */
#define HEADPHONES_DETECTION_THRESHOLD 100




// ================================================================================================
/** AVR Pinout
 */
#define AVR_PIN_HEADPHONES A1
#define AVR_PIN_CHARGING A2
#define AVR_PIN_PWM_RUMBLER A3
#define AVR_PIN_VBATT A4
#define AVR_PIN_PWM_BACKLIGHT A5
#define AVR_PIN_3V3_ON A6
#define AVR_PIN_5V_ON A7

#define AVR_PIN_SCL B0
#define AVR_PIN_SDA B1
#define AVR_PIN_BTN_HOME B2
#define AVR_PIN_BTN_1 B3
#define AVR_PIN_BTN_2 B4
#define AVR_PIN_BTN_ABSELSTART B5
#define AVR_PIN_BTN_3 B6
#define AVR_PIN_RGB B7

#define AVR_PIN_BTN_CTRL C0
#define AVR_PIN_CHARGE_EN C1
#define AVR_PIN_DISP_RDX C2
#define AVR_PIN_QSPI_SS C3
#define AVR_PIN_BTN_DPAD C4
#define AVR_PIN_BTN_4 C5

/** In version 2.2 the headphones and charge enable pins were swapped.
 */
#ifdef RCKID_VERSION_2_2
#undef AVR_PIN_HEADPHONES
#undef AVR_PIN_CHARGE_EN
#define AVR_PIN_HEADPHONES C1 
#define AVR_PIN_CHARGE_EN A1
#endif


/** Index of the notification LED in the RGB strip. 
 */
#define RGB_LED_INDEX_NOTIFICATION 2

/** \section RP2040Pinout RP2040 Pinout
 

 */

// i2c0 -- managed by the SDK, accessible by the user
#define RP_PIN_SDA 0
#define RP_PIN_SCL 1
// display -- managed by SDK
#define RP_PIN_DISP_CSX 2
#define RP_PIN_DISP_DCX 3
#define RP_PIN_DISP_WRX 4
#define RP_PIN_DISP_TE 5
#define RP_PIN_DISP_DB8 6
#define RP_PIN_DISP_DB9 7
#define RP_PIN_DISP_DB10 8
#define RP_PIN_DISP_DB11 9
#define RP_PIN_DISP_DB12 10
#define RP_PIN_DISP_DB13 11
#define RP_PIN_DISP_DB14 12
#define RP_PIN_DISP_DB15 13
// user available gpio on the cartridge -- full user control
#define RP_PIN_GPIO_14 14
#define RP_PIN_GPIO_15 15
#define RP_PIN_GPIO_16 16
#define RP_PIN_GPIO_17 17
// user available gpio on the cartridge / spi0 -- full user control
#define RP_PIN_SPI0_SCK_GPIO_18 18
#define RP_PIN_SPI0_TX_GPIO_19 19
#define RP_PIN_SPI0_RX_GPIO_20 20
#define RP_PIN_SPI0_CSN_GPIO_21 21
// pwm3a, pwm3b audio output -- managed by SDK
#define RP_PIN_PWM_LEFT 22
#define RP_PIN_PWM_RIGHT 23
// PDM mic (via pio) -- managed by SDK
#define RP_PIN_PDM_CLK 24
#define RP_PIN_PDM_DATA 25
// spi1 connected to the SD card -- managed by SDK
#define RP_PIN_SD_SCK 26
#define RP_PIN_SD_TX 27
#define RP_PIN_SD_RX 28
#define RP_PIN_SD_CSN 29


/** PWM slice used for the audio out, which on pins 22 and 23 correspon to PWM slice 3. 
 */
#define RP_AUDIO_PWM_SLICE 3


#define RP_I2C_BAUDRATE 100000
//#define RP_I2C_BAUDRATE 50000

#define I2C_AVR_ADDRESS 0x43 


/** When enabled, instead of logging to the USB serial port, logs all data directly to the hardware serial port at pins 16 & 17 (uart0) at 74880 baudrate.
 */
//#define RP_LOG_TO_SERIAL





/** The baudrate for parallel data transfer to the display. 
 
    15MHz gives us 66.7ns per byte cycle, which is just within the display's limit. 
 */
#define RP_ST7789_BAUDRATE 15000000




#define COMMS_UART_TX_TIMEOUT_US 1000








// OLD and unchecked yet ==================================================================================================

#define RCKID_AUDIO_DEBUG_


#define RCKID_DEBUG_FPS_

#define RCKID_SPLASHSCREEN_OFF_

#define RCKID_RUMBLER_DEFAULT_STRENGTH 128
#define RCKID_RUMBLER_OK_TIME_ON 100
#define RCKID_RUMBLER_OK_TIME_OFF 100
#define RCKID_RUMBLER_OK_CYCLES 1

#define RCKID_RUMBLER_FAIL_TIME_ON 20
#define RCKID_RUMBLER_FAIL_TIME_OFF 30
#define RCKID_RUMBLER_FAIL_CYCLES 3

#define RCKID_RUMBLER_NUDGE_STRENGTH 128
#define RCKID_RUMBLER_NUDGE_TIME_ON 40


#define RADIO_MAX_CONNECTIONS 10

/** Pins assignment for the cartridge with NRF radio chip. 
 */
#define RADIO_NRF_PIN_CS 21
#define RADIO_NRF_PIN_RXTX 20
#define RADIO_NRF_PIN_IRQ 17
#define RADIO_NRF_PIN_MISO 16
#define RADIO_NRF_PIN_MOSI 19
#define RADIO_NRF_PIN_SCK 18

/** Pin assignment for the WiFi cartridge with ESP8266 and other cartridge settings
 */
#define RADIO_WIFI_PIN_TX 16
#define RADIO_WIFI_PIN_RX 17
#define RADIO_WIFI_PIN_RESET 14
#define RADIO_WIFI_PIN_FLASH 15
#define RADIO_WIFI_UART uart0
#define RADIO_WIFI_BAUDRATE 115200

/* - pio for the display
   - for PDM microphone
   - ? timer ?  for sound out? 

   - then other pio is left for the user

*/

#define RP_SD_SPI spi1
// Actual frequency: 10416666. - ????????????
#define RP_SD_SPI_BAUDRATE 12000000


#define RP_DEBUG_UART uart1
#define RP_DEBUG_UART_BAUDRATE 115200
#define RP_DEBUG_UART_TX_PIN 20
#define RP_DEBUG_UART_RX_PIN 21


/** I2C address of the RP2040 chip (to wake up from sleep)
 */
#define RP_I2C_ADDRESS 0x55

/** PWM slice for the microphone in, which on pins 24 and 25 corresponds to PWM slice 4 */
#define RP_MIC_SLICE 4

/** AVR Pinout
 
    PA1 -- CHARGE_EN (floating, pull low to disable charging)
    PA2 -- CHARGING (ADC0 channel 2, can also be just digital ?)
    PA3 -- PWM_RUMBLER (TCB1 WO)
    PA4 -- VBATT (ADC0 channel 4)
    PA5 -- PWM_BACKLIGHT (TCB0 WO)
    PA6 -- 3V3_ON
    PA7 -- 5V_ON
    PB7 -- RGB
    PB6 -- BTN_3
    PB5 -- BTN_ABXY
    PB4 -- BTN_2
    PB3 -- BTN_1
    PB2 -- BTN_HOME (-> GND, internal pullup)
    PB1 -- SDA
    PB0 -- SCL
    PC0 -- BTN_CTRL
    PC1 -- HEADPHONES (ADC 1 channel 7)
    PC2 -- DISP_RDX
    PC3 -- QSPI_SS
    PC4 -- BTN_DPAD
    PC5 -- BTN_4
*/






