#include <avr/sleep.h>
#include <avr/delay.h>
#include <avr/interrupt.h>

#include <platform.h>
#include <ina219.h>
#include <neopixel.h>

#include "common/config.h"
#include "common/state.h"
#include "common/commands.h"

/** Displays AVR status and events on an external SSD1306 OLED display. Note this flag is only for debug purposes when writing own AVR code and that the AVR will hang & be reset via wdt if this is enabled and the oled display is not attached. 
*/
#define RCKID_AVR_DEBUG_OLED_DISPLAY_


/** Flag that enables code for the deprecated version 2.2. This includes having headphones on ADC1 instead of ADC2 and having the charger disable as high as opposed to floating. */
#define DEPRECATED_VERSION_2_2

// //*
#define BEGIN_ACTIVE_MODE do {} while (false) 
#define END_ACTIVE_MODE do {} while (false)
//   */

//#define BEGIN_ACTIVE_MODE gpio::outputHigh(AVR_PIN_DISP_RDX)
//#define END_ACTIVE_MODE gpio::outputLow(AVR_PIN_DISP_RDX);





#if (defined RCKID_AVR_DEBUG_OLED_DISPLAY)
#include <ssd1306.h>
#endif

using namespace rckid;


/** RCKid device controller
 
    The following peripherals are used:

    - RTC periodic interrupt in 1 second intervals for timekeeping and for system ticks
    - TCB0 and TCB1 for PWM control of the rumbler and backlight
    - ADC0 for reading charging status, vbatt & vcc and temperature
    - TWI for communication with RP2040 via I2C or as GPIO pins to wake RP2040 up from sleep

    Timekeeping

    Timekeeping is done by the RTC PIT which is one the few peripherals available in the power down mode. The PIT is operated at different frequencies at different device modes. 

    
*/
class RCKid {
public:
    /** System tick counter. Two LSBs determine the systemTick type for the per frame operation (4 types, total ~10 ms interval )
     */
    static inline uint16_t tick_;

    static inline TransferrableState ts_;

    /** Counter for the home button long press. 
     */
    static inline uint16_t btnHomeCounter_ = 0;

    /** Number of measurmeents for the VCC before reaction to prevent glitches. 
     */
    static inline uint8_t vccMeasurementCounter_ = 0;

    /** Counter to measure the current consumed by the device by the INA219. 
     */
    static inline uint8_t iMeasureCounter_ = 0;
    static inline uint16_t current_ = 0;

#if (RCKID_INA219_I2C_ADDRESS != 0)
    static inline platform::INA219 ina_{RCKID_INA219_I2C_ADDRESS};
#endif

    static inline platform::NeopixelStrip<6> rgbs_{AVR_PIN_RGB}; 
    static inline platform::ColorStrip<6> rgbsTarget_;
    static inline RGBEffect rgbEffects_[6];
    static inline volatile bool rgbTick_ = false;

    static inline RumblerEffect rumblerEffect_;
    static inline RumblerEffect rumblerCurrent_;
    static inline volatile bool rumblerTick_ = false;

    /** Determines if the AVR needs to sleep in standby mode alone, or if the power-down mode can be used. A non-zero value requires standby sleep mode. 
     */
    static inline uint8_t standbyRequired_ = 0;
    static constexpr uint8_t STANDBY_REQUIRED_BRIGHTNESS = 1;
    static constexpr uint8_t STANDBY_REQUIRED_RUMBLER = 2;
    static constexpr uint8_t STANDBY_REQUIRED_ADC0 = 4;
    static constexpr uint8_t STANDBY_REQUIRED_ADC1 = 8;

#if (defined RCKID_AVR_DEBUG_OLED_DISPLAY)
    static inline platform::SSD1306 oled_;
#endif


    /** \name Device Management 
     
        When initialized (chip power on), the device 
         
     */
    //@{

    /** Initialize the peripherals. 
     */
    static void initialize() __attribute__((always_inline)) {
        // on power on, we can assume chip is in a known state and all pins are in highZ state, which translates to both 3V3 and 5V rails powered off, no interference with charging and no bleeding into headphones, QSPI, display, or charge detect

        // enable 2 second watchdog so that the second tick resets it always with enough time to spare
        while (WDT.STATUS & WDT_SYNCBUSY_bm); // required busy wait
            _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_2KCLK_gc);      
        // set CLK_PER prescaler to 2, i.e. 10Mhz, which is the maximum the chip supports at voltages as low as 3.0V
        CCP = CCP_IOREG_gc;
        CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm;
        // initialize the RTC that fires every second for a semi-accurate real time clock keeping on the AVR and start counting
        RTC.CLKSEL = RTC_CLKSEL_INT32K_gc; // run from the internal 32.768kHz oscillator
        RTC.PITINTCTRL |= RTC_PI_bm; // enable the PIT interrupt
        // initialize ADC0 common properties without turning it on
        ADC0.CTRLB = ADC_SAMPNUM_ACC32_gc;
        ADC0.CTRLD = ADC_INITDLY_DLY32_gc;
        ADC0.SAMPCTRL = 31;
        ADC0.INTCTRL = ADC_RESRDY_bm;
        // same for ADC1, which is now used for headphones detection. 
        // TODO remove this in new version when headphones are reported on ADC0
        ADC1.CTRLB = ADC_SAMPNUM_ACC32_gc;
        ADC1.CTRLD = ADC_INITDLY_DLY32_gc;
        ADC1.SAMPCTRL = 31;
        ADC1.INTCTRL = ADC_RESRDY_bm;
        // set voltage reference to 1v1 for temperature checking
        VREF.CTRLA &= ~ VREF_ADC0REFSEL_gm;
        VREF.CTRLA |= VREF_ADC0REFSEL_1V1_gc;
        // initialize the home button to input pullup and set its ISR
        gpio::setAsInputPullup(AVR_PIN_BTN_HOME);
        static_assert(AVR_PIN_BTN_HOME == B2); // otherwise the ISR won't work
        PORTB.PIN2CTRL |= PORT_ISC_FALLING_gc;
        // be a pessimist and ensure we sleep in standby first, only going to powerdown if really warranted
        set_sleep_mode(SLEEP_MODE_STANDBY);
        // initialize the transferrable state, start in normal mode (power on on power on)
        // force the debug mode on startup
        ts_.state.setABXYKeys(false, false, true, false);
        setDeviceMode(DeviceMode::Normal);
        ts_.state.setABXYKeys(false, false, false, false);
        sei();
        // determine the last error
        ts_.error = AVR_POWER_ON;
        if (RSTCTRL.RSTFR & RSTCTRL_WDRF_bm)
            ts_.error = AVR_ERROR_WDT;
        if (RSTCTRL.RSTFR & RSTCTRL_BORF_bm)
            ts_.error = AVR_ERROR_BOD;
        // debug display
#if (defined RCKID_AVR_DEBUG_OLED_DISPLAY)
        oled_.initialize128x32();
        oled_.clear32();
        oled_.write(0, 0, "RCKid AVR INIT_DONE");        
#endif
        // TODO Debug enable breathe effect on RGB 0
        power5v(true);
        cpu::delayMs(50);
        rgbEffects_[0] = RGBEffect::Rainbow(0, 50, 1, 128);
        rgbEffects_[1] = RGBEffect::Rainbow(75, 40, 1, 128);
        rgbEffects_[3] = RGBEffect::Rainbow(180, 30, 1, 128);
        rgbEffects_[4] = RGBEffect::Rainbow(110, 20, 1, 128);
        rgbEffects_[5] = RGBEffect::Rainbow(230, 10, 1, 128);
    }

    /** The main loop. 
     
        Making the AVR code really simple, the main loop simply runs and services any interrupts, while sleeping between the runs to conserve as much power as possible. 
     */
    static void loop() {
        END_ACTIVE_MODE;
#if (defined RCKID_AVR_DEBUG_OLED_DISPLAY)
        oled_.clear32();
#endif
        while (true) {
            //BEGIN_ACTIVE_MODE;
            //gpio::outputHigh(AVR_PIN_DISP_RDX);
            cpu::wdtReset();
            // if there is I2C message, process
            if (i2cCommandReady_)
                processI2CCommand();
#if (RCKID_INA219_I2C_ADDRESS != 0)
            if (power3v3Active() && iMeasureCounter_ == 0) {
                current_ = ina_.current(); // ina_.initialize(platform::INA219::Gain::mv_40, 10);
                iMeasureCounter_ = RCKID_CURRENT_SENSE_TIMEOUT_TICKS;
            }
#endif
            if (rgbTick_)
                rgbTick();
            if (rumblerTick_)
                rumblerTick();
            // make sure interrupts are enabled or we won't wake up, the appropriate sleep mode has already been set by the various peripheral interactions so we can happily go to sleep here
            sei();
            sleep_enable();
            END_ACTIVE_MODE;
            sleep_cpu();
        }
    }

    /** Forces sleep mode to standby with given reason. Also sets the sleep mode if there is a change under disabled interrupts so that when AVR goes to sleep, it has the correct sleep mode set. 
     */
    static void requireSleepStandby(uint8_t reason) {
        cli();
        if (standbyRequired_ == 0)
            set_sleep_mode(SLEEP_MODE_STANDBY);
        standbyRequired_ |= reason;
        sei();
    }

    /** Clears the given reason for standby sleep mode. If there are no more reasons for standby, sets the sleep mode to power down. Sets the sleep mode if there is a change under disabled interrupts so that when AVR goes to sleep, it has the correct sleep mode set. 
     */
    static void allowSleepPowerDown(uint8_t reason) {
        cli();
        standbyRequired_ &= ~ reason;
        if (standbyRequired_ == 0)
            set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sei();
    }

    /** Triggered when the home button press down is detected. 
     
        If home button press is not in progress yet, set the home button press counter and update the device's mode accordingly. 
     */
    static void btnHomeDown() {
        if (btnHomeCounter_ == 0) {
            btnHomeCounter_ = BTN_HOME_LONG_PRESS_THRESHOLD;
            if (ts_.state.deviceMode() == DeviceMode::PowerOff)
                setDeviceMode(DeviceMode::Wakeup);
        }
    }

    /** Home button long press. 
     
        In a normal state, long press is immediate powerOff. In all other states, it is a singnal to enter normal mode. 
     */
    static void btnHomeLongPress() {
        switch (ts_.state.deviceMode()) {
            case DeviceMode::Normal:
                setDeviceMode(DeviceMode::PowerOff);
                break;
            case DeviceMode::Sleep:
                rpWakeup();
                setDeviceMode(DeviceMode::Normal);
                break;
            default:
                setDeviceMode(DeviceMode::Normal);
                break;
        }
    }

    /** Long button press cancellation only matters in the wakeup mode where we need to transition back to the power off mode and turn off the system ticks. 
     */
    static void btnHomeLongPressCancelled() {
        btnHomeCounter_ = 0; // disable the count
        if (ts_.state.deviceMode() == DeviceMode::Wakeup)
            setDeviceMode(DeviceMode::PowerOff);
    }

    static void setDeviceMode(DeviceMode mode) {
        ts_.state.setDeviceMode(mode);
        switch (mode) {
            case DeviceMode::Normal:
                initializePWM();
                ts_.state.setDebugMode(ts_.state.btnSel());
                // if we are in the debug mode, set brightness to 1/2 so that the display contents are visible even if not set by the app
                if (ts_.state.debugMode())
                    setBacklightPWM(128);
                startSystemTicks();
                power3v3(true);
                // enable I2C slave mode so that we can talk to the RP and enable interrupts for wakeup
                i2c::initializeSlave(AVR_I2C_ADDRESS);
                TWI0.SCTRLA |= TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm;
                break;
            case DeviceMode::Sleep:
                i2c::disableSlave();
                startSystemTicks();
                power3v3(true);
                break;
            case DeviceMode::PowerOff:
                i2c::disableSlave();
                // kill all these always, they will be picked up on the next second tick and reenabled if on dc power
                stopSystemTicks();
                power3v3(false);
                power5v(false);
                ts_.state.setDCPower(false);
                // disable potential ongoing btnHome counter 
                btnHomeCounter_ = 0;
                break;
            case DeviceMode::Wakeup:
                i2c::disableSlave();
                startSystemTicks();
                break;
        }
    }

    static bool systemTicksActive() { return (RTC.PITCTRLA & RTC_PERIOD_gm) == RTC_PERIOD_CYC64_gc; }

    static void startSystemTicks() {
        if (!systemTicksActive()) {
            // pull all buttons up
            gpio::setAsInputPullup(AVR_PIN_BTN_1);
            gpio::setAsInputPullup(AVR_PIN_BTN_2);
            gpio::setAsInputPullup(AVR_PIN_BTN_3);
            gpio::setAsInputPullup(AVR_PIN_BTN_4);
            // force all button groups to high 
            gpio::outputHigh(AVR_PIN_BTN_DPAD);
            gpio::outputHigh(AVR_PIN_BTN_ABXY);
            gpio::outputHigh(AVR_PIN_BTN_CTRL);
            // enable the RTC PIT at 1/512th second interval, ~ 2ms
            while (RTC.PITSTATUS & RTC_CTRLBUSY_bm);
            RTC.PITCTRLA = RTC_PERIOD_CYC64_gc | RTC_PITEN_bm;
        }
    }

    static void stopSystemTicks() {
        // enable the RTC PIT at 1 second interval
        while (RTC.PITSTATUS & RTC_CTRLBUSY_bm);
        RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc | RTC_PITEN_bm;
        // disable buttons by setting all lines high
        gpio::setAsInputPullup(AVR_PIN_BTN_1);
        gpio::setAsInputPullup(AVR_PIN_BTN_2);
        gpio::setAsInputPullup(AVR_PIN_BTN_3);
        gpio::setAsInputPullup(AVR_PIN_BTN_4);
        gpio::setAsInputPullup(AVR_PIN_BTN_DPAD);
        gpio::setAsInputPullup(AVR_PIN_BTN_ABXY);
        gpio::setAsInputPullup(AVR_PIN_BTN_CTRL);
    }

    /** Periodic ~500Hz system tick, i.e. every 2ms. In power down mode, the system tick is lowered to 1 second interval 
     */
    static void systemTick() __attribute__((always_inline)) {
        if (systemTicksActive()) {
            // if the 5V rail is on, do RGB tick roughly 60 times per second
            if (power5vActive() && (tick_ % 8) == 0)
                rgbTick_ = true;
            // do rumbler tick, if rumbler is on
            if (rumblerCurrent_.active())
                rumblerTick_ = true;
            // increase the system tick counter
            ++tick_;
            if (iMeasureCounter_ > 0)
                --iMeasureCounter_;
            // in case of the fast system tick, only do second every 512 ticks
            if (tick_ % 512 == 0)
                secondTick();
            // check if we are charging when DC power is available
            if (ts_.state.dcPower())
                ts_.state.setCharging(!gpio::read(AVR_PIN_CHARGING));
            // based on what tick we are, do what needs to be done             
            switch (tick_ % 4) {
                case 0: {
                    // do effects, which we do at ~ 100Hz (i.e. 4 ticks)
                    // TODO check the effects & stuff
                    // check the home button state
                    bool home = !gpio::read(AVR_PIN_BTN_HOME);
                    ts_.state.setBtnHome(!gpio::read(AVR_PIN_BTN_HOME));
                    // verify if we have a long press, or a long press has been cancelled
                    if (btnHomeCounter_ != 0) {
                        if (!home)
                            btnHomeLongPressCancelled();
                        else if (--btnHomeCounter_ == 0)
                            btnHomeLongPress();
                    }
                    // set the stage for measuring the DPAD buttons
                    gpio::low(AVR_PIN_BTN_DPAD);
                    // sample internal voltage reference using VDD for reference to determine VCC 
                    ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
                    ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;
                    break;
                }
                case 1: {
                    // check the DPAD buttons and set the stage for ABXY buttons
                    ts_.state.setDPadKeys(
                        !gpio::read(AVR_PIN_BTN_2), // left
                        !gpio::read(AVR_PIN_BTN_4), // right
                        !gpio::read(AVR_PIN_BTN_1), // up
                        !gpio::read(AVR_PIN_BTN_3) // down
                    );
                    gpio::high(AVR_PIN_BTN_DPAD);
                    gpio::low(AVR_PIN_BTN_ABXY);
                    // sample the battery voltage using VDD ref 
                    ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
                    ADC0.MUXPOS = gpio::getADC0muxpos(AVR_PIN_VBATT);
                    break;
                }
                case 2: {
                    // check the ABXY buttons and set the stage for CTRL buttons
                    ts_.state.setABXYKeys(
                        !gpio::read(AVR_PIN_BTN_2), // a
                        !gpio::read(AVR_PIN_BTN_1), // b
                        !gpio::read(AVR_PIN_BTN_4), // sel
                        !gpio::read(AVR_PIN_BTN_3) // start
                    );
                    // move the matrix to ctrl
                    gpio::high(AVR_PIN_BTN_ABXY);
                    gpio::low(AVR_PIN_BTN_CTRL);
                    // check if headphones are present if audio is enabled
                    if (ts_.state.audioEnabled()) {
#if (defined DEPRECATED_VERSION_2_2)                        
                        // TODO remove this when headphones are on ADC0
                        ADC1.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
                        ADC1.MUXPOS = gpio::getADC1muxpos(AVR_PIN_HEADPHONES);
                        ADC1.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc | ADC_RUNSTBY_bm;
                        ADC1.COMMAND = ADC_STCONV_bm;
                        requireSleepStandby(STANDBY_REQUIRED_ADC1);
                        return;
#else
                        // sample headphones
                        ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
                        ADC0.MUXPOS = gpio::getADC0muxpos(AVR_PIN_HEADPHONES);
#endif
                    // otherwise return immediately, i.e. don't start the ADC
                    } else {
                        return;
                    }
                }
                case 3: {
                    // check the CTRL buttons (volume up & down) and pull all matrix up
                    ts_.state.setVolumeKeys(
                        !gpio::read(AVR_PIN_BTN_2), // vol up
                        !gpio::read(AVR_PIN_BTN_3) // vol down
                    );
                    gpio::high(AVR_PIN_BTN_CTRL);
                    // if in debug mode, react to the volume keys by either resetting the RP, or entering its bootloader mode
                    if (ts_.state.debugMode()) {
                        if (ts_.state.btnVolUp())
                            rpReset();
                        else if (ts_.state.btnVolDown())
                            rpBootloader();
                    }
                    // sample temperature
                    ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_INTREF_gc | ADC_SAMPCAP_bm;
                    ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;
                    break;
                }
                default:
                    UNREACHABLE;
                    return;
            }
            // start the ADC conversion
            ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc | ADC_RUNSTBY_bm;
            ADC0.COMMAND = ADC_STCONV_bm;
            requireSleepStandby(STANDBY_REQUIRED_ADC0);
        } else {
            secondTick();
            // sample internal voltage reference using VDD for reference to determine VCC 
            ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
            ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;
            // start the ADC conversion
            ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc | ADC_RUNSTBY_bm;
            ADC0.COMMAND = ADC_STCONV_bm;
            requireSleepStandby(STANDBY_REQUIRED_ADC0);
        }
    }

    /** A second tick. 
     
        MODE  VCC   VBATT TMP 
        TICK  HOME  IDEV SEC
        DC CH AE HP AL DBG LED R
        Se S A B L R U D
     */
    static void secondTick() __attribute__((always_inline)) {
#if (defined RCKID_AVR_DEBUG_OLED_DISPLAY)
        if (power3v3Active()) {
            oled_.write(0, 0, static_cast<uint8_t>(ts_.state.deviceMode()), ' ');
            oled_.write(32, 0, ts_.state.vcc(), ' ');
            oled_.write(64, 0, ts_.state.vBatt(), ' ');
            oled_.write(96, 0, ts_.state.temp(), ' ');
            oled_.write(0, 1, tick_, ' ');
            oled_.write(32, 1, btnHomeCounter_, ' ');
            oled_.write(64, 1, current_, ' ');
            oled_.write(96, 1, ts_.time.seconds(), ' ');
            oled_.write(0, 2, ts_.state.dcPower() ? "DC" : "  ");
            oled_.write(16, 2, ts_.state.charging() ? "CH" : "  ");
            oled_.write(32, 2, ts_.state.audioEnabled() ? "AE" : "  ");
            oled_.write(48, 2, ts_.state.headphones() ? "HP" : "  ");
            oled_.write(64, 2, ts_.state.alarm() ? "AL" : "  ");
            oled_.write(80, 2, ts_.state.debugMode() ? "DBG" : "   ");
            oled_.write(96, 2, power5vActive() ? "LED" : " ");
            oled_.write(112, 2, rumblerCurrent_.active() ? "R" : " ");
            oled_.write(0, 3, ts_.state.btnSel() ? "Se" : "  ");
            oled_.write(10, 3, ts_.state.btnStart() ? "St" : "  ");
            oled_.write(20, 3, ts_.state.btnA() ? "A" : " ");
            oled_.write(30, 3, ts_.state.btnB() ? "B" : " ");
        }
#endif
        if (power5vActive())
            rgbSecondTick();
        // keep time & uptime
        ++ts_.uptime;
        ts_.time.secondTick();
    }

    /** ADC0 is done measuring. 
     
        Depending on the muxpos, process either the voltage, battery voltage, charging current, or temperature. 
     */
    static void adcDone() __attribute__((always_inline)) {
        uint16_t value = ADC0.RES / 32;
        uint8_t muxpos = ADC0.MUXPOS;
        switch (muxpos) {
            case ADC_MUXPOS_INTREF_gc:
                value = 110 * 512 / value;
                value = value * 2;
                ts_.state.setVcc(value);
                // check power events
                if ((ts_.state.deviceMode() != DeviceMode::PowerOff) && (value < VCC_CRITICAL_THRESHOLD))
                    criticalBattery();
                else if (value > VCC_DC_POWER_THRESHOLD)
                    dcPowerPlugged();
                else
                    dcPowerUnplugged();
                break;
            case gpio::getADC0muxpos(AVR_PIN_VBATT):
                // convert the battery reading to voltage. The battery reading is relative to vcc, which we already have
                // VBATT = VCC * VBATT / 255 
                // but 500 * 255 is too high to fit in uint16, so we divide VCC by 2 and then divide by 128 instead
                value >>= 2; // go for 8bit precision, which should be enough
                value = (ts_.state.vcc() / 2 * value)  / 128; 
                ts_.state.setVBatt(value);
#ifdef RCKID_HAS_LIPO_CHARGER
                if (ts_.state.charging() && value >= RCKID_VBATT_CHARGE_CUTOFF_VOLTAGE)
                    disableCharging();
#endif
                break;
            case gpio::getADC0muxpos(AVR_PIN_HEADPHONES):
                ts_.state.setHeadphones(value < HEADPHONES_DETECTION_THRESHOLD);
                break;
            case ADC_MUXPOS_TEMPSENSE_gc: {
                int8_t sigrow_offset = SIGROW.TEMPSENSE1; 
                uint8_t sigrow_gain = SIGROW.TEMPSENSE0;
                int32_t t = value - sigrow_offset; // Result might overflow 16 bit variable (10bit+8bit)
                t *= sigrow_gain;
                // temp is now in kelvin range, to convert to celsius, remove -273.15 (x256)
                t -= 69926;
                // and now loose precision to 0.5C (x10, i.e. -15 = -1.5C)
                value = (t >>= 7) * 5;
                ts_.state.setTemp(value);
                // check temp too high for charging
#ifdef RCKID_HAS_LIPO_CHARGER
                if (ts_.state.charging() && value >= RCKID_VBATT_CUTOFF_TEMPERATURE)
                    disableCharging();
#endif
                break;
            }
            default:
                UNREACHABLE;
                break;
        }
        ADC0.CTRLA = 0; // turn ADC off, will be enabled by next tick and set sleep mode to power down 
        allowSleepPowerDown(STANDBY_REQUIRED_ADC0);
    }

#if (defined DEPRECATED_VERSION_2_2)
    /** ADC1 ready IRQ. Triggered for headphones only. 
     
        TODO Delete this when headphones are on ADC0 in never revision
    */
    static void adc1Done() {
        uint16_t value = ADC1.RES / 32;
        uint8_t muxpos = ADC1.MUXPOS;
        switch (muxpos) {
            case gpio::getADC1muxpos(AVR_PIN_HEADPHONES):
                ts_.state.setHeadphones(value < HEADPHONES_DETECTION_THRESHOLD);
                break;
            default:
                UNREACHABLE;
                break;
        }
        ADC1.CTRLA = 0; // turn ADC off, will be enabled by next tick and set sleep mode to power down 
        allowSleepPowerDown(STANDBY_REQUIRED_ADC1);
    }
#endif

    /** Turns the 3V3 power rail for RP2040, cartridge, sensors and audio on or off. 
     */
    static void power3v3(bool state) {
        using namespace platform;
        if (state) {
            gpio::outputHigh(AVR_PIN_3V3_ON);
            // allow some time for the voltages to stabilize
            cpu::delayMs(50);
#if (RCKID_INA219_I2C_ADDRESS != 0)            
            i2c::initializeMaster();
            ina_.initialize_32V_4A(INA219::Resolution::bit12_samples4, 10);
#endif
            iMeasureCounter_ = 0; 
        } else {
            gpio::outputFloat(AVR_PIN_3V3_ON);
#if (!defined RCKID_AVR_DEBUG_OLED_DISPLAY)
            // we need I2C master for the oled debug display
            i2c::disableMaster();
#endif
        }
    }

    static bool power3v3Active() {
        return gpio::read(AVR_PIN_3V3_ON);
    }

    /** Turns the 5V power rail for the neopixels on or off. 
     */
    static void power5v(bool state) {
        if (state) {
            gpio::outputHigh(AVR_PIN_5V_ON);
            gpio::setAsOutput(AVR_PIN_RGB);
            cpu::delayMs(50);
        } else {
            gpio::outputFloat(AVR_PIN_5V_ON);
            gpio::outputFloat(AVR_PIN_RGB);
        }
    }

    static bool power5vActive() {
        return gpio::read(AVR_PIN_5V_ON);
    }
 
    //@}

    /** \name Power and control events
     */
    //@{

    static void enableAudio(bool value) {
        ts_.state.setAudioEnabled(value);
        if (value) {
            gpio::outputFloat(AVR_PIN_HEADPHONES);
        } else {
            gpio::outputLow(AVR_PIN_HEADPHONES);
        }
    }

    static void rpReset() {
        power3v3(false);
        power5v(true);
        rgbs_.fill(platform::Color::Blue());
        rgbs_.update();
        cpu::delayMs(1000);
        power3v3(true);
        power5v(false);
        setBacklightPWM(128);
        // leave the RGBs on, the RP2040 should deal with them when it restarts
    }

    static void rpBootloader() {
        power3v3(false);
        power5v(true);
        rgbs_.fill(platform::Color::Green());
        rgbs_.update();
        gpio::outputLow(AVR_PIN_QSPI_SS);
        cpu::delayMs(500);
        power3v3(true);
        power5v(false);
        setBacklightPWM(128);
        cpu::delayMs(300);
        gpio::outputFloat(AVR_PIN_QSPI_SS);
        // keep the green lights on, the RP2040 app that has just been uploaded should turn them off
    }

    /** Signals to the RP2040 to wake up */
    static void rpWakeup() {
        using namespace platform;
        // send the wakeup command to the RP2040 via I2C 
        i2c::masterTransmit(RP_I2C_ADDRESS, nullptr, 0, nullptr, 0);
    }

    /** Triggered when DC power is detected. 

        DC power is detected by VCC being above li-ion fully charged battery threshold (VCC_DC_POWER_THRESHOLD). When DC power is enabled, the  li-po charger must be monitored, so we need to enable the ticks if in powerDown mode and start monitoring the charging current, battery voltage and own temperature. 
     */
    static void dcPowerPlugged() {
        // don't do anything if already plugged
        if (ts_.state.dcPower())
            return;
        // otherwise change state and get ready for charge monitoring
        ts_.state.setDCPower(true);
    #ifdef RCKID_HAS_LIPO_CHARGER
        ts_.state.setCharging(true); // we assume this
        if (!systemTicksActive())
            startSystemTicks();
#if (defined DEPRECATED_VERSION_2_2)
        // ensure the charge enable pin is configured as input so that we can read it to determine charging current
        gpio::outputFloat(AVR_PIN_CHARGE_EN);
#else
        // set output low to enable charging
        gpio::outputLow(AVR_PIN_CHARGE_EN);
#endif
    #endif
    }

    static void dcPowerUnplugged() {
        // don't do anything if already unplugged
        if (!ts_.state.dcPower())
            return;
        // otherwise change state and disable the charger monitoring circuitry
        ts_.state.setDCPower(false);
        ts_.state.setCharging(false);
#ifdef RCKID_HAS_LIPO_CHARGER
        if (ts_.state.deviceMode() == DeviceMode::PowerOff)
            stopSystemTicks();
        ts_.state.setCharging(false);
        // disable charging
        gpio::outputFloat(AVR_PIN_CHARGE_EN);
#endif
    }

    /** Goes to the power off mode immediately while flashing some red colors. 
     
        Since this is called from the ADC0 done method, it will fire even when in the wakeup phase we detect a too low voltage. 
     */
    static void criticalBattery() {
        if (++vccMeasurementCounter_ >= RCKID_VCC_CRITICAL_REQUIRED_SAMPLES) {
            vccMeasurementCounter_ = 0;
            // turn off the 3v3 rail first
            power3v3(false);
            // flash the LEDs red three times
            power5v(true);
            for (int i = 0; i < 3; ++i) {
                rgbs_.fill(platform::Color::Red().withBrightness(128));
                rgbs_.update();
                cpu::delayMs(200);
                rgbs_.fill(platform::Color::Black());
                rgbs_.update();
                cpu::delayMs(200);
                cpu::wdtReset(); 
            }
            setDeviceMode(DeviceMode::PowerOff);
        }
    }

    /** Programmatically disables the charger. 
     
        As per the charger's datasheet, section 5.2.2, charging can be terminated by applying logic 1 to the charger's prog pin. 
     */
    static void disableCharging() {
        // TODO enable this in the final version when the charging pin is directly connected to RPROG on the charger via resistor, not to ground as it is now
#if (defined DEPRECATED_VERSION_2_2)
        gpio::outputHigh(AVR_PIN_CHARGE_EN);
#else
        gpio::outputFloat(AVR_PIN_CHARGE_EN);
#endif
        // TODO this is serious error and should be reported somewhere
    }
    //@}

    /** \name PWM (rumbler and backlight)

        The PWM signals used for backlight and rumbler control are generated by the TCB0 and TCB1 respectively.

        Backlight is pulled low externally, setting the pin to 1 make the backlight work, hence the value is unchanged.  
     */
    //@{

    static void initializePWM() {
        // do not leak voltage and turn the pins as inputs
        static_assert(AVR_PIN_PWM_BACKLIGHT == A5); // TCB0 WO
        gpio::outputFloat(AVR_PIN_PWM_BACKLIGHT);
        TCB0.CTRLA = 0;
        TCB0.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
        TCB0.CCMPL = 255;
        TCB0.CCMPH = 0; 
        static_assert(AVR_PIN_PWM_RUMBLER == A3); //TCB1 WO
        gpio::outputFloat(AVR_PIN_PWM_RUMBLER);
        TCB1.CTRLA = 0;
        TCB1.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
        TCB1.CCMPL = 255;
        TCB1.CCMPH = 0; 

     }

    static void setBacklightPWM(uint8_t value) {
        if (value == 0) {
            TCB0.CTRLA = 0;
            gpio::outputFloat(AVR_PIN_PWM_BACKLIGHT);
            allowSleepPowerDown(STANDBY_REQUIRED_BRIGHTNESS);
        } else if (value == 255) {
            TCB0.CTRLA = 0;
            gpio::outputHigh(AVR_PIN_PWM_BACKLIGHT);
            allowSleepPowerDown(STANDBY_REQUIRED_BRIGHTNESS);
        } else {
            gpio::outputLow(AVR_PIN_PWM_BACKLIGHT);
            TCB0.CCMPH = value;
            TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm | TCB_RUNSTDBY_bm;
            requireSleepStandby(STANDBY_REQUIRED_BRIGHTNESS);
        }
    }

    static void setRumblerPWM(uint8_t value) {
        if (value == 0) {
            TCB1.CTRLA = 0;
            gpio::outputFloat(AVR_PIN_PWM_RUMBLER);
            allowSleepPowerDown(STANDBY_REQUIRED_RUMBLER);
        } else if (value == 255) {
            TCB1.CTRLA = 0;
            gpio::outputHigh(AVR_PIN_PWM_RUMBLER);
            allowSleepPowerDown(STANDBY_REQUIRED_RUMBLER);
        } else {
            gpio::outputLow(AVR_PIN_PWM_RUMBLER);
            TCB1.CCMPH = 255 - value;
            TCB1.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm | TCB_RUNSTDBY_bm;
            requireSleepStandby(STANDBY_REQUIRED_RUMBLER);
        }
    }

    static void rumblerTick() {
        rumblerTick_ = false;
        if (rumblerCurrent_.strength != 0) {
            if (rumblerCurrent_.timeOn > 0) {
                if (--rumblerCurrent_.timeOn > 0) 
                    return;
                else 
                    setRumblerPWM(0);
            }
            if (rumblerCurrent_.timeOff > 0) {
                if (--rumblerCurrent_.timeOff > 0)
                    return;
            }
            if (rumblerCurrent_.cycles != 0) {
                --rumblerEffect_.cycles;
                rumblerCurrent_ = rumblerEffect_;
                if (rumblerCurrent_.timeOn > 0)
                    setRumblerPWM(rumblerCurrent_.strength);
                return;
            }
            // we are done
            rumblerEffect_ = RumblerEffect::Off();
            rumblerCurrent_ = RumblerEffect::Off();
        }
    }
    //@}

    /** \name RGB LEDs

     */
    //@{

    static void rgbTick() {
        rgbTick_ = false;
        // for all LEDs, move them towards their target at speed given by their effect
        bool turnOff = true;
        for (int i = 0; i < 6; ++i) {
            bool done = ! rgbs_[i].moveTowards(rgbsTarget_[i], rgbEffects_[i].speed);
            // if the current transition is done, make next effect transition
            if (done) {
                rgbsTarget_[i] = rgbEffects_[i].nextColor(rgbsTarget_[i]);
                if (rgbEffects_[i].active())
                    turnOff = false;
            } else {
                turnOff = false;
            }
        }
        // if all the LEDs are off, turn the 5V rail off to save power, otherwise update the LEDs
        if (turnOff)
            power5v(false);
        else
            rgbs_.update(true);
    }

    static void rgbSecondTick() {
        for (int i = 0; i < 6; ++i) {
            // see if the effect should end
            if (rgbEffects_[i].duration > 0) {
                if (--rgbEffects_[i].duration == 0) {
                    rgbEffects_[i].turnOff();
                }
            }
        }
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
    static inline void i2cSlaveIRQHandler() __attribute__((always_inline)) {
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
        // sending data to accepting master simply starts sending the ts_.state buffer. 
        if ((status & I2C_DATA_MASK) == I2C_DATA_TX) {
            TWI0.SDATA = ((uint8_t*) & ts_.state)[i2cTxIdx_];
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
            ++i2cTxIdx_;
            // TODO send nack when done sending all state
        // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
        } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
            ts_.buffer[i2cRxIdx_++] = TWI0.SDATA;
            TWI0.SCTRLB = (i2cRxIdx_ == sizeof(ts_.buffer)) ? TWI_SCMD_COMPTRANS_gc : TWI_SCMD_RESPONSE_gc;
        // master requests slave to write data, reset the sent bytes counter, initialize the actual read address from the read start and reset the IRQ
        } else if ((status & I2C_START_MASK) == I2C_START_TX) {
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc + TWI_SCMD_RESPONSE_gc;
        // master requests to write data itself. ACK if there is no pending I2C message, NACK otherwise. The buffer is reset to 
        } else if ((status & I2C_START_MASK) == I2C_START_RX) {
            TWI0.SCTRLB = (! i2cCommandReady_) ? TWI_SCMD_RESPONSE_gc : TWI_ACKACT_NACK_gc;
        // sending finished, reset the tx address and when in recording mode determine if more data is available
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_TX) {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            i2cTxIdx_ = 0;
            // since we are done sending, time to check the INA219 for current measurements to avoid clashes on the bus
            iMeasureCounter_ = 0;
        // receiving finished, inform main loop we have message waiting if we have received at laast one byte (0 bytes received is just I2C ping)
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_RX) {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            if (i2cRxIdx_ > 0)
                i2cCommandReady_ = true;
        } else {
            // error - a state we do not know how to handle
        }
    }

    static void processI2CCommand() {
        switch (ts_.buffer[0]) {
            case cmd::Nop::ID:
                break;
            case cmd::PowerOff::ID:
                setDeviceMode(DeviceMode::PowerOff);
                break;
            case cmd::Sleep::ID:
                setDeviceMode(DeviceMode::Sleep);
                break;
            case cmd::ResetRP::ID:
                rpReset();
                break;
            case cmd::ResetAVR::ID:
                cpu::reset();
                // unreachable here
            case cmd::BootloaderRP::ID:
                rpBootloader(); 
                break;
            case cmd::BootloaderAVR::ID:
                // TODO
                break;
            case cmd::DebugModeOn::ID:
                NO_ISR(ts_.state.setDebugMode(true));
                break;
            case cmd::DebugModeOff::ID:
                NO_ISR(ts_.state.setDebugMode(false));
                break;
            case cmd::AudioEnabled::ID:
                NO_ISR(enableAudio(true));
                break;
            case cmd::AudioDisabled::ID:
                NO_ISR(enableAudio(false));
                break;
            case cmd::SetBrightness::ID: {
                uint8_t value = cmd::SetBrightness::fromBuffer(ts_.buffer).value;
                setBacklightPWM(value);
                NO_ISR(ts_.state.setBrightness(value));
                break;
            }
            case cmd::SetTime::ID: {
                TinyDate t = cmd::SetTime::fromBuffer(ts_.buffer).value;
                NO_ISR(ts_.time = t);
                break;
            }
            case cmd::DisplayRead::ID: {
                gpio::outputLow(AVR_PIN_DISP_RDX);
                break;
            }
            case cmd::DisplayWrite::ID: {
                gpio::outputFloat(AVR_PIN_DISP_RDX);
                break;
            }
            case cmd::ResetAVRError::ID: {
                ts_.error = AVR_NO_ERROR;
                break;
            }
            case cmd::Rumbler::ID: {
                auto & c = cmd::Rumbler::fromBuffer(ts_.buffer);
                if (c.effect.cycles > 0) {
                    rumblerEffect_ = c.effect;
                    --rumblerEffect_.cycles;
                    rumblerCurrent_ = rumblerEffect_;
                } else {
                    rumblerEffect_ = RumblerEffect::Off();
                    rumblerCurrent_ = RumblerEffect::Off();
                }
                break;
            }
            case cmd::RGBOff::ID: {
                for (int i = 0; i < 6; ++i)
                    rgbEffects_[i] = RGBEffect::Off();
                break;
            }
            case cmd::SetRGBEffect::ID: {
                auto & c = cmd::SetRGBEffect::fromBuffer(ts_.buffer);
                rgbEffects_[c.index] = c.effect;
                power5v(true);
                break;
            }
            case cmd::SetRGBEffects::ID: {
                auto & c = cmd::SetRGBEffects::fromBuffer(ts_.buffer);
                rgbEffects_[0] = c.b;
                rgbEffects_[1] = c.a;
                rgbEffects_[3] = c.dpad;
                rgbEffects_[4] = c.sel;
                rgbEffects_[5] = c.start;
                power5v(true);
                break;
            }
            default:
                // unknown command
                break;
        }
        // mark ready to get new command and reset the rx buffer
        i2cRxIdx_ = 0;
        i2cCommandReady_ = false;
    }

    //@}

}; // class RCKid

ISR(RTC_PIT_vect) {
    BEGIN_ACTIVE_MODE;
    RTC.PITINTFLAGS = RTC_PI_bm; // clear the interrupt
    RCKid::systemTick();
}

ISR(TWI0_TWIS_vect) {
    BEGIN_ACTIVE_MODE;
    RCKid::i2cSlaveIRQHandler();
}

ISR(ADC0_RESRDY_vect) {
   BEGIN_ACTIVE_MODE;
   ADC0.INTFLAGS = ADC_RESRDY_bm;
   RCKid::adcDone(); 
}

#if (defined DEPRECATED_VERSION_2_2)
// TODO delete when headphones are on ADC0 in never revision
ISR(ADC1_RESRDY_vect) {
   BEGIN_ACTIVE_MODE;
   ADC1.INTFLAGS = ADC_RESRDY_bm;
   RCKid::adc1Done(); 
}
#endif

ISR(PORTB_PORT_vect) {
    BEGIN_ACTIVE_MODE;
    static_assert(AVR_PIN_BTN_HOME == B2);
    VPORTB.INTFLAGS = (1 << 2);
    RCKid::btnHomeDown();
}


int main() {
    RCKid::initialize();
    RCKid::loop();
} 
