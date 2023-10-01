
#include "platform/platform.h"
#include "platform/peripherals/neopixel.h"

using namespace platform;

/** RCKid mk II

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
class RCKid {
public:
    static constexpr gpio::Pin PIN_5V_ON = 0;
    static constexpr gpio::Pin PIN_PWM_BACKLIGHT = 1;
    static constexpr gpio::Pin PIN_HEADPHONES = 2;
    static constexpr gpio::Pin PIN_RGB = 3;
    static constexpr gpio::Pin PIN_BTN_3 = 4;
    static constexpr gpio::Pin PIN_BTN_2 = 5;
    static constexpr gpio::Pin PIN_BTN_ABXY = 6;
    static constexpr gpio::Pin PIN_BTN_1 = 7;
    static constexpr gpio::Pin PIN_SDA = 8;
    static constexpr gpio::Pin PIN_SCL = 9;
    static constexpr gpio::Pin PIN_BTN_CTRL = 10;
    static constexpr gpio::Pin PIN_CHARGING = 11;
    static constexpr gpio::Pin PIN_QSPI_SS = 12;
    static constexpr gpio::Pin PIN_3V3_ON = 13;
    static constexpr gpio::Pin PIN_BTN_4 = 14;
    static constexpr gpio::Pin PIN_BTN_DPAD = 15;
    static constexpr gpio::Pin PIN_PWM_RUMBLER =16;

    static void initialize() __attribute__((always_inline)) {
        // enable 2 second watchdog so that the second tick resets it always with enough time to spare
        while (WDT.STATUS & WDT_SYNCBUSY_bm); // required busy wait
            _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_2KCLK_gc);                
        // set CLK_PER prescaler to 2, i.e. 10Mhz, which is the maximum the chip supports at voltages as low as 3.3V
        CCP = CCP_IOREG_gc;
        CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm; 
        gpio::initialize();

        initializeButtons();

    }

    static void loop() __attribute__((always_inline)) {

    }

private:

    /** \name Buttons
        
        The buttons are connected in a matrix of 3 button groups and 4 buttons per group. 
     */
    //@{
    static void initializeButtons() {
        // initialize the button pins to inputs with pullup
        gpio::inputPullup(PIN_BTN_1);
        gpio::inputPullup(PIN_BTN_2);
        gpio::inputPullup(PIN_BTN_3);
        gpio::inputPullup(PIN_BTN_4);
        // initialize all button groups to ouput & set them high
        gpio::output(PIN_BTN_DPAD);
        gpio::output(PIN_BTN_ABXY);
        gpio::output(PIN_BTN_CTRL);
    }

     //@}
 }; // RCKid

void setup() { RCKid::initialize(); }
void loop() { RCKid::loop(); }
