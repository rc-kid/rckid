
#include "platform/platform.h"
#include "platform/peripherals/neopixel.h"

#include "rckid/config.h"
#include "rckid/state.h"

using namespace platform;
using namespace rckid;

#define NO_ISR(...) do { cli(); __VA_ARGS__; sei(); } while (false)

class RCKid {
public:

    static void initialize()  __attribute__((always_inline)) {
        // immediately after startup, kill the power to 3V3 and 5V rails by ensuring they are configured as input pins w/o pull-up. (they should start as input pins anyways, but to be sure)
        gpio::initialize();
        gpio::input(AVR_PIN_3V3_ON);
        gpio::input(AVR_PIN_5V_ON);
        // enable 2 second watchdog so that the second tick resets it always with enough time to spare
        while (WDT.STATUS & WDT_SYNCBUSY_bm); // required busy wait
            _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_2KCLK_gc);                
        // set CLK_PER prescaler to 2, i.e. 10Mhz, which is the maximum the chip supports at voltages as low as 3.0V
        CCP = CCP_IOREG_gc;
        CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm;
        // initialize the RTC that fires every second for a semi-accurate real time clock keeping on the AVR, also start the timer
        RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;
        RTC.PITINTCTRL |= RTC_PI_bm; // enable the interrupt
        while (RTC.PITSTATUS & RTC_CTRLBUSY_bm);
        RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc | RTC_PITEN_bm;
        // initialize the button pins using the matrix configuration
        // initialize the button pins to inputs with pullup
        gpio::inputPullup(AVR_PIN_BTN_1);
        gpio::inputPullup(AVR_PIN_BTN_2);
        gpio::inputPullup(AVR_PIN_BTN_3);
        gpio::inputPullup(AVR_PIN_BTN_4);
        // initialize all button groups to ouput & set them high
        gpio::output(AVR_PIN_BTN_DPAD);
        gpio::output(AVR_PIN_BTN_ABXY);
        gpio::output(AVR_PIN_BTN_CTRL);
        gpio::high(AVR_PIN_BTN_DPAD);
        gpio::high(AVR_PIN_BTN_ABXY);
        gpio::low(AVR_PIN_BTN_CTRL); // start reading the ctrl buttons
        // enable rising edge on the charging detection 
        static_assert(AVR_PIN_CHARGING == 11); // PC1
        PORTC.PIN1CTRL |= PORT_ISC_RISING_gc;


        // TODO DELETE ME -- we want no debug mode & sleep instead of wakeup in the initialization in release 
        state_.info.setDebugMode();
        wakeup();
        powerOn();
    }

    static void loop()  __attribute__((always_inline)) {
        if (mode_ == Mode::Sleep)
            sleep();
        if (checkADC0()) {
            checkButtons();
            checkLongHomePress();
            /*
            if (mode_ == Mode::Wakeup)
                checkWakeup();
            else 
                checkLongHomePress();
            */
        }
        if (secondTick_)
            secondTick();
    }

    static void secondTick() {
        secondTick_ = false;
        // reset the watchdog. Resetting here means that (a) the RTC interrupt is working and (b) the main loop is working and progressing. We don't care about the rp here - if it freezes, the home button can always be used to reset the entire system. 
        wdt::reset();
        // TODO advance clock
    }



    /** \name Power Management
     * 
     */
    //@{
 
    enum class Mode {
        Sleep, 
        Wakeup,
        On, 
        Charging
    }; 

    static inline volatile Mode mode_;
    static inline volatile uint16_t homeCounter_ = 0;

    static void sleep() {
        // disable power rails
        power3v3(false);
        power5v(false);
        // turn off ADC and PWM circuitry
        ADC0.CTRLA = 0;
        TCB0.CTRLA = 0;
        TCB1.CTRLA = 0;
        // make PWM pins input so that we do not leak power
        gpio::input(AVR_PIN_PWM_BACKLIGHT);
        gpio::input(AVR_PIN_PWM_RUMBLER);
        // enable the CTRL bank of buttons for continuous checking for wakeup
        gpio::high(AVR_PIN_BTN_DPAD);
        gpio::high(AVR_PIN_BTN_ABXY);
        gpio::low(AVR_PIN_BTN_CTRL);
        // enable falling edge interrupt on the home button to power the device on 
        static_assert(AVR_PIN_BTN_2 == 5); // PB4
        PORTB.PIN4CTRL |= PORT_ISC_FALLING_gc;
        // resume sleep after each second tick
        while (mode_ == Mode::Sleep) {
            cpu::sleep();
            if (secondTick_)
                secondTick();
        }
        if (mode_ == Mode::Wakeup)
            wakeup();
        // TODO what to do with charging? 
    }

    /** Wakeup sequence. Start the ADC, and mark the 
     */
    static wakeup() {
        // disable home interrupt
        static_assert(AVR_PIN_BTN_2 == 5); // PB4
        PORTB.PIN4CTRL &= ~PORT_ISC_gm;

        state_.status.setBtnHome(true);
        initializeADC0();
        initializePWM();
        i2c::initializeSlave(AVR_I2C_ADDRESS);
        homeCounter_ = BTN_HOME_POWER_ON_DURATION;
        state_.status.setBtnHome(true);
    }

    /** Checks that the wakeup condition (long press home button is pressed)
     */
    static void checkLongHomePress() {
        if (homeCounter_ > 0) {
            if (state_.status.btnHome()) {
                if (--homeCounter_ == 0) {
                    if (mode_ == Mode::Wakeup)
                        powerOn();
                    else 
                        mode_ = Mode::Sleep;
                }
            } else {
                if (mode_ == Mode::Wakeup)
                    mode_ = Mode::Sleep;
                homeCounter_ = 0;
            }
        }
    }

    /** The poweron sequence that happens when the long enough home button press has been detected. Enables the 3V3 rail in normal mode for the RPI 
     */
    static void powerOn() {
        homeCounter_ = 0;
        mode_ = Mode::On;
        // TODO do rumbler effect
        power3v3(true);
        // when in debug mode, set the backlight to half power immediately so that we can see what's happening on the screen
        if (state_.info.debugMode())
            setBacklightPWM(128); 
    }

    static void power5v(bool state) {
        if (state) {
            gpio::output(AVR_PIN_5V_ON);
            gpio::high(AVR_PIN_5V_ON);
        } else {
            gpio::input(AVR_PIN_5V_ON);
        }
    }

    static void power3v3(bool state) {
        if (state) {
            gpio::output(AVR_PIN_3V3_ON);
            gpio::high(AVR_PIN_3V3_ON);
        } else {
            gpio::input(AVR_PIN_3V3_ON);
        }
    }

    /** Resets the rpi pico by turning off/on the 3V3 rail. Flashes all RGB leds red and enabled display backlight. 
     */
    static void rpReset() {
        power3v3(false);
        rgbOn();
        rgbFill(Color::Red());
        cpu::delayMs(1000);
        rgbOff();
        power3v3(true);
        setBacklightPWM(128);
    }

    /** Makes the rpi pico enter the bootloader mode by resetting it with QSPI pin pulled low. Flashes all RGB leds green and enables display backlight.
     */
    static void rpBootloader() {
        power3v3(false);
        rgbOn();
        rgbFill(Color::Blue());
        gpio::output(AVR_PIN_QSPI_SS);
        gpio::low(AVR_PIN_QSPI_SS);
        cpu::delayMs(500);
        power3v3(true);
        setBacklightPWM(128);
        cpu::delayMs(300);
        gpio::input(AVR_PIN_QSPI_SS);
        rgbOff();
    }

    //@}

    /** \name I2C Comms 
     
        The communication is rather simple - an I2C slave that when read from returns the state buffer and when written to, stores data in the state's comms buffer. The data will be interpreted as a command and performed after the stop condition is received.

        This mode simplifies the AVR part and prioritizes short communication burts for often needed data, while infrequent operations, such as full state and even EEPROM data reads take more time. 
     */
    //@{

    static inline volatile uint8_t i2cTxIdx_ = 0;
    static inline volatile uint8_t i2cRxIdx_ = 0;
    static inline volatile bool i2cCommandReady_ = false;

    /** The I2C interrupt handler. 
     */
    static inline void TWI0_TWIS_vect() __attribute__((always_inline)) {
        #define I2C_DATA_MASK (TWI_DIF_bm | TWI_DIR_bm) 
        #define I2C_DATA_TX (TWI_DIF_bm | TWI_DIR_bm)
        #define I2C_DATA_RX (TWI_DIF_bm)
        #define I2C_START_MASK (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm)
        #define I2C_START_TX (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm)
        #define I2C_START_RX (TWI_APIF_bm | TWI_AP_bm)
        #define I2C_STOP_MASK (TWI_APIF_bm | TWI_DIR_bm)
        #define I2C_STOP_TX (TWI_APIF_bm | TWI_DIR_bm)
        #define I2C_STOP_RX (TWI_APIF_bm)
        uint8_t status = TWI0.SSTATUS;
        // sending data to accepting master simply starts sending the state_ buffer. 
        if ((status & I2C_DATA_MASK) == I2C_DATA_TX) {
            TWI0.SDATA = ((uint8_t*) & state_)[i2cTxIdx_];
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
            ++i2cTxIdx_;
            // TODO send nack when done sending all state
        // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
        } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
            state_.buffer[i2cRxIdx_++] = TWI0.SDATA;
            TWI0.SCTRLB = (i2cRxIdx_ == sizeof(state_.buffer)) ? TWI_SCMD_COMPTRANS_gc : TWI_SCMD_RESPONSE_gc;
        // master requests slave to write data, reset the sent bytes counter, initialize the actual read address from the read start and reset the IRQ
        } else if ((status & I2C_START_MASK) == I2C_START_TX) {
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc + TWI_SCMD_RESPONSE_gc;
        // master requests to write data itself. ACK if there is no pending I2C message, NACK otherwise. The buffer is reset to 
        } else if ((status & I2C_START_MASK) == I2C_START_RX) {
            TWI0.SCTRLB = (i2cRxIdx_ == 0) ? TWI_SCMD_RESPONSE_gc : TWI_ACKACT_NACK_gc;
        // sending finished, reset the tx address and when in recording mode determine if more data is available
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_TX) {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            i2cTxIdx_ = 0;
        // receiving finished, inform main loop we have message waiting if we have received at laast one byte (0 bytes received is just I2C ping)
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_RX) {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            if (i2cRxIdx_ > 0)
                i2cCommandReady_ = true;
        } else {
            // error - a state we do not know how to handle
        }
    }

    //@}

    /** \name Buttons and analog inputs
     
        ADC0 is used to continuously sample 3 channels: 
        
        - internal voltage to determine the AVR's voltage (which is poor man's approximation of the battery voltage when running on batteries and the DC voltage detection if greater than battery max)
        - internal temperature sensor (because why not)
        - headphones jack (because it runs on 3.0V tops, which may not triger as digital 1 at higher voltages)

        Buttons are connected via a matrix with three groups (control, dpad and abxy) with 4 buttons in each group. 
     */
    //@{

    // TODO maybe we want this only 32 accumulated samples for faster speed
    static void initializeADC0() {
        static_assert(AVR_PIN_HEADPHONES == 2); // PA6, ADC0 AIN6
        //PORTA.PIN6CTRL &= ~PORT_ISC_gm;
        //PORTA.PIN6CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        //PORTA.PIN6CTRL &= ~PORT_PULLUPEN_bm;
        // do not disable digital circuitry on the pin as we are using it for output as well to turn off audio entirely (and we don't really care about ADC performance) 
        gpio::input(AVR_PIN_HEADPHONES);
        // initialize ADC1 to continuously read VCC, headphones and charging info
        ADC0.CTRLA = 0; // turn off
        ADC0.CTRLB = ADC_SAMPNUM_ACC32_gc;
        ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; // 1.25MHz, reduced capacitance for >1V voltages
        ADC0.CTRLD = ADC_INITDLY_DLY32_gc;
        ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc; // start measuring VCC
        ADC0.SAMPCTRL = 31;
        ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc;
        ADC0.COMMAND = ADC_STCONV_bm; // start the conversion
    }

    /** Checks the ADC0 measurements (vcc, temp and headphones)
     */
    static bool checkADC0() {
        if (! (ADC0.INTFLAGS & ADC_RESRDY_bm))
            return false;
        uint16_t value = ADC0.RES / 64;
        uint8_t muxpos = ADC0.MUXPOS;
        // determine next muxpos and start next conversion 
        switch (muxpos) {
            case ADC_MUXPOS_INTREF_gc:
                ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_INTREF_gc | ADC_SAMPCAP_bm; 
                break;
            case ADC_MUXPOS_TEMPSENSE_gc:
                ADC0.MUXPOS = ADC_MUXPOS_AIN6_gc;
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; 
            case ADC_MUXPOS_AIN6_gc:
            default:
                ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;
                break;
        }
        ADC0.COMMAND = ADC_STCONV_bm;
        // while already measuring next, process the currently measured value
        switch (muxpos) {
            case ADC_MUXPOS_INTREF_gc: {
                value = 110 * 512 / value;
                value = value * 2;
                NO_ISR(state_.info.setVcc(value));
                // TODO deal with threshold vcc values
                break;
            }
            case ADC_MUXPOS_TEMPSENSE_gc: {
                int8_t sigrow_offset = SIGROW.TEMPSENSE1; 
                uint8_t sigrow_gain = SIGROW.TEMPSENSE0;
                int32_t t = value - sigrow_offset; // Result might overflow 16 bit variable (10bit+8bit)
                t *= sigrow_gain;
                // temp is now in kelvin range, to convert to celsius, remove -273.15 (x256)
                t -= 69926;
                // and now loose precision to 0.5C (x10, i.e. -15 = -1.5C)
                value = (t >>= 7) * 5;
                NO_ISR(state_.info.setTemp(value));
                break;
            }
            case ADC_MUXPOS_AIN6_gc: {
                if (state_.status.audioEnabled()) {
                    bool headphones = value > HEADPHONES_DETECTION_THRESHOLD;
                    NO_ISR(state_.status.setHeadphones(headphones));
                }
                break;
            }
        }
        return true;
    }

    static void checkButtons() {
        uint16_t value;
        if (gpio::read(AVR_PIN_BTN_CTRL) == 0) {
            if (state_.info.debugMode()) {
                if (! gpio::read(AVR_PIN_BTN_3))
                    rpReset();
                else if (! gpio::read(AVR_PIN_BTN_4))
                    rpBootloader();
                value = Status::calculateControlValue(
                    !gpio::read(AVR_PIN_BTN_2), // home
                    false, // vol up
                    false // vol down
                );
            } else {
                value = Status::calculateControlValue(
                    !gpio::read(AVR_PIN_BTN_2), // home
                    !gpio::read(AVR_PIN_BTN_3), // vol up
                    !gpio::read(AVR_PIN_BTN_4) // vol down
                );
            }
            // if this is home button press, start the long press countdown
            if (!state_.status.btnHome() && ! gpio::read(AVR_PIN_BTN_2))
                homeCounter_ = BTN_HOME_POWER_OFF_DURATION;
            NO_ISR(state_.status.setControlValue(value));
            // move the matrix to dpad
            gpio::high(AVR_PIN_BTN_CTRL);
            gpio::low(AVR_PIN_BTN_DPAD);
        } else if (gpio::read(AVR_PIN_BTN_DPAD) == 0) {
            value = Status::calculateDPadValue(
                !gpio::read(AVR_PIN_BTN_2), // left
                !gpio::read(AVR_PIN_BTN_4), // right
                !gpio::read(AVR_PIN_BTN_3), // up
                !gpio::read(AVR_PIN_BTN_1) // down
            );
            NO_ISR(state_.status.setDPadValue(value));
            // move the matrix to ABXY
            gpio::high(AVR_PIN_BTN_DPAD);
            gpio::low(AVR_PIN_BTN_ABXY);
        } else if (gpio::read(AVR_PIN_BTN_ABXY) == 0) {
            value = Status::calculateABXYValue(
                !gpio::read(AVR_PIN_BTN_2), // a
                !gpio::read(AVR_PIN_BTN_1), // b
                !gpio::read(AVR_PIN_BTN_4), // sel
                !gpio::read(AVR_PIN_BTN_3) // start
            );
            NO_ISR(state_.status.setABXYValue(value));
            // move the matrix to ctrl
            gpio::high(AVR_PIN_BTN_ABXY);
            gpio::low(AVR_PIN_BTN_CTRL);
        } else {
            // this should really not happen, but if it does, reset to read CTRL buttons next time
            gpio::high(AVR_PIN_BTN_DPAD);
            gpio::high(AVR_PIN_BTN_ABXY);
            gpio::low(AVR_PIN_BTN_CTRL);
        }
    }
    //@}

    /** \name PWM (rumbler and backlight)

        The PWM signals used for backlight and rumbler control are generated by the TCB0 and TCB1 respectively.

        Backlight is pulled low externally, setting the pin to 1 make the backlight work, hence the value is unchanged.  

     */
    //@{

    static void initializePWM() {
        // do not leak voltage and turn the pins as inputs
        static_assert(AVR_PIN_PWM_BACKLIGHT == 1); // PA5, TCB0 WO
        gpio::input(AVR_PIN_PWM_BACKLIGHT);
        TCB0.CTRLA = 0;
        TCB0.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
        TCB0.CCMPL = 255;
        TCB0.CCMPH = 0; 
        TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc;
        static_assert(AVR_PIN_PWM_RUMBLER == 16); // PA3, TCB1 WO
        gpio::input(AVR_PIN_PWM_RUMBLER);
        TCB1.CTRLA = 0;
        TCB1.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
        TCB1.CCMPL = 255;
        TCB1.CCMPH = 0; 
        TCB1.CTRLA = TCB_CLKSEL_CLKDIV2_gc;
     }

    static void setBacklightPWM(uint8_t value) {
        if (value == 0) {
            TCB0.CTRLA = 0;
            gpio::input(AVR_PIN_PWM_BACKLIGHT);
        } else if (value == 255) {
            TCB0.CTRLA = 0;
            gpio::output(AVR_PIN_PWM_BACKLIGHT);
            gpio::high(AVR_PIN_PWM_BACKLIGHT);
        } else {
            gpio::output(AVR_PIN_PWM_BACKLIGHT);
            TCB0.CCMPH = value;
            TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
        }
    }

    static void setRumblerPWM(uint8_t value) {
        if (value == 0) {
            TCB1.CTRLA = 0;
            gpio::input(AVR_PIN_PWM_RUMBLER);
        } else if (value == 255) {
            TCB1.CTRLA = 0;
            gpio::output(AVR_PIN_PWM_RUMBLER);
            gpio::high(AVR_PIN_PWM_RUMBLER);
        } else {
            gpio::output(AVR_PIN_PWM_BACKLIGHT);
            TCB1.CCMPH = 255 - value;
            TCB1.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
        }
    }
    //@}

    /** \name RGB LEDs

        In total we have four LEDs (A & B buttons and dpad for light effects and the upper right corner for notifications). 

        TODO effects /notifications
     */
    //@{

    static inline NeopixelStrip<4> rgb_{AVR_PIN_RGB};
    static inline ColorStrip<4> rgbTarget_;

    static void rgbOn() {
        power5v(true);
        gpio::output(AVR_PIN_RGB);
        gpio::low(AVR_PIN_RGB);
        // allow the rail to stabilize
        cpu::delayMs(20);
    }

    static void rgbOff() {
        // note the order is important so that there are no stray voltages on the control line
        gpio::input(AVR_PIN_RGB);
        power5v(false);
    }

    static void rgbFill(Color color) {
        rgb_.fill(color);
        rgb_.update(true);
    }
    //@}


    static inline State state_;
    static inline volatile bool secondTick_ = false;

}; // RCKid

/** The RTC one second interval tick ISR. 
 */
ISR(RTC_PIT_vect) {
    RTC.PITINTFLAGS = RTC_PI_bm; // clear the interrupt
    RCKid::secondTick_ = true;
}

ISR(TWI0_TWIS_vect) { 
    RCKid::TWI0_TWIS_vect(); 
}

ISR(PORTB_PORT_vect) {
    static_assert(AVR_PIN_BTN_2 == 5); // PB4, connected to Home button
    VPORTB.INTFLAGS = (1 << 4); 
    RCKid::mode_ = RCKid::Mode::Wakeup;
}

ISR(PORTC_PORT_vect) {
    static_assert(AVR_PIN_CHARGING == 11); // PC1
    VPORTC.INTFLAGS = (1 << 1); 
    // TODO start charing animation 
}

void setup() { RCKid::initialize(); }
void loop() { RCKid::loop(); }
