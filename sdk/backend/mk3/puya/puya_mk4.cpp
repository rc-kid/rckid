#include <platform.h>
#include <writer.h>
#include <config.h>

#define LOG(...) debugWrite() << __VA_ARGS__ << "\r\n"
#define ASSERT(...) do { if (!(__VA_ARGS__)) { LOG("ASSERT " << __LINE__); while (true) {}; } } while (false)
#define UNREACHABLE do { LOG("Unreachable: " << __LINE__); while (true) {}; } while (false)
#define UNIMPLEMENTED do { LOG("Unimplemented: " << __LINE__); while (true) {} } while (false)

inline Writer debugWrite() {
    return Writer{serial::write};
}

#include "puya_commands.h"

/** PY32F030 as IO MCU
 
    In mkIV we are switching from the ATTiny3217 to PY32F030 as the IO MCU. This is mostly to save costs as the ATTiny3217 is even more expensive than the RP2350 itself. 

    - checking & decoupling the input buttons
    - timekeeping
    - monitoring of the battery voltage levels, USB connection and charging status
    - 
    - powering the device on & off


    States

    *Off*: The off state is the only one in which the IO MCU sleeps as well. It will wake up on the following conditions:

    - DC power is plugged in, in which case it transitions to the OffDC state
    - Alarm is triggered -> Alarm state
    - Home button is pressed, in which case the device transition to the Wake state

    *OffDC*: The off state (RP2350 is off) with DC power plugged in. The IO MCU is alive and runs the normal loop checking buttons, monitors battery charging / DC power and flashes the RGB LEDs as intended to indicate statuss. This state is necessary so that RGB light effects and charging monitoring can take place. Transitions from this state are as follows:

    - DC power removed -> go to Off state and sleep
    - Home button pressed -> go to wake state 
    - Alarm triggered -> go to Alarm state

    *Wake*: An intermediate state where the device runs the puya main loop so that we have precise timing available and can time long press of a home button. The transitions are as follows:

    - Alarm triggered - go to Alarm state
    - Home button long press detected - go to On state
    - Home button released - go to Off state, or OffDC state if DC power is plugged in

    *Alarm*: Alarm is a state when a powered off device (or waking device) is woken up by an alarm. Alarm state immediately turns on the RP2350, but otherwise behaves as a combination of Off and Wake states, i.e. it detects long press, to power on, etc. In other words in this state the RP2350 is *on*, but should not exercise full control over the device.

    - home button long press detected - to go On state
    - wakeup state timeouts or power down is requested - go to PowerDown state
    - RP2350 requests transition to On state - go to On state

    *On*: This is the state when the RP2350 is powered on and the control of the device's I/O is handed over to the big chip & cartridge firmware. The on state can only transition to Power down state either on request, or on timeout. 

    *PowerDown*: In this state device is preparing to power the RP2350 down. The RP2350 is still powered on, but the IO chip informs it of the transition. The RP2350 now *must* initiate its own shutdown sequence, but *if* it acknowledges the power down request, it is given a generous timeout to complete the shutdown. When the timeout expries (either the short one, or the ecxtended one after ack), or when immediate power off is requested by the RP2350, the IO chip powers the RP2350 down and goes to Off or OffDC state depending on whether the DC power is plugged in or not. 

    One small drawback is that the PowerDown state does not check for home button presses, i.e. if a device is powered on immediately after it is powered off, the home button press will be ignored. This is not a big deal as the user can simply press the home button again to power on the device. Since the press is ignored completely, users won't even get the rumbler feedback from it, masking as a failed press in this very unlikely situation.
 */


using namespace rckid;


extern "C" {
    void TIM3_IRQHandler();
    void RTC_IRQHandler();
}

class KidDo {
public:

    static void initialize() {
        cpu::initialize(PUYA_CLOCKSPEED);
        // TODO initialize watchdog
        serial::initializeTx(RCKID_RP_SERIAL_SPEED, PUYA_PIN_TX);
        LOG("PY32F030 initialize");

        ts_.setState(mkIV::DeviceState::State::On);
        initializeGPIO();
        initializeRTC();
        initializeI2C();


        initializeButtons();
        enableSystemTick(true);

        /*
        initializeRumbler();
        setRumblerPWM(0);
        setRumblerPWM(5);
        setRumblerPWM(1);
        */

        LOG("done.");        
    }

    static void loop() {
        if (systemTick_)
            systemTick();
        if (secondTick_)
            secondTick();
        if (ts_.alarmInterrupt()) {
            ts_.setAlarmInterrupt(false);
            LOG("Alarm interrupt");
        }
    }

private:
    /** Initializes the GPIO pins to their default values.
     */
    static void initializeGPIO() {


        LOG("  gpio");
    }

    /** \name DeviceState, power management and system ticks
     */
    //@{
    static inline mkIV::TransferrableState ts_;
    static inline volatile bool systemTick_ = false;
    static inline uint32_t systemTickId_ = 0;

    //@}
    static void setState(mkIV::DeviceState::State s) {
        switch (s) {
            case mkIV::DeviceState::State::Off:
                ASSERT(ts_.dcPower() == false);
                enableIOVDD(false);
                enableSystemTick(false);

                break;
            case mkIV::DeviceState::State::OffDC:
                // TODO power off the RP2350
                break;
            case mkIV::DeviceState::State::Wake:
                // TODO power on the RP2350
                break;
            case mkIV::DeviceState::State::Alarm:
                // TODO power on the RP2350
                break;
            case mkIV::DeviceState::State::PowerDown:
                // TODO request power down from the RP2350
                break;
            case mkIV::DeviceState::State::On:
                // TODO power on the RP2350
                break;
        }
    }

    static void enableIOVDD(bool value = true) {
        if (value) {
            // enable IOVDD
            // ebable I2C
        } else {
            // disable I2C
            // disable IOVDD
        }

    }

    /** System tick is driven by TIM3's update interrupt. The timer is prescaled to 1MHz and reloads every 1MHz / SYSTEM_TICK_HZ ticks (5000 for 200Hz, fits the 16bit counter).
     */
    static void enableSystemTick(bool value = true) {
        if (value) {
            ASSERT(PUYA_CLOCKSPEED == cpu::ClockSpeed::MHz8);
            timer3::enable(1000000 / PUYA_SYSTEM_TICK_HZ, 8); // 8 MHz / 8 / 5000 == 200 Hz
            LL_TIM_EnableIT_UPDATE(TIM3);
            NVIC_SetPriority(TIM3_IRQn, 1);
            NVIC_EnableIRQ(TIM3_IRQn);
        } else {
            NVIC_DisableIRQ(TIM3_IRQn);
            LL_TIM_DisableIT_UPDATE(TIM3);
            timer3::disable();
            LL_TIM_ClearFlag_UPDATE(TIM3);
            NVIC_ClearPendingIRQ(TIM3_IRQn);
            // disable ADC
            // reset buttons to control group reading
            rgbOff();
            
        }

    }

    /** Called from the TIM3 interrupt at SYSTEM_TICK_HZ when system tick is enabled.
     */
    static void systemTick() { 
        systemTick_ = false;
        switch (systemTickId_++) {
            case 0:
                readControlGroup();
                break;
            case 1:
                readABXYGroup();
                break;
            default:
                readDPadGroup();
                systemTickId_ = 0;
                break;
        }
    }


    /** RTC timekeeping and wakeup alarm
     */
    //@{

    static constexpr uint32_t EPOCH_START_MJD = 58119; // 2018/1/1 
    static constexpr uint32_t SECONDS_PER_DAY = 3600 * 24;
    static volatile inline bool secondTick_ = false;

    static void initializeRTC() {
        rtc::enable();
        rtc::setCounter(0);
        rtc::setAlarmCounter(0);
        rtc::enableAlarmInterrupt(false);
        rtc::enableSecondInterrupt(true);
        // TODO set the puya RTC values correctly
        LOG("  rtc " << rtc::enabled());
    }

    static void setDateTime(TinyDateTime const & dt) {
        ts_.time = dt;
        rtc::setCounter(
            (dt.date.mjd() - EPOCH_START_MJD) * SECONDS_PER_DAY + 
            dt.time.hour() * 3600 + dt.time.minute() * 60 + dt.time.second()
        );
        // TODO update the alarm
    }

    static void updateDateTime() {
        uint32_t seconds = rtc::counter();
        ts_.time.date = TinyDate{EPOCH_START_MJD + seconds / SECONDS_PER_DAY};
        ts_.time.time = TinyTime{seconds % SECONDS_PER_DAY};
    }

    static void setWakeupAlarm(uint32_t seconds) {
        rtc::setAlarmCounter(rtc::counter() + seconds);
    }

    static void secondTick() {
        secondTick_ = false;
        updateDateTime();

        //LOG(ts_.time.time);
    }


    //@}

    /** I2C Communications with the RP2350
     */
    //@{

    static void initializeI2C() {

        LOG("  i2c");
    }
    //@}


    /** \name Buttons 

        The button matrix is 3 groups of 4 buttons. Namely the BTN_CTRL group selects the Home button and volume up & down side buttons, the BTN_ABXY group selects the A, B and Select & Start buttons and the BTN_DPAD group selects the dpad. 

        When idle, all button pins are in input mode, pulled up. Button groups all output logical high. When a button group is selected, only that group's pin goes to logical 0. We then read the four buttons and reading 0 means the button is pressed (i.e. connected to the button group). The diodes from buttons to group pins ensure that no ghosting happens (i.e. low from one button would through different group enter other button here.

        All button pins are digital and are read as part of system tick. 
     */
    //@{
    static inline uint32_t homeBtnLongPress_ = 0;
    static inline uint32_t debounceCounters_[11] = {0};

    static void initializeButtons() {
        // button matrix - set buttons as input pullup so they stay high when not pressed
        gpio::setAsInputPullup(PUYA_PIN_BTN_1);
        gpio::setAsInputPullup(PUYA_PIN_BTN_2);
        gpio::setAsInputPullup(PUYA_PIN_BTN_3);
        gpio::setAsInputPullup(PUYA_PIN_BTN_4);
        // set all controls to high except for the CTRL buttons which we are reading
        gpio::outputHigh(PUYA_PIN_BTN_DPAD);
        gpio::outputHigh(PUYA_PIN_BTN_ABXY);
        gpio::outputLow(PUYA_PIN_BTN_CTRL);
        // reset home button long press
        homeBtnLongPress_ = RCKID_HOME_BUTTON_LONG_PRESS_FPS;
    }

    static bool getDebouncedButtonValue(Btn btn) {
        bool last = ts_.button(btn);
        bool current; 

        if (debounceCounters_[static_cast<uint8_t>(btn)] != 0) {
            --debounceCounters_[static_cast<uint8_t>(btn)];
            return last;
        }
        switch (btn) {
            case Btn::Home:
            case Btn::A:
            case Btn::Right:
                current = ! gpio::read(PUYA_PIN_BTN_1);
                break;
            case Btn::VolumeDown:
            case Btn::B:
            case Btn::Up:
                current = ! gpio::read(PUYA_PIN_BTN_2);
                break;
            case Btn::VolumeUp:
            case Btn::Start:
            case Btn::Left:
                current = ! gpio::read(PUYA_PIN_BTN_3);
                break;
            case Btn::Select:
            case Btn::Down:
                current = ! gpio::read(PUYA_PIN_BTN_4);
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

        bool changed = ts_.setButton(Btn::Home, btnHome);
             changed = ts_.setButton(Btn::VolumeUp, btnVolumeUp) || changed;
             changed = ts_.setButton(Btn::VolumeDown, btnVolumeDown) || changed;


        // move to the next group immediately to maximize the time between group mode & button read, print state change id debugging enabled. Note that this function is only expected to be called when we are not powered down (i.e. we have running ticks)
        ASSERT(ts_.state() != mkIV::DeviceState::State::Off);
        gpio::high(PUYA_PIN_BTN_CTRL);
        gpio::low(PUYA_PIN_BTN_ABXY);

        if (changed) {
            LOG("CTRL: " << ts_.button(Btn::Home) << " " << ts_.button(Btn::VolumeUp) << " " << ts_.button(Btn::VolumeDown));
        }
        
#ifdef HAHA        
        bool changed = ts_.state.setButton(Btn::Home, btnHome);

        // in case we are running normal system ticks, check if the home button press should be interpreted as wakeup start
        if (changed && btnHome && ! isPowerModeOn())
            setPowerMode(POWER_MODE_WAKEUP);

        changed = ts_.state.setButton(Btn::VolumeUp, btnVolumeUp) || changed;
        changed = ts_.state.setButton(Btn::VolumeDown, btnVolumeDown) || changed;

        // in power off mode, the buttons are not checked via the control groups, but directly via the ISR for home button
        ASSERT(! isPowerModeOff());

        // move to the next group immediately in non poweroff modes (ticks are running)
        gpio::high(AVR_PIN_BTN_CTRL);
        gpio::low(AVR_PIN_BTN_ABXY);

        // check long press of home button here to enable device power on, or power off
        if (btnHome && (homeBtnLongPress_ == 0))
            startHomeButtonLongPress();
        else 
            checkHomeButtonLongPress();

        if (changed) {
            LOG("CTRL: " << ts_.state.button(Btn::Home) << " " << ts_.state.button(Btn::VolumeUp) << " " << ts_.state.button(Btn::VolumeDown));

            // if we are in wakeup mode *and* the volume down button is not pressed, we should exit the debug mode (it was set in wakeup mode enable proactively)
            if (isPowerModeWakeup()) {
                if (! btnVolumeDown)
                    ts_.state.setDebugMode(false);
            }

            // if we are in debug mode, react to volume button presses immediately to either reboot the RP, or enter bootloader mode
            if (isPowerModeOn() && ts_.state.debugMode()) {
                if (btnVolumeUp) {
                    rebootRP();
                    return;
                }
                if (btnVolumeDown) {
                    bootloaderRP();
                    return;
                }
            }
        }
#endif
    }

    static void readABXYGroup() {
        bool btnA = getDebouncedButtonValue(Btn::A);
        bool btnB = getDebouncedButtonValue(Btn::B);
        bool btnSelect = getDebouncedButtonValue(Btn::Select);
        bool btnStart = getDebouncedButtonValue(Btn::Start);
        bool changed = ts_.setButton(Btn::A, btnA);
             changed = ts_.setButton(Btn::B, btnB) || changed;
             changed = ts_.setButton(Btn::Select, btnSelect) || changed;
             changed = ts_.setButton(Btn::Start, btnStart) || changed;

        // move to the next group immediately to maximize the time between group mode & button read, print state change id debugging enabled. Note that this function is only expected to be called when we are not powered down (i.e. we have running ticks)
        ASSERT(ts_.state() != mkIV::DeviceState::State::Off);
        gpio::high(PUYA_PIN_BTN_ABXY);
        gpio::low(PUYA_PIN_BTN_DPAD);
        if (changed) {
            LOG("ABXY: " << ts_.button(Btn::A) << " " << ts_.button(Btn::B) << " " << ts_.button(Btn::Select) << " " << ts_.button(Btn::Start));
        }
    }

    static void readDPadGroup() {
        bool btnLeft = getDebouncedButtonValue(Btn::Left);
        bool btnRight = getDebouncedButtonValue(Btn::Right);
        bool btnUp = getDebouncedButtonValue(Btn::Up);
        bool btnDown = getDebouncedButtonValue(Btn::Down);
        bool changed = ts_.setButton(Btn::Left, btnLeft);
             changed = ts_.setButton(Btn::Right, btnRight) || changed;
             changed = ts_.setButton(Btn::Up, btnUp) || changed;
             changed = ts_.setButton(Btn::Down, btnDown) || changed;

        // move to the next group immediately to maximize the time between group mode & button read, print state change id debugging enabled. Note that this function is only expected to be called when we are not powered down (i.e. we have running ticks)
        ASSERT(ts_.state() != mkIV::DeviceState::State::Off);
        gpio::high(PUYA_PIN_BTN_DPAD);
        gpio::low(PUYA_PIN_BTN_CTRL);
        if (changed) {
            LOG("DPAD: " << ts_.button(Btn::Left) << " " << ts_.button(Btn::Right) << " " << ts_.button(Btn::Up) << " " << ts_.button(Btn::Down));
        }
    }

    //@}

    /** \name ADC (voltage and temperature)

        The ADC is used to measure battery voltage and temperature, alternating every second between the two. This should be a compromise between the latency of the measurements and power consumption. The ADC works in a single converstion mode, is triggered manually and we are notified about the result via interrupt (the ADC can run in standby mode).
     */
    //@{

    //@}

    /** \name Accelerometer
     
        The accelerometer is connected to SPI1. 
     */
    //@{
    //@}

    /** \name RGB LEDs
     
        The RGB LEDs are connected to SPI2, where we are using only MOSI to drive the 1 wire interface.
     */
    //@{
    static void rgbOff() {
        // clear effects, send black to all LEDs to power them off
    }
    //@}

    /** \name PWM (Rumbler)
     
        Rumbler is connected to PA11, which is Timer 1 channel 4. The timer is configured to run at 20kHz, which is a good compromise between audible noise and power consumption. The duty cycle can be set from 0 to 10, where 0 means full off and 10 means full on. 
     */
    //@{

    static constexpr uint8_t RUMBLER_DUTY_PWM [] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 99};

    static void initializeRumbler() {
        // enable the timer1 to run at 20 kHz
        if (PUYA_CLOCKSPEED == cpu::ClockSpeed::MHz8) {
            timer1::enable(100, 4); // 8 MHz / 4 / 100 == 20 kHz
        } else {
            UNIMPLEMENTED;
        }
    }

    /** Sets the rumbler PWM by converting the duty cycle from 0 to 10 to the actual PWM comparison values.
     */
    static void setRumblerPWM(uint32_t duty) {
        ASSERT(duty <= 10);
        static_assert(PUYA_PIN_PWM_RUMBLER == gpio::PA11, "Rumbler pin must be PA11 (timner 1 channel 4)");
        if (duty == 0) {
            timer1::disableChannel(4);
            gpio::outputLow(PUYA_PIN_PWM_RUMBLER);
        } else if (duty >= 10) {
            timer1::disableChannel(4);
            gpio::outputHigh(PUYA_PIN_PWM_RUMBLER);
        } else {
            gpio::configureAsAlternate(PUYA_PIN_PWM_RUMBLER, gpio::AlternateFunction::TIM1_CH4);
            timer1::enableChannelPWM(4, RUMBLER_DUTY_PWM[duty]);
        }
    }
    //@}


    friend void TIM3_IRQHandler();
    friend void RTC_IRQHandler();

}; // class KidDo

extern "C" {
    void TIM3_IRQHandler() {
        if (LL_TIM_IsActiveFlag_UPDATE(TIM3)) {
            LL_TIM_ClearFlag_UPDATE(TIM3);
            KidDo::systemTick_ = true;
        }
    }

    void RTC_IRQHandler() {
        if (LL_RTC_IsActiveFlag_SEC(RTC)) {
            LL_RTC_ClearFlag_SEC(RTC);
            KidDo::secondTick_ = true;
        }
        if (LL_RTC_IsActiveFlag_ALR(RTC)) {
            LL_RTC_ClearFlag_ALR(RTC);
            // KidDo::ts_.setAlarmInterrupt(true);
            // TODO code for device wakeup
        }
    }
}

int main() {
    KidDo::initialize();
    while (true)
        KidDo::loop();
}