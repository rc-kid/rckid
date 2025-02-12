#pragma once

// ================================================================================================
/** Hardware thresholds and settings. 
 
    Those are hardware thresholds and settings used by the firmware. There should be no need to change those past the initial development and calibration, unless you are doing a heavily modified build. 
 */
// ================================================================================================

/** Memory limits are created automatically by the linker script on the backend so we just have to extern the symbols here. 
 */
#define RCKID_MEMORY_INITIALIZATION \
    extern char __bss_end__; \
    extern char __StackLimit;

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

/** PWM slice used for the audio out, which on pins 22 and 23 correspon to PWM slice 3. 
 */
#define RP_AUDIO_PWM_SLICE 3

/** Headphones detection threshold (when no headphones are inserted, the line is pulled up to 3V, i.e. some 930. 
 */
#define HEADPHONES_DETECTION_THRESHOLD 100

/** The baudrate for parallel data transfer to the display. 
 
    15MHz gives us 66.7ns per byte cycle, which is just within the display's limit. 
 */
#define RP_ST7789_BAUDRATE 15000000

/** SPI on which the SD card is connected. 
 */
#define RP_SD_SPI spi1

/** I2C communication speed. Set to the conservative 100kHz, but technically the 400kHz fast mode should work as well. 
 */
#define RP_I2C_BAUDRATE 100000

/** I2C address of the AVR. There should be no change to edit this.
 */
#define I2C_AVR_ADDRESS 0x43 



/** RP2040 Pinout 
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
