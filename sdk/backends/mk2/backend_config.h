#pragma once

/** Memory limits are created automatically by the linker script on the backend so we just have to extern the symbols here. 
 */
#define RCKID_MEMORY_INITIALIZATION \
    extern char __bss_end__; \
    extern char __StackLimit;






/** The baudrate for parallel data transfer to the display. 
 
    15MHz gives us 66.7ns per byte cycle, which is just within the display's limit. 
 */
#define RP_ST7789_BAUDRATE 15000000

/** SPI on which the SD card is connected. 
 */
#define RP_SD_SPI spi1


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

