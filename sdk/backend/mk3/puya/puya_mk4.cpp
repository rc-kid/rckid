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


 */


using namespace rckid;


class KidDo {
public:

    static void initialize() {
        cpu::initialize();
        // TODO initialize watchdog
        serial::initializeTx(RCKID_RP_SERIAL_SPEED, PUYA_PIN_TX);
        LOG("PY32F030 initialize");
        initializeGPIO();
        initializeRTC();
        initializeI2C();


        
    }

    static void loop() {

    }

    static void secondTick() {
        updateDateTime();
    }

private:
    /** Initializes the GPIO pins to their default values.
     */
    static void initializeGPIO() {


        LOG("  gpio");
    }

    /** RTC & wakeup alarm
     */
    //@{

    static constexpr uint32_t EPOCH_START_MJD = 58119; // 2018/1/1 
    static constexpr uint32_t SECONDS_PER_DAY = 3600 * 24;

    static void initializeRTC() {
        rtc::enable();
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

    //@}

    /** I2C Communications with the RP2350
     */
    //@{
    static inline TransferrableState ts_;

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
     */
    //@{
    //@}

    /** \name PWM (Rumbler)
     */
    //@{
    //@}

}; // class KidDo

extern "C" {
    void RTC_IRQHandler(void) {
        if (LL_RTC_IsActiveFlag_SEC(RTC)) {
            LL_RTC_ClearFlag_SEC(RTC);
            KidDo::secondTick();
        }
        if (LL_RTC_IsActiveFlag_ALR(RTC)) {
            LL_RTC_ClearFlag_ALR(RTC);
            // TODO code
        }
    }
}

int main() {
    KidDo::initialize();
    while (true)
        KidDo::loop();
}