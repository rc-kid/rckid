
#include "platform/platform.h"
#include "platform/peripherals/neopixel.h"

#include "config.h"
#include "common.h"

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
        // immediately after startup, kill the power to 3V3 and 5V rails by ensuring they are configured as input pins w/o pull-up. (they should start as input pins anyways, but to be sure)
        gpio::initialize();
        gpio::input(PIN_3V3_ON);
        gpio::input(PIN_5V_ON);
        // enable 2 second watchdog so that the second tick resets it always with enough time to spare
        while (WDT.STATUS & WDT_SYNCBUSY_bm); // required busy wait
            _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_2KCLK_gc);                
        // set CLK_PER prescaler to 2, i.e. 10Mhz, which is the maximum the chip supports at voltages as low as 3.0V
        CCP = CCP_IOREG_gc;
        CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm;
        // initialize the RTC for timekeeping and the I/O so that we can wake up 
        initializeRTC();
        initializeButtonsAndIO();
        // ensure we will go sleep mode when the main loop is entered
        flags_.sleep = true;    
    }

    static void loop() __attribute__((always_inline)) {
        // if we should sleep, go to sleep immediately
        if (flags_.sleep)
            sleep();
        // otherwise perform the normal loop
        tick();
        if (flags_.secondTick)
            secondTick();
    }

    /** \name Power Control 

        Power and charging management.
     */
    //@{

    static void sleep() {
        // cut power to 3V3 and 5V rails
        gpio::input(PIN_3V3_ON);
        gpio::input(PIN_5V_ON);
        // turn off ADCs
        ADC0.CTRLA = 0;
        ADC1.CTRLA = 0;
        // turn off PWMs 
        TCB0.CTRLA = 0;
        TCB1.CTRLA = 0;
        gpio::input(PIN_PWM_BACKLIGHT);
        gpio::input(PIN_PWM_RUMBLER);
        // set the button pins to read the control buttons (home, cartridge, vol up/down)
        gpio::high(PIN_BTN_DPAD);
        gpio::high(PIN_BTN_ABXY);
        gpio::low(PIN_BTN_CTRL);
        // register ISR for the home button 
        PORTB.PIN4CTRL |= PORT_ISC_BOTHEDGES_gc;
        // initialize ADC1 to read voltage
        ADC1.CTRLB = ADC_SAMPNUM_ACC64_gc;
        ADC1.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; // 1.25MHz, reduced capacitance for >1V voltages
        ADC1.CTRLD = ADC_INITDLY_DLY128_gc;
        ADC1.MUXPOS = ADC_MUXPOS_INTREF_gc; // start measuring VCC
        ADC1.SAMPCTRL = 0;
        // time to go to sleep
        // the outer sleep cycle - ensures that if the wakeup conditions are not met (not long enough home button press or not enough power available)
        while (true) {
            // the inner sleep loop, terminated when home button is pressed (IRS), wakes up every second to check VCC (charging) and update the clock
            while (true) {
                cpu::sleep();
                // start reading the voltage
                ADC1.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc;
                ADC1.COMMAND = ADC_STCONV_bm;
                // TODO
                if (flags_.secondTick)
                    secondTick();
                // break sleep if the HOME button has been pressed
                if (state_.status.btnHome())
                    break;
                // otherwise wait for ADC0 to finish the VCC measurement and determine charging/DC accordingly
                // TODO
            }
            // wait for the ADC0 to finish the VCC measurement, 
        }            




    }

    
    //@}

    /** \name Communication 
     */
    //@{

    static void initializeComms() {
        // initialize the I2C in alternate position
        PORTMUX.CTRLB |= PORTMUX_TWI0_bm;
        // initalize the i2c slave with our address
        i2c::initializeSlave(AVR_I2C_ADDRESS);
    }

    //@}

    /** \name Timekeeping

        The AVR keeps track of time using the RTC oscillator and provides basic time-related functions such as alarm clock. 
     */
    //@{

    static void initializeRTC() {
        // initialize the RTC that fires every second for a semi-accurate real time clock keeping on the AVR, also start the timer
        RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;
        RTC.PITINTCTRL |= RTC_PI_bm; // enable the interrupt
        while (RTC.PITSTATUS & RTC_CTRLBUSY_bm);
        RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc | RTC_PITEN_bm;
    }

    static void secondTick() {
        flags_.secondTick = false;
        // TODO update time & uptime
    }

    //@}

    /** \name Buttons & IO
        
        The buttons are connected in a matrix of 3 button groups and 4 buttons per group. 
     */
    //@{
    static void initializeButtonsAndIO() {
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
        // turn off pullups and digital I/O on ADC connected pins (headphones, charging)
        static_assert(PIN_HEADPHONES == 2); // PA6, ADC1 AIN2
        PORTA.PIN6CTRL &= ~PORT_ISC_gm;
        PORTA.PIN6CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTA.PIN6CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(PIN_CHARGING == 11); // PC1, ADC1 AIN7
        PORTC.PIN1CTRL &= ~PORT_ISC_gm;
        PORTC.PIN1CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTC.PIN1CTRL &= ~PORT_PULLUPEN_bm;
        // initialize ADC1 to continuously read VCC, headphones and charging info
        ADC1.CTRLA = 0; // turn off
        ADC1.CTRLB = ADC_SAMPNUM_ACC64_gc;
        ADC1.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; // 1.25MHz, reduced capacitance for >1V voltages
        ADC1.CTRLD = ADC_INITDLY_DLY32_gc;
        ADC1.MUXPOS = ADC_MUXPOS_INTREF_gc; // start measuring VCC
        ADC1.SAMPCTRL = 0;
        ADC1.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc;
        ADC1.COMMAND = ADC_STCONV_bm; // start the conversion
    }

    static bool tick() {
        if (! (ADC1.INTFLAGS & ADC_RESRDY_bm))
            return false;
        uint16_t value = ADC1.RES / 64;
        uint8_t muxpos = ADC1.MUXPOS;
        switch (muxpos) {
            case ADC_MUXPOS_INTREF_gc:
                ADC1.MUXPOS = ADC_MUXPOS_INTREF_gc;
                break;
            case ADC_MUXPOS_AIN2_gc:
                ADC1.MUXPOS = ADC_MUXPOS_AIN7_gc;
                break;
            case ADC_MUXPOS_AIN7_gc:
            default:
                ADC1.MUXPOS = ADC_MUXPOS_INTREF_gc;
                break;
        }
        ADC1.COMMAND = ADC_STCONV_bm;
        // now that the conversion is running, process the value and check the appropriate buttons bank
        switch (muxpos) {
            case ADC_MUXPOS_INTREF_gc:
                value = 110 * 512 / value;
                value = value * 2;
                state_.info.setVcc(value);
                state_.status.setDCPower(value > VCC_DC_POWER_THRESHOLD);
                readBtnCtrl();
                gpio::high(PIN_BTN_CTRL);
                gpio::low(PIN_BTN_DPAD);
                break;
            case ADC_MUXPOS_AIN2_gc:
                state_.status.setHeadphones(state_.status.audioEnabled() && value < HEADPHONES_DETECTION_THRESHOLD);
                readBtnDpad();
                gpio::high(PIN_BTN_DPAD);
                gpio::low(PIN_BTN_ABXY);
                break;
            case ADC_MUXPOS_AIN7_gc:
                state_.status.setCharging(state_.status.dcPower() && (value < CHARGING_DETECTION_THRESHOLD));
                readBtnABXY();
                gpio::high(PIN_BTN_ABXY);
                gpio::low(PIN_BTN_CTRL);
                break;
            default:
                break;
        }
        return true;
    }

    static void readBtnCtrl() {
        flags_.cartridgeIn = ! gpio::read(PIN_BTN_1);
        // TODO shold we do anything if cartridge not present? 
        state_.status.setBtnHome(!gpio::read(PIN_BTN_2));
        state_.status.setBtnVolUp(!gpio::read(PIN_BTN_3));
        state_.status.setBtnVolDown(!gpio::read(PIN_BTN_4));
    }

    static void readBtnDpad() {
        state_.status.setDpadLeft(!gpio::read(PIN_BTN_2));
        state_.status.setDpadRight(!gpio::read(PIN_BTN_4));
        state_.status.setDpadUp(!gpio::read(PIN_BTN_3));
        state_.status.setDpadDown(!gpio::read(PIN_BTN_1));
    }

    static void readBtnABXY() {
        state_.status.setBtnA(!gpio::read(PIN_BTN_2));
        state_.status.setBtnB(!gpio::read(PIN_BTN_1));
        state_.status.setBtnStart(gpio::read(PIN_BTN_3));
        state_.status.setBtnSelect(gpio::read(PIN_BTN_4));
    }
    //@}

    /** \name PWM (rumbler and backlight)

        The PWM signals used for backlight and rumbler control are generated by the TCB0 and TCB1 respectively.

        Backlight is pulled low externally, setting the pin to 1 make the backlight work, hence the value is unchanged.  

     */
    //@{
    static void initializePWM() {
        // do not leak voltage and turn the pins as inputs
        static_assert(PIN_PWM_BACKLIGHT == 1); // PA5, TCB0 WO
        gpio::input(PIN_PWM_BACKLIGHT);
        TCB0.CTRLA = 0;
        TCB0.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
        TCB0.CCMPL = 255;
        TCB0.CCMPH = 0; 
        TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc;
        static_assert(PIN_PWM_RUMBLER == 16); // PA3, TCB1 WO
        gpio::input(PIN_PWM_RUMBLER);
        TCB1.CTRLA = 0;
        TCB1.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
        TCB1.CCMPL = 255;
        TCB1.CCMPH = 0; 
        TCB1.CTRLA = TCB_CLKSEL_CLKDIV2_gc;
     }

    static void setBacklightPWM(uint8_t value) {
        state_.info.setBacklight(value);
        if (value == 0) {
            TCB0.CTRLA = 0;
            gpio::input(PIN_PWM_BACKLIGHT);
        } else if (value == 255) {
            TCB0.CTRLA = 0;
            gpio::output(PIN_PWM_BACKLIGHT);
            gpio::high(PIN_PWM_BACKLIGHT);
        } else {
            gpio::output(PIN_PWM_BACKLIGHT);
            TCB0.CCMPH = value;
            TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
        }
    }

    static void setRumblerPWM(uint8_t value) {
        if (value == 0) {
            TCB1.CTRLA = 0;
            gpio::input(PIN_PWM_RUMBLER);
        } else if (value == 255) {
            TCB1.CTRLA = 0;
            gpio::output(PIN_PWM_RUMBLER);
            gpio::high(PIN_PWM_RUMBLER);
        } else {
            gpio::output(PIN_PWM_BACKLIGHT);
            TCB1.CCMPH = 255 - value;
            TCB1.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
        }
    }

    //@}

    /** \name RGB LEDs

        In total we have four LEDs (A & B buttons and dpad for light effects and the upper right corner for notifications). 
     */
    //@{

    static inline NeopixelStrip<4> rgb_{PIN_RGB};
    static inline ColorStrip<4> rgbTarget_;

    static void rgbOn() {
        gpio::output(PIN_5V_ON);
        gpio::high(PIN_5V_ON);
        gpio::output(PIN_RGB);
        gpio::low(PIN_RGB);
    }

    static void rgbOff() {
        gpio::input(PIN_RGB);
        gpio::input(PIN_5V_ON);
    }
    //@}

    static inline State state_;

    static inline volatile struct {
        bool secondTick : 1;
        bool cartridgeIn : 1;
        bool sleep: 1;
    } flags_;

}; // RCKid

/** The RTC one second interval tick ISR. 
 */
ISR(RTC_PIT_vect) {
    RTC.PITINTFLAGS = RTC_PI_bm; // clear the interrupt
    RCKid::flags_.secondTick = true;
}

/** Called when the home button is pressed or released while in sleep mode. Simply update the home button flag, which will be picked up by the sleep() method and acted accordingly. 
 */
ISR(PORTB_PORT_vect) {
    VPORTB.INTFLAGS = (1 << 4); // PB4 (BTN_2, connected to Home button)
    RCKid::state_.status.setBtnHome(!gpio::read(PIN_BTN_2));
}


void setup() { RCKid::initialize(); }
void loop() { RCKid::loop(); }
