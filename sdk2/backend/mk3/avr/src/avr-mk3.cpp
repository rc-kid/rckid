/** AVR Firmware for RCKid
 
    This is the firmware for ATTiny3217 in RCKid mkIII, specifically SDK version 1.0 and up. The main difference from the previous mkIII versions is simpler execution model with fewer commands and firmware logic as per the SDK 1.0 refactoring.


 */
#include <avr/sleep.h>
#include <avr/delay.h>
#include <avr/interrupt.h>

#include "avr-commands.h"

/** \name Debugging support. 
 
    To ease firmware development, the AVR chip supports serial port (only TX) as alternate function of the AVR_INT pin. The functionality has to be enabled in the macro below as it is offby default. 
 */
//@{

#define AVR_INT_IS_SERIAL_TX 0

inline Writer debugWrite() {
    return Writer(serial::write);
}
//@}


#define ASSERT(...) if (!(__VA_ARGS__)) { debugWrite() << "ASSERT " << __LINE__ << "\r\n"; }
#define UNREACHABLE do { debugWrite() << "Unreachable: " << __LINE__; while (true) {} } while (false)
#define UNIMPLEMENTED do { debugWrite() << "Unimplemented: " << __LINE__; while (true) {} } while (false)

#if AVR_INT_IS_SERIAL_TX
    #define LOG(...) debugWrite() << __VA_ARGS__ << "\r\n";
#else
    #define LOG(...)
#endif

using namespace rckid;

class RCKid {
public:

    static void initialize() {

    }

    static void loop() {

    }

    /** \name Power Management
     
        The power is controlled via the power mode register, which when zero signals device off, in which case the AVR goes to deep sleep only to wake up when the accel or power interrupts are triggered, or the home button is pressed. When non-zero the register acts as a bitmask of possible reasons for the AVR to stay awake. This could be either the user-facing power on mode, when also the RP2350 is powered on, or can be when the device is charging (we need to indicate the charging status using the LEDs), or when waking up (need to calculate the power button press duration).
        
        Any time the power mode is *not* zero, the AVR is expected to be awake and running the main loop, including the ticks that time the executions & i/o (see System ticks and clocks below). 
     */
    //@{
    static constexpr uint8_t POWER_MODE_DC = 1;
    static constexpr uint8_t POWER_MODE_CHARGING = 2;
    static constexpr uint8_t POWER_MODE_WAKEUP = 4;
    static constexpr uint8_t POWER_MODE_ON = 8;
    static inline uint8_t powerMode_ = 0;

    // flag indicating critical battery levels (reset by connecting charger)
    static inline bool criticalBattery_ = false;

    // timeout the RP2350 has to power itself off before AVR cuts power forcefully
    static inline uint16_t powerOffTimeout_ = 0;

    static bool isPowerOff() { return powerMode_ == 0; }
    static bool isPowerModeDC() { return powerMode_ & POWER_MODE_DC; }
    static bool isPowerModeCharging() { return powerMode_ & POWER_MODE_CHARGING; }
    static bool isPowerModeWakeup() { return powerMode_ & POWER_MODE_WAKEUP; }
    static bool isPowerModeOn() { return powerMode_ & POWER_MODE_ON; }

    static void setPowerMode(uint8_t mode) {
        if (powerMode_ & mode)
            return;
        // if we are transitionioning from complete off, start system ticks and set sleep mode to standby
        if (powerMode_ == 0) {
            LOG("systick start, sleep standby");
            //set_sleep_mode(SLEEP_MODE_STANDBY);
            set_sleep_mode(SLEEP_MODE_IDLE);
            enableSystemTicks(true);
        }
        powerMode_ |= mode;
        switch (mode) {
            // we never enter power on directly from off, so no need to do anything here
            case POWER_MODE_ON:
                break;
            
            // clear the critical battery flag when DC power is connected, flash RGB LEDs if the device is not on (when on, the RGBs are controlled by the applications). No need to update state (the VCC value alone determines the DC mode as it did when calling this function)
            case POWER_MODE_DC:
                criticalBattery_ = false;
                if (! (powerMode_ & POWER_MODE_ON)) 
                    //setNotification(RGBEffect::Breathe(platform::Color::Green().withBrightness(RCKID_RGB_BRIGHTNESS), RCKID_RGB_NOTIFICATION_SPEED));
                break;
            // update the state to indicate charging, flash the RGBs if not powered on
            case POWER_MODE_CHARGING:
                ts_.state.setCharging(true);
                if (! (powerMode_ & POWER_MODE_ON)) 
                    //setNotification(RGBEffect::Breathe(platform::Color::Orange().withBrightness(RCKID_RGB_BRIGHTNESS), RCKID_RGB_NOTIFICATION_SPEED));
                break;
            default:
                // no action
                break;
        }
    }

    static void clearPowerMode(uint8_t mode) {
        if (!(powerMode_ & mode))
            return;
        powerMode_ &= ~mode;
        switch (mode) {
            // when leaving power on mode, clear debug mode (will be determined on next power on). If we are  charging, or DC enabled, set the notifications accordingly.
            case POWER_MODE_ON:
                ts_.state.setDebugMode(false);
                // if we are turning off, set notification according to other power modes
                /*
                if (powerMode_ & POWER_MODE_CHARGING)
                    setNotification(RGBEffect::Breathe(platform::Color::Orange().withBrightness(RCKID_RGB_BRIGHTNESS), RCKID_RGB_NOTIFICATION_SPEED));
                else if (powerMode_ & POWER_MODE_DC)
                    setNotification(RGBEffect::Breathe(platform::Color::Green().withBrightness(RCKID_RGB_BRIGHTNESS), RCKID_RGB_NOTIFICATION_SPEED));
                */
                // reset the poweroff timeout
                powerOffTimeout_ = 0;
                break;
            // when leaving DC mode, no need to do anything special
            case POWER_MODE_DC:
                // TODO verify this is not needed
                //if (! (powerMode_ & POWER_MODE_ON)) 
                //    setNotification(RGBEffect::Off());
                break;
            // when leaving charging mode, update the chatging status. If we the DC mode is enabled, we should transition the notification to it (charging notification takes precedence over DC when both enabled)
            case POWER_MODE_CHARGING:
                ts_.state.setCharging(false);
                //if (! (powerMode_ & POWER_MODE_ON)) 
                //    setNotification(RGBEffect::Breathe(platform::Color::Green().withBrightness(RCKID_RGB_BRIGHTNESS), RCKID_RGB_NOTIFICATION_SPEED));
                break;
            // no action otherwise
            default:
                break;  
        }
        // if we have transitioned to complete off, stop system ticks and set sleep mode to power down
        if (powerMode_ == 0) {
            LOG("systick stop, sleep pwrdown");
            // setNotification(RGBEffect::Off());
            enableSystemTicks(false);
            set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        }

    }


    //@}

    /** \name System ticks and clocks
     */
    //@{

    static inline volatile bool systemTick_ = false;
    static inline volatile bool secondTick_ = false;

    static inline uint8_t tickCounter_ = 0;

    static void enableSystemTicks(bool value = true) {
        if (value) {
            if (RTC.CTRLA & RTC_RTCEN_bm)
                return;
            LOG("systick - enable RTC");
            // initialize buttons matrix for CTRL row
            tickCounter_ = 0;
            initializeButtons();
            // initialize the RTC
            while (RTC.STATUS & RTC_CTRLABUSY_bm);
            RTC.CTRLA = 0;
            while (RTC.STATUS & RTC_PERBUSY_bm);
            RTC.PER = 164; // for 5 ms (32768 / 200)
            while (RTC.STATUS & RTC_CNTBUSY_bm);
            RTC.CNT = 0;
            RTC.INTCTRL = RTC_OVF_bm;
            while (RTC.STATUS & RTC_CTRLABUSY_bm);
            RTC.CTRLA = RTC_RUNSTDBY_bm | RTC_RTCEN_bm;
            // whenever system ticks are started, register the IRQ interrupts (rising edge of PWR_INT and ACCEL_INT)
            GPIO_PIN_PINCTRL(AVR_PIN_ACCEL_INT) |= PORT_ISC_BOTHEDGES_gc;
            // do not use interrupt on home button (we handle it in the loop due to matrix row rotation)
            GPIO_PIN_PINCTRL(AVR_PIN_BTN_1) &= ~PORT_ISC_gm;
            // do not use interrupt on charging pin either
            GPIO_PIN_PINCTRL(AVR_PIN_CHARGING) &= ~PORT_ISC_gm;
        } else {
            // no harm disabling multiple times
            LOG("systick - disable RTC");
            while (RTC.STATUS & RTC_CTRLABUSY_bm);
            RTC.CTRLA = 0;
            // initialize buttons so that we are always reading control row when system ticks not used
            tickCounter_ = 0;
            initializeButtons();
            // allow some time for propagation
            cpu::delayMs(5);
            // enable interrupts for the power down mode where only pin change is available
            GPIO_PIN_PINCTRL(AVR_PIN_ACCEL_INT) |= PORT_ISC_BOTHEDGES_gc;
            GPIO_PIN_PINCTRL(AVR_PIN_BTN_1) |= PORT_ISC_BOTHEDGES_gc;
            GPIO_PIN_PINCTRL(AVR_PIN_CHARGING) |= PORT_ISC_BOTHEDGES_gc;
            // disable any pending system tick
            systemTick_ = false;
            // since we are powering off, make sure the LEDs are off as well so that we do not leak
            rgbOn(false);
        }
    }

    /** Performs system tick if the time is right and returns true, false otherwise. As part of the system tick also checks the button states.
     */
    static bool systemTick() {
        // do nothing if system tick interrupt is not requested, clear the flag otherwise
        if (! systemTick_)
            return false;
        systemTick_ = false;
        // increment the system tick counter and perform the tick actions
        switch (tickCounter_++) {
            case 0:
                readControlGroup();
                break;
            case 1:
                readABXYGroup();
                break;
            default:
                tickCounter_ = 0;
                readDPadGroup();
                break;
        }
        return true;
    }

    static void secondTick() {
        if (! secondTick_)
            return;
        secondTick_ = false;
        ++ts_.uptime;
        ts_.time.inc();
        // read either battery voltage, or temperature every other second using ADC0 (but only do so when in power on mode - otherwise there is no-one to use the measurements)
        if (isPowerModeOn()) {
            if ((ts_.uptime & 1) == 0) {
                static_assert(AVR_PIN_VCC_SENSE == gpio::A2);
                startADC(ADC_MUXPOS_AIN2_gc);
            } else {
                startADC(ADC_MUXPOS_TEMPSENSE_gc);
            }
        }
        // see if we should wake up the device via the wakeup irq
        // TODO
    }
    //@}




    /** \name I2C Communication
     
        - maybe have transferrable state that returns state, date time, all of user bytes
        - reset at the end of tx to state_ 
        - then have command sthat do things 
     */
    //@{

    static inline volatile bool irq_ = false;
    static inline volatile bool i2cCommandReady_ = false;
    static inline uint8_t const * i2cRxAddr_ = nullptr;
    static inline uint8_t i2cBytes_ = 0;
    static inline uint8_t i2cBuffer_[16];

    static void initializeComms() {
        // make sure we'll start reading the transferrable state first
        i2cRxAddr_ = reinterpret_cast<uint8_t *>(& ts_);
    }

    static void setIrq() {
        // TODO do we want to do anything here? 
    }

    static void i2cSlaveIRQHandler() {
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
            TWI0.SDATA = i2cRxAddr_[i2cBytes_++];
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
        // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
        } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
            i2cBuffer_[i2cBytes_++] = TWI0.SDATA;
            TWI0.SCTRLB = (i2cBytes_ == sizeof(i2cBuffer_)) ? TWI_SCMD_COMPTRANS_gc : TWI_SCMD_RESPONSE_gc;
        // master requests slave to write data, reset the sent bytes counter, initialize the actual read address from the read start and reset the IRQ
        } else if ((status & I2C_START_MASK) == I2C_START_TX) {
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc + TWI_SCMD_RESPONSE_gc;
            i2cBytes_ = 0;
        // master requests to write data itself. ACK if there is no pending I2C message, NACK otherwise. The buffer is reset to 
        } else if ((status & I2C_START_MASK) == I2C_START_RX) {
            TWI0.SCTRLB = (! i2cCommandReady_) ? TWI_SCMD_RESPONSE_gc : TWI_ACKACT_NACK_gc;
        // sending finished, reset the tx address and when in recording mode determine if more data is available
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_TX) {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            i2cRxAddr_ = reinterpret_cast<uint8_t *>(& ts_);
        // receiving finished, inform main loop we have message waiting if we have received at laast one byte (0 bytes received is just I2C ping)
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_RX) {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            if (i2cBytes_ > 0)
                i2cCommandReady_ = true;
        } else {
            // error - a state we do not know how to handle
        }
    }

    static void processI2CCommand() {
        if (!i2cCommandReady_)
            return;
        // process the commands
        LOG("Cmd: " << i2cBuffer_[0]);
        switch (i2cBuffer_[0]) {
            case cmd::Nop::ID:
                i2cRxAddr_ = reinterpret_cast<uint8_t *>(& ts_);
                break;
            case cmd::GetVersion::ID:
                i2cRxAddr_ = reinterpret_cast<uint8_t const *>(& ts_.version);
                break;
            case cmd::GetTime::ID:
                i2cRxAddr_ = reinterpret_cast<uint8_t *>(& ts_.time);
                break;
            case cmd::SetTime::ID:
                ts_.time = cmd::SetTime::fromBuffer(i2cBuffer_).value;
                break;
            case cmd::ReadStorage::ID:
                i2cRxAddr_ = ts_.storage + cmd::ReadStorage::fromBuffer(i2cBuffer_).offset;
                break;
            case cmd::WriteStorage::ID: {
                auto & c = cmd::WriteStorage::fromBuffer(i2cBuffer_);
                memcpy(ts_.storage + c.offset, c.data, i2cBytes_ - sizeof(c.offset) - 1);
                break;
            }
            default:
                // unknown command, ignore for now
                LOG("Unknown cmd: " << i2cBuffer_[0]);
                break;
        }
        // and reset the command state so that we can read more commands 
        NO_ISR(
            i2cBytes_ = 0;
            i2cCommandReady_ = false;
        );
    }
    //@}


    /** \name Buttons 

        The button matrix is 3 groups of 4 buttons. Namely the BTN_CTRL group selects the Home button and volume up & down side buttons, the BTN_ABXY group selects the A, B and Select & Start buttons and the BTN_DPAD group selects the dpad. 

        When idle, all button pins are in input mode, pulled up. Button groups all output logical high. When a button group is selected, only that group's pin goes to logical 0. We then read the four buttons and reading 0 means the button is pressed (i.e. connected to the button group). The diodes from buttons to group pins ensure that no ghosting happens (i.e. low from one button would through different group enter other button here.

        All button pins are digital and are read as part of system tick. 
     */
    //@{

    // counter for home btn long press detection
    static inline uint8_t homeBtnLongPress_ = 0;
    // debouncing counters for each button
    static inline uint8_t debounceCounters_[11] = {0};

    static void initializeButtons() {
        // required to be in sync with the button sampling
        ASSERT(tickCounter_ == 0);
        LOG("init buttons...")
        // pull all buttons up
        gpio::setAsInputPullup(AVR_PIN_BTN_1);
        gpio::setAsInputPullup(AVR_PIN_BTN_2);
        gpio::setAsInputPullup(AVR_PIN_BTN_3);
        gpio::setAsInputPullup(AVR_PIN_BTN_4);
        // force all button groups to go high
        gpio::outputHigh(AVR_PIN_BTN_DPAD);
        gpio::outputHigh(AVR_PIN_BTN_ABXY);
        gpio::outputHigh(AVR_PIN_BTN_CTRL);
        // enable the control group
        gpio::outputLow(AVR_PIN_BTN_CTRL);
        // reset home button long press
        homeBtnLongPress_ = RCKID_HOME_BUTTON_LONG_PRESS_FPS;
    }

    static bool getDebouncedButtonValue(Btn btn) {
        bool last = ts_.state.button(btn);
        bool current; 

        if (debounceCounters_[static_cast<uint8_t>(btn)] != 0) {
            --debounceCounters_[static_cast<uint8_t>(btn)];
            return last;
        }
        switch (btn) {
            case Btn::Home:
            case Btn::A:
            case Btn::Right:
                current = ! gpio::read(AVR_PIN_BTN_1);
                break;
            case Btn::VolumeUp:
            case Btn::B:
            case Btn::Up:
                current = ! gpio::read(AVR_PIN_BTN_2);
                break;
            case Btn::VolumeDown:
            case Btn::Start:
            case Btn::Left:
                current = ! gpio::read(AVR_PIN_BTN_3);
                break;
            case Btn::Select:
            case Btn::Down:
                current = ! gpio::read(AVR_PIN_BTN_4);
                break;
            default:
                ASSERT(false);
        }
        if (current != last)
            debounceCounters_[static_cast<uint8_t>(btn)] = RCKID_BUTTON_DEBOUNCE_TICKS;
        return current;
    }

    static void readControlGroup() {
        bool btnHome = getDebouncedButtonValue(Btn::Home);
        bool btnVolumeUp = getDebouncedButtonValue(Btn::VolumeUp);
        bool btnVolumeDown = getDebouncedButtonValue(Btn::VolumeDown);
        bool changed = ts_.state.setButton(Btn::Home, btnHome);
        changed = ts_.state.setButton(Btn::VolumeUp, btnVolumeUp) || changed;
        changed = ts_.state.setButton(Btn::VolumeDown, btnVolumeDown) || changed;

        // in power off mode, the buttons are not checked via the control groups, but directly via the ISR for home button
        ASSERT(! isPowerOff());

        // move to the next group immediately in non poweroff modes (ticks are running)
        gpio::high(AVR_PIN_BTN_CTRL);
        gpio::low(AVR_PIN_BTN_ABXY);

        // TODO



    }

    static void readABXYGroup() {
        bool btnA = getDebouncedButtonValue(Btn::A);
        bool btnB = getDebouncedButtonValue(Btn::B);
        bool btnSelect = getDebouncedButtonValue(Btn::Select);
        bool btnStart = getDebouncedButtonValue(Btn::Start);
        bool changed = ts_.state.setButton(Btn::A, btnA);
        changed = ts_.state.setButton(Btn::B, btnB) || changed;
        changed = ts_.state.setButton(Btn::Select, btnSelect) || changed;
        changed = ts_.state.setButton(Btn::Start, btnStart) || changed;

        // move to the next group immediately to maximize the time between group mode & button read, print state change id debugging enabled. Note that this function is only expected to be called when we are not powered down (i.e. we have running ticks)
        ASSERT(! isPowerOff());
        gpio::high(AVR_PIN_BTN_ABXY);
        gpio::low(AVR_PIN_BTN_DPAD);
        if (changed) {
            LOG("ABXY: " << ts_.state.button(Btn::A) << " " << ts_.state.button(Btn::B) << " " << ts_.state.button(Btn::Select) << " " << ts_.state.button(Btn::Start));
            setIrq();
        }
    }

    static void readDPadGroup() {
        bool btnLeft = getDebouncedButtonValue(Btn::Left);
        bool btnRight = getDebouncedButtonValue(Btn::Right);
        bool btnUp = getDebouncedButtonValue(Btn::Up);
        bool btnDown = getDebouncedButtonValue(Btn::Down);
        bool changed = ts_.state.setButton(Btn::Left, btnLeft);
        changed = ts_.state.setButton(Btn::Right, btnRight) || changed;
        changed = ts_.state.setButton(Btn::Up, btnUp) || changed;
        changed = ts_.state.setButton(Btn::Down, btnDown) || changed;

        // move to the next group immediately to maximize the time between group mode & button read, print state change id debugging enabled. Note that this function is only expected to be called when we are not powered down (i.e. we have running ticks)
        ASSERT(! isPowerOff());
        gpio::high(AVR_PIN_BTN_DPAD);
        gpio::low(AVR_PIN_BTN_CTRL);
        if (changed) {
            LOG("DPAD: " << state_.status.button(Btn::Left) << " " << state_.status.button(Btn::Right) << " " << state_.status.button(Btn::Up) << " " << state_.status.button(Btn::Down));
            setIrq();
        }
    }
    //@}

    /** \name ADC (voltage and temperature)

        The ADC is used to measure battery voltage and temperature, alternating every second between the two. This should be a compromise between the latency of the measurements and power consumption. The ADC works in a single converstion mode, is triggered manually and we are notified about the result via interrupt (the ADC can run in standby mode).
     */
    //@{

    static void startADC(uint8_t muxpos) {
        // disable the ADC        
        ADC0.CTRLA = 0;
        ADC0.INTCTRL = 0;
        // set voltage reference to 1v1 for temperature checking
        VREF.CTRLA &= ~ VREF_ADC0REFSEL_gm;
        VREF.CTRLA |= VREF_ADC0REFSEL_1V1_gc;
        // initialize ADC0 common properties without turning it on
        ADC0.CTRLB = ADC_SAMPNUM_ACC32_gc;
        ADC0.CTRLD = ADC_INITDLY_DLY32_gc;
        ADC0.SAMPCTRL = 31;
        switch (muxpos) {
            case ADC_MUXPOS_TEMPSENSE_gc:
                // for temperature sensor, use the internal 1.1V reference
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_INTREF_gc | ADC_SAMPCAP_bm;
                ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;
                break;
            case ADC_MUXPOS_AIN2_gc:
                // use VDD as reference for battery voltage measurement
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
                ADC0.MUXPOS = ADC_MUXPOS_AIN2_gc;
                break;
            default:
                // default values, athough we do not support other channel and this should never happen
                UNREACHABLE;
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
                ADC0.MUXPOS = muxpos;
                break;
        }
        // enable the interrupt and start the comversion start the ADC conversion
        ADC0.INTCTRL = ADC_RESRDY_bm;
        ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc | ADC_RUNSTBY_bm;
        ADC0.COMMAND = ADC_STCONV_bm;
    }

    static void measureADC() {
        // don't measure if not ready
        if ((ADC0.INTFLAGS & ADC_RESRDY_bm) == 0)
            return;
        // clear the flag
        ADC0.INTFLAGS = ADC_RESRDY_bm;
        uint16_t value = ADC0.RES / 32;
        uint8_t muxpos = ADC0.MUXPOS;
        switch (muxpos) {
            case ADC_MUXPOS_TEMPSENSE_gc: {
                int8_t sigrow_offset = SIGROW.TEMPSENSE1; 
                uint8_t sigrow_gain = SIGROW.TEMPSENSE0;
                int32_t t = value - sigrow_offset; // Result might overflow 16 bit variable (10bit+8bit)
                t *= sigrow_gain;
                // temp is now in kelvin range, to convert to celsius, remove -273.15 (x256)
                t -= 69926;
                t += 0x40; // rounding on 0.5 degrees
                // and now loose precision to 0.5C (x10, i.e. -15 = -1.5C)
                t = (t >>= 7) * 5;
                ASSERT(t >= -1000 && t < 1000);
                ts_.temp = static_cast<int16_t>(t);
                break;
            }
            case ADC_MUXPOS_AIN2_gc:
                // the battery voltage can be anything from 2.5 to 5V since the voltage of the AVR is fixed at 3v3, the VCC is measured through a voltage divider of 100k + 200k. With 10bit ADC this gives us 1023 being equal 4.95V and lsb of 4.838mV. 
                // this converts the 16bit value to volts x 100:
                value = (48 * value + 50) / 100;
                ts_.state.setVcc(value);
                // if we are below VCC threshold, clear the DC and VUSB power modes, which also forces the device to go to sleep unless other power mode is enabled
                // in the below calls, it is ok to set & clear modes even if already in the expected state as the function bail out immediately
                if (isPowerModeDC() && (value < RCKID_VUSB_THRESHOLD)) {
                    clearPowerMode(POWER_MODE_CHARGING);
                    clearPowerMode(POWER_MODE_DC);
                } else if (value > RCKID_VUSB_THRESHOLD) {
                    setPowerMode(POWER_MODE_DC);
                    if (gpio::read(AVR_PIN_CHARGING) == 0) {
                        setPowerMode(POWER_MODE_CHARGING);
                    } else {
                        clearPowerMode(POWER_MODE_CHARGING);
                    }
                }
                // emergency shutdown if battery too low
                if (value < RCKID_POWER_ON_THRESHOLD && (powerMode_ & POWER_MODE_ON)) {
                    // TODO we might need a ring buffer for this to elliminate spurious shutdowns etc
                    powerOffTimeout_ = RCKID_POWEROFF_ACK_TIMEOUT_FPS;
                    ts_.state.setPowerOffInterrupt(true);
                    // inform user the shutdown is because of critical battery level
                    criticalBattery();
                }
                break;
            default:
                UNREACHABLE;
        }
        // turn ADC off to save power
        ADC0.CTRLA = 0;
    }

    //@}


    /** \name RGB LEDs
     */
    //@{

    static void rgbOn(bool value) {
        // TODO
    }

    static void criticalBattery() {
        // TODO
    }
    //@}

    static inline TransferrableState ts_;

}; // RCKid

/** I2C slave action.
 */
ISR(TWI0_TWIS_vect) {
    RCKid::i2cSlaveIRQHandler();    
}


int main() {
    RCKid::initialize();
    while (true)
        RCKid::loop();
}
