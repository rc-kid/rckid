#define RCKID_AUDIO_DEBUG_


#define RCKID_DEBUG_FPS_

#define RCKID_SPLASHSCREEN_OFF_


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
// PDM mic (via pio) -- managed by SDK
#define RP_PIN_PDM_DATA 22
#define RP_PIN_PDM_CLK 23
// pwm4a, pwm4b audio output -- managed by SDK
#if (! defined RCKID_AUDIO_DEBUG)
    #define RP_PIN_PWM_RIGHT 24
    #define RP_PIN_PWM_LEFT 25
#else // RCKID_AUDIO_DEBUG
    #define RP_PIN_PWM_RIGHT 14
    #define RP_PIN_PWM_LEFT 15
#endif
// spi1 connected to the SD card -- managed by SDK
#define RP_PIN_SD_SCK 26
#define RP_PIN_SD_TX 27
#define RP_PIN_SD_RX 28
#define RP_PIN_SD_CSN 29

/* - pio for the display
   - for PDM microphone
   - ? timer ?  for sound out? 

   - then other pio is left for the user

*/

#define RP_SD_SPI spi1
// Actual frequency: 10416666. - ????????????
#define RP_SD_SPI_BAUDRATE 12000000

#define RP_I2C_BAUDRATE 100000

#define RP_ST7789_BAUDRATE 30000000

/** Goes down to 4.096MHz for the actual clock rate. 
 */
#define RP_MIC_BAUDRATE 8192000


#define RP_DEBUG_UART uart1
#define RP_DEBUG_UART_BAUDRATE 115200
#define RP_DEBUG_UART_TX_PIN 20
#define RP_DEBUG_UART_RX_PIN 21


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
#define AVR_PIN_CHARGE_EN A1
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
#define AVR_PIN_BTN_ABXY B5
#define AVR_PIN_BTN_3 B6
//#define AVR_PIN_RGB B7
#define AVR_PIN_RGB A7 

#define AVR_PIN_BTN_CTRL C0
#define AVR_PIN_HEADPHONES C1
#define AVR_PIN_DISP_RDX C2
#define AVR_PIN_QSPI_SS C3
#define AVR_PIN_BTN_DPAD C4
#define AVR_PIN_BTN_4 C5

/*

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
 */
/*
// PA4
#define AVR_PIN_5V_ON 0
// PA5
#define AVR_PIN_PWM_BACKLIGHT 1
// PA6
#define AVR_PIN_HEADPHONES 2
// PA7
#define AVR_PIN_RGB 3
#define AVR_PIN_BTN_3 4
#define AVR_PIN_BTN_2 5
#define AVR_PIN_BTN_ABXY 6
#define AVR_PIN_BTN_1 7
#define AVR_PIN_SDA 8
#define AVR_PIN_SCL 9
#define AVR_PIN_BTN_CTRL 10
#define AVR_PIN_CHARGING 11
#define AVR_PIN_QSPI_SS 12
#define AVR_PIN_3V3_ON 13
#define AVR_PIN_BTN_4 14
#define AVR_PIN_BTN_DPAD 15
#define AVR_PIN_PWM_RUMBLER 16
*/

/** I2C address of the AVR chip (always slave)
 */
#define AVR_I2C_ADDRESS 0x43

#define RCKID_INA219_I2C_ADDRESS 0x40

/** Determines home button's long press duration in 4 tick multiples ticks (~10ms). Defaults to 1 second */
#define BTN_HOME_LONG_PRESS_THRESHOLD 100

/* Duration of the home button press necessasy to power the device on. Measured in ADC cycles for the initial voltage detection, i.e. 887 cycles per second. 
 */
//#define BTN_HOME_POWER_ON_DURATION 900
//#define BTN_HOME_POWER_OFF_DURATION 1800

#define VCC_DC_POWER_THRESHOLD 430

#define VCC_CRITICAL_THRESHOLD 310

/** If defined, the RCKid is powered by a LiPo battery and when DC power is inserted, the battery will charge and must be monitored. 
 
    THIS MUST BE SET TO FALSE FOR THE AAA POWERED VERSION. 
  */
#define RCKID_HAS_LIPO_CHARGER

/** When the 3V3 rail is on, the onboard current sensor is being checked each frame right after AVR is done sending status to master. Should this fail, this number of ticks provides a failsafe for current measurements even with frameskips, or RP2040 hangup. 
*/
#define RCKID_CURRENT_SENSE_TIMEOUT_TICKS 10

#define RCKID_VBATT_CHARGE_CUTOFF_VOLTAGE 435

#define RCKID_VBATT_CUTOFF_TEMPERATURE 450

/** Headphones detection threshold (when no headphones are inserted, the line is pulled up to 3V, i.e. some 930. 
 */
#define HEADPHONES_DETECTION_THRESHOLD 100
