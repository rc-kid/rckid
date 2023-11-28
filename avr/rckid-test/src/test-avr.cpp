
#include "platform/platform.h"
#include "platform/peripherals/neopixel.h"

#include "rckid/config.h"

using namespace platform;

/** AVR Test App

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

    /** \name Common routines
     */
    //@{
    static void initialize() __attribute__((always_inline)) {
        // immediately after startup, kill the power to 3V3 and 5V rails by ensuring they are configured as input pins w/o pull-up. (they should start as input pins anyways, but to be sure)
        gpio::initialize();
        gpio::input(PIN_3V3_ON);
        gpio::input(PIN_5V_ON);
        // enable 2 second watchdog so that the second tick resets it always with enough time to spare
        //while (WDT.STATUS & WDT_SYNCBUSY_bm); // required busy wait
        //    _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_2KCLK_gc);                
        // set CLK_PER prescaler to 2, i.e. 10Mhz, which is the maximum the chip supports at voltages as low as 3.0V
        CCP = CCP_IOREG_gc;
        CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm;
        // initialize the RTC that fires every second for a semi-accurate real time clock, also start the timer
        RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;
        RTC.PITINTCTRL |= RTC_PI_bm; // enable the interrupt
        while (RTC.PITSTATUS & RTC_CTRLBUSY_bm);
        RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc | RTC_PITEN_bm;
    }

    static inline volatile bool tick = false;
    
    static inline NeopixelStrip<4> rgb{PIN_RGB};

    //@}

    /** \name LevelShifter Test 
     
     */
    //@{
    static void initializeTestLevelShifter() __attribute__((always_inline)) {
        initialize();
        // turn on the 5V rail 
        gpio::output(PIN_5V_ON);
        gpio::high(PIN_5V_ON);
        // mark the RGB pin as output and set low
        gpio::output(PIN_RGB);
        gpio::low(PIN_RGB);
    }

    static void loopTestLevelShifter() __attribute__((always_inline)) {
        cpu::delayUs(10);
        gpio::high(PIN_RGB);
        cpu::delayUs(10);
        gpio::low(PIN_RGB);
    }

    //@}    

    /** \name Neopixel Test 
     
        The simplest test, turns on the 5V rail and cycles the 4 neopixels and colors in one second intervals. 
     */

    //@{

    static inline uint8_t ledTest = 0;

    static void initializeTestNeopixel() __attribute__((always_inline)) {
        initialize();
        // turn on the 5V rail 
        gpio::output(PIN_5V_ON);
        gpio::high(PIN_5V_ON);
        // mark the RGB pin as output and set low
        gpio::output(PIN_RGB);
        gpio::low(PIN_RGB);
        cpu::delayMs(100);
        rgb.clear();
        rgb[0].r = 10;
        rgb[1].g = 10;
        rgb[3].b = 10;
        rgb.update();
    }

    static void loopTestNeopixel() __attribute__((always_inline)) {
        if (tick) {
            tick = false;
            uint8_t ledIndex = ledTest & 0x3;
            uint8_t ledColor = ledTest >> 2;
            rgb.clear();
            switch (ledColor) {
                case 0:
                    rgb[ledIndex] = Color::RGB(10, 0, 0);
                    break;
                case 1: 
                    rgb[ledIndex] = Color::RGB(0, 10, 0);
                    break;
                case 2:
                    rgb[ledIndex] = Color::RGB(0, 0, 10);
                    break;
                case 3:
                    rgb[ledIndex] = Color::RGB(10, 10, 10);
                    break;
                // violet, so that we always see something
                default:
                    rgb[ledIndex] = Color::RGB(10, 0, 10);
                    break;
            }
            rgb.update(true);
            ledTest = (++ledTest) % 16;
        }
    }
    //@}

    /** \name Buttons test
     
        Scans for the button presses and displays LED colors accordingly. 
     */
    //@{

    static inline uint8_t buttonGroup = 0;

    static void initializeTestButtons() __attribute__((always_inline)) {
        initialize();
        // change RTC frequency to 128Hz
        while (RTC.PITSTATUS & RTC_CTRLBUSY_bm);
        RTC.PITCTRLA = RTC_PERIOD_CYC256_gc | RTC_PITEN_bm;
        // turn on the 5V rail 
        gpio::output(PIN_5V_ON);
        gpio::high(PIN_5V_ON);
        // mark the RGB pin as output and set low
        gpio::output(PIN_RGB);
        gpio::low(PIN_RGB);
        // initialize the button pins to inputs with pullup
        gpio::inputPullup(PIN_BTN_1);
        gpio::inputPullup(PIN_BTN_2);
        gpio::inputPullup(PIN_BTN_3);
        gpio::inputPullup(PIN_BTN_4);
        // initialize all button groups to ouput & set them high
        gpio::output(PIN_BTN_DPAD);
        gpio::output(PIN_BTN_ABXY);
        gpio::output(PIN_BTN_CTRL);
        gpio::high(PIN_BTN_DPAD);
        gpio::high(PIN_BTN_ABXY);
        gpio::high(PIN_BTN_CTRL);
        // start the dpad
        gpio::low(PIN_BTN_DPAD);
    }

    static void loopTestButtons() __attribute__((always_inline)) {
        if (tick) {
            tick = false;
            switch (buttonGroup) {
                case 0:
                    rgb[0].r = gpio::read(PIN_BTN_1) ? 0 : 10;
                    rgb[1].r = gpio::read(PIN_BTN_2) ? 0 : 10;
                    rgb[2].r = gpio::read(PIN_BTN_3) ? 0 : 10;
                    rgb[3].r = gpio::read(PIN_BTN_4) ? 0 : 10;
                    buttonGroup = 1;
                    gpio::high(PIN_BTN_DPAD);
                    gpio::low(PIN_BTN_ABXY);
                    break;
                case 1:
                    rgb[0].g = gpio::read(PIN_BTN_1) ? 0 : 10;
                    rgb[1].g = gpio::read(PIN_BTN_2) ? 0 : 10;
                    rgb[2].g = gpio::read(PIN_BTN_3) ? 0 : 10;
                    rgb[3].g = gpio::read(PIN_BTN_4) ? 0 : 10;
                    buttonGroup = 2;
                    gpio::high(PIN_BTN_ABXY);
                    gpio::low(PIN_BTN_CTRL);
                    break;
                default: // really just 2
                    rgb[0].b = gpio::read(PIN_BTN_1) ? 0 : 10;
                    rgb[1].b = gpio::read(PIN_BTN_2) ? 0 : 10;
                    rgb[2].b = gpio::read(PIN_BTN_3) ? 0 : 10;
                    rgb[3].b = gpio::read(PIN_BTN_4) ? 0 : 10;
                    buttonGroup = 0;
                    gpio::high(PIN_BTN_CTRL);
                    gpio::low(PIN_BTN_DPAD);
                    break;
            }
            rgb.update(true);
        }
    }

    //@}

    /** \name RP2040 Test
     
        Powers on the 3V3 rail to RP2040. Press and hold of the start button disables the 3V3 rail, while pressing the select button drives the QSPI_SS to ground, forcing the RP2040 into bootloader mode.
     */
    //@{

    static inline bool rail3V3Reset = true;

    static void initializeTestRP2040() __attribute__((always_inline)) {
        initialize();
        // change RTC frequency to 128Hz
        while (RTC.PITSTATUS & RTC_CTRLBUSY_bm);
        RTC.PITCTRLA = RTC_PERIOD_CYC256_gc | RTC_PITEN_bm;
        // turn on the 5V rail 
        gpio::output(PIN_5V_ON);
        gpio::high(PIN_5V_ON);
        // mark the RGB pin as output and set low
        gpio::output(PIN_RGB);
        gpio::low(PIN_RGB);
        // initialize the button pins to inputs with pullup
        gpio::inputPullup(PIN_BTN_1);
        gpio::inputPullup(PIN_BTN_2);
        gpio::inputPullup(PIN_BTN_3);
        gpio::inputPullup(PIN_BTN_4);
        // initialize all button groups to ouput & set them high
        gpio::output(PIN_BTN_DPAD);
        gpio::output(PIN_BTN_ABXY);
        gpio::output(PIN_BTN_CTRL);
        gpio::high(PIN_BTN_DPAD);
        gpio::high(PIN_BTN_ABXY);
        gpio::high(PIN_BTN_CTRL);
        // start the dpad
        gpio::low(PIN_BTN_ABXY);
        

        rgb.clear();
        rgb.update(true);

        // enable the backlight at 25% intensity
        static_assert(PIN_PWM_BACKLIGHT == 1); // PA5, TCB0 WO
        gpio::input(PIN_PWM_BACKLIGHT);
        TCB0.CTRLA = 0;
        TCB0.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
        TCB0.CCMPL = 255;
        TCB0.CCMPH = 0; 
        TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc;
        gpio::output(PIN_PWM_BACKLIGHT);
        TCB0.CCMPH = 128;
        TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
    }

    static void loopTestRP2040() __attribute__((alwaysInline)) {
        if (tick) {
            tick = false;
            if (gpio::read(PIN_BTN_3) == rail3V3Reset) {
                rail3V3Reset = ! rail3V3Reset;
                if (rail3V3Reset) {
                    gpio::input(PIN_3V3_ON);
                    rgb[0].r = 0;
                } else {
                    gpio::output(PIN_3V3_ON);
                    gpio::high(PIN_3V3_ON);
                    rgb[0].r = 10;
                }
            }
            if (!gpio::read(PIN_BTN_4)) {
                rgb[3].g = 128;
                rgb.update();
                gpio::input(PIN_3V3_ON); // turn rp2040 off
                cpu::delayMs(100);
                gpio::output(PIN_QSPI_SS); // pull QSPI_CS low
                gpio::low(PIN_QSPI_SS);
                cpu::delayMs(100); 
                gpio::output(PIN_3V3_ON); // turn rpi on
                gpio::high(PIN_3V3_ON);
                cpu::delayMs(500);
                gpio::input(PIN_QSPI_SS); // QSPI_CS can be used
                cpu::delayMs(300); // so that the whole cycle takes 1 second
                rgb[3].g = 0;
                /*
                qspiSSLow = ! qspiSSLow;
                if (qspiSSLow) {
                    gpio::output(PIN_QSPI_SS);
                    gpio::low(PIN_QSPI_SS);
                    rgb[1].g = 10;
                } else {
                    gpio::input(PIN_QSPI_SS);
                    rgb[1].g = 0;
                }
                */
            }
            rgb.update();
        }
    }

    //@}

}; // RCKid


/** The RTC one second interval tick ISR. 
 */
ISR(RTC_PIT_vect) {
    RTC.PITINTFLAGS = RTC_PI_bm; // clear the interrupt
    RCKid::tick = true;
}

#define RUN_TEST(NAME) \
    void setup() { RCKid::initializeTest ## NAME (); } \
    void loop() { RCKid::loopTest ## NAME (); }

//RUN_TEST(LevelShifter)
//RUN_TEST(Neopixel)
RUN_TEST(Buttons)
//RUN_TEST(RP2040)