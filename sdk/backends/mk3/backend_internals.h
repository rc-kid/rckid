#pragma once


/** Memory limits are created automatically by the linker script on the backend so we just have to extern the symbols here. 
 */
#define RCKID_MEMORY_INITIALIZATION \
    extern char __bss_end__; \
    extern char __StackLimit;




#define AVR_I2C_ADDRESS 0x43

/** AVR Pinout 
 
    - I2C is routed to B0 and B1, their default position. 
    - PWM pins (rumbler and backlight) are routed to TCB0 (alternate) and TCB1 respectively.
    - button matrix pins are default digital pins. We need iterrupt on the home button (BTN_1 of BTN_CTRL group) to wake up when powered on
    - AVR_TX is alternate position of serial TxD and can be used for debugging the firmware. Its is also the only free pin.
    - 

    
 
 */
#define AVR_PIN_AVR_TX          gpio::A1
#define AVR_PIN_PWR_INT         gpio::A2
#define AVR_PIN_PWM_RUMBLER     gpio::A3
#define AVR_PIN_VDD_EN          gpio::A4
#define AVR_PIN_BTN_4           gpio::A5
#define AVR_PIN_RGB             gpio::A6
#define AVR_PIN_BTN_ABXY        gpio::A7
#define AVR_PIN_I2C_SCL         gpio::B0
#define AVR_PIN_I2C_SDA         gpio::B1
#define AVR_PIN_BTN_1           gpio::B4
#define AVR_PIN_BTN_CTRL        gpio::B5
#define AVR_PIN_BTN_3           gpio::B6
#define AVR_PIN_BTN_2           gpio::B7
#define AVR_PIN_PWM_BACKLIGHT   gpio::C0
#define AVR_PIN_QSPI_SS         gpio::C1
#define AVR_PIN_BTN_DPAD        gpio::C2
#define AVR_PIN_AVR_INT         gpio::C3
#define AVR_PIN_5V_ON           gpio::C4
#define AVR_PIN_ACCEL_INT       gpio::C5
