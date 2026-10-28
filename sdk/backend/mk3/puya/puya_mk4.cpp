#include <platform.h>
#include <py32f0xx_ll_i2c.h>
#include <py32f0xx_ll_dma.h>
#include <py32f0xx_ll_system.h>
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

    - Home button long press detected - go to On state
    - Home button released - go to Off state, or OffDC state if DC power is plugged in
    - note that we do not allow transition to alarm state from the wake state - if the wake is successful, then the wakeup is reflected in the interrupts and the RP will react to it as if happens during the on state. If unsuccessful then the revert from wake to Off/OffDC will instead trigger the alarm state immediately.

    *Alarm*: Alarm is a state when a powered off device is woken up by an alarm. Alarm state immediately turns on the RP2350, but otherwise behaves as a combination of Off and Wake states, i.e. it detects long press, to power on, etc. In other words in this state the RP2350 is *on*, but should not exercise full control over the device.

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
    void I2C1_IRQHandler();
    void DMA1_Channel1_IRQHandler();
    void EXTI0_1_IRQHandler();
    void EXTI2_3_IRQHandler();
    void ADC_COMP_IRQHandler();
}

class KidDo {
public:

    static void initialize() {
        cpu::initialize(PUYA_CLOCKSPEED);
        // TODO initialize watchdog
        serial::initializeTx(RCKID_RP_SERIAL_SPEED, PUYA_PIN_TX);
        LOG("PY32F030 initialize");
        initializeGPIO();
        initializeButtons();
        bool wdtReset = LL_RCC_IsActiveFlag_IWDGRST();
        if (wdtReset) {
            LOG("  wdt triggered reset - rtc unchanged");
            rtc::resume();
            updateDateTime();
        } else {
            initializeRTC();
        }
        LL_RCC_ClearResetFlags();
        cpu::wdtReset();
        cpu::wdtEnable(RCKID_PUYA_WATCHDOG_TIMEOUT_MS);
        LOG("  wdt");
        // the RTC second tick runs in all states, in the Off state its wakeups are what keeps resetting the watchdog
        rtc::enableSecondInterrupt(true);

        if (wdtReset) {
            // we have crashed or hung, so do not power the RP2350 on (the device might have been off). Stay in the default Off state with the wakeup sources configured, or go to OffDC if charging (active low). Note that DC power without charging is not detected here as the ADC does not run in the Off state
            gpio::outputLow(PUYA_PIN_IOVDD_EN);
            enableSystemTick(false);
            if (! gpio::read(PUYA_PIN_CHARGING))
                setState(mkIV::DeviceState::State::OffDC);
        } else {
            setState(mkIV::DeviceState::State::On);
            // TODO remove this when ready testing on the device
            ts_.setDebugMode(true);
        }
        LOG("done.");        
    }

    static void loop() {
        cpu::wdtReset();
        systemTick();
        secondTick();
        processI2CCommand();
        adcMeasure();
        trySleep();
    }

private:
    /** Initializes the GPIO pins to their default values.
     */
    static void initializeGPIO() {
        // initialkize the charging interrupt pin (this is active at all times)
        static_assert(PUYA_PIN_CHARGING == gpio::PB1, "Charging signal must be on PB1 (EXTI line 1)");
        // TODO verify pull against the charger's status output
        gpio::setAsInputPullup(PUYA_PIN_CHARGING);
        LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTB, LL_EXTI_CONFIG_LINE1);
        // TODO I need to figure which one (this only fires when charging starts)
        LL_EXTI_EnableRisingTrig(LL_EXTI_LINE_1);
        LL_EXTI_EnableFallingTrig(LL_EXTI_LINE_1);
        LL_EXTI_ClearFlag(LL_EXTI_LINE_1);
        LL_EXTI_EnableIT(LL_EXTI_LINE_1);
        NVIC_SetPriority(EXTI0_1_IRQn, 1);
        NVIC_EnableIRQ(EXTI0_1_IRQn);
        LOG("  gpio");
    }

    /** \name DeviceState, power management and system ticks
     */
    //@{
    static inline mkIV::TransferrableState ts_;
    static inline volatile bool systemTick_ = false;
    static inline uint32_t systemTickId_ = 0;

    static inline volatile uint32_t rpWatchdogCounter_ = 0;
    static inline bool powerDownAck_ = false;

    //@}
    static void setState(mkIV::DeviceState::State s) {
        LOG("State: " << static_cast<uint32_t>(ts_.state()) << " -> " << static_cast<uint32_t>(s));
        // ensure this is indeed a transition
        ASSERT(ts_.state() != s);
        // ifthe current state is Off, we must be moving to a state that requires the system ticks and therefore must enable them
        if (ts_.state() == mkIV::DeviceState::State::Off)
            enableSystemTick();
        // if we are leaving alarm state, clear the alarm interrupt
        if (ts_.state() == mkIV::DeviceState::State::Alarm) {
            ts_.setAlarmInterrupt(false);
            // TODO we might want to schedule some default alarm for next here, if not set by the rp to deal with daily allowances, etc? Or deal with this on the PUYA on its own? 
        }
        // switch what we do based on the desired state
        switch (s) {
            case mkIV::DeviceState::State::Off:
                // when going to the power off state, we must ensure that everything is truly off, as the next loop cycle will enter the sleep method and put the device to sleep.
                ASSERT(! poweredOn());
                ASSERT(ts_.dcPower() == false);
                ts_.setState(s);
                enableSystemTick(false);
                // if we have pending alarm interrupt, ener the alarm state instead
                if (ts_.alarmInterrupt()) {
                    ts_.setState(s);
                    setState(mkIV::DeviceState::State::Alarm);
                }
                return; // we already set the state above
            case mkIV::DeviceState::State::OffDC:
                ASSERT(! poweredOn());
                // force the DC power flag to true - it will be updated by the ADCs later, but we need to set it now so that the state is consistent with the actual power state
                ts_.setDcPower(true);
                // system tick already enabled by the above, if moving from Off state
                // if we have pending alarm interrupt, ener the alarm state instead
                if (ts_.alarmInterrupt()) {
                    ts_.setState(s);
                    setState(mkIV::DeviceState::State::Alarm);
                    return;
                }
                break;
            case mkIV::DeviceState::State::Wake:
                // when waking up, ensure that the Btn::Home is considered pressed and reset the long press counter 
                ts_.setButton(Btn::Home, true);
                homeBtnLongPressCounter_ = RCKID_HOME_BUTTON_LONG_PRESS_MS / (1000 / PUYA_SYSTEM_TICK_HZ);
                break;
            case mkIV::DeviceState::State::Alarm:
                // power the device on (we canonly reach alarm state if the device is off)
                ASSERT(ts_.state() == mkIV::DeviceState::State::Off || ts_.state() == mkIV::DeviceState::State::OffDC);
                powerOn();
                // reload the watchdog state to the alarm timeout
                reloadRpWatchdogCounter(RCKID_ALARM_TIMEOUT_MS);
                break;
            case mkIV::DeviceState::State::PowerDown:
                // the power down state is a hint for the rp2350 to power itself down. We reset the rp watchog to the power down timeout to ensure timely compliance. In the power down mode the watchdog counter is not reset on I2C transactions and can only be explicitly extended via the power down acknowledge message 
                ASSERT(poweredOn());
                // no need for debug mode anymore
                ts_.setDebugMode(false);
                // set the state early to ensure we do not extend the watchog during I2C reads
                ts_.setState(s);
                reloadRpWatchdogCounter(RCKID_POWERDOWN_TIMEOUT_MS);
                powerDownAck_ = false;
                // we altready set the state above, return immediately, no need to set again at the end
                return;
            case mkIV::DeviceState::State::On:
                // power on RP2350 if we are not coming from the Alarm state
                if (ts_.state() != mkIV::DeviceState::State::Alarm)
                    powerOn();
                reloadRpWatchdogCounter(RCKID_RP2350_WATCHDOG_TIMEOUT_MS);
                // if the RGB lights are doing anything go to black (they are controlled by the device when powered on)
                // TODO
                break;
        }
        // set the state
        ts_.setState(s);
    }

    static void setDebugMode(bool enabled) {
        // TODO do more things here?
        ts_.setDebugMode(enabled);
    }

    static void reloadRpWatchdogCounter(uint32_t time_ms) {
        rpWatchdogCounter_ = time_ms / (1000 / PUYA_SYSTEM_TICK_HZ);
    }

    static bool poweredOn() {
        return gpio::read(PUYA_PIN_IOVDD_EN);
    }

    /** Powers the device on, but does not set the state. 
     */
    static void powerOn() {
        gpio::outputHigh(PUYA_PIN_IOVDD_EN);
        // ebable I2C
        initializeI2C();
    }

    /** Powers the device off and sets the appropriate off state (Off or OffDC)
     */
    static void powerOff() {
        disableI2C();
        gpio::outputLow(PUYA_PIN_IOVDD_EN);
        // set the counter to 0 so that it stops firing
        rpWatchdogCounter_ = 0;
        setState(ts_.dcPower() ? mkIV::DeviceState::State::OffDC : mkIV::DeviceState::State::Off);
    }

    static void dcPowerConnected(bool value = true) {
        if (value) {
            // if enabling DC power, we should not be in the Off or OffDC state - here the charging enabled pin interrupt should trigger the transition to OffDC first, which also proactively enables the DC power flag even without the ADC readout
            ASSERT(ts_.state() != mkIV::DeviceState::State::Off);
            ASSERT(ts_.state() != mkIV::DeviceState::State::OffDC);
            ts_.setDcPower(true);
        } else {
            // do nothing if we are currently charging (means ADC glitch)
            if (ts_.charging())
                return;
            // change the state
            ts_.setDcPower(false);
            // if we are in the OffDC state, go to off state instead, in all other states the DC power flag is enough
            if (ts_.state() == mkIV::DeviceState::State::OffDC)
                setState(mkIV::DeviceState::State::Off);
        }
    }

    static void criticalBattery() {
        LOG("vcc critical");
        // TODO fancy RGB, power off and remember not to power on
    }

    static void reboot() {
        // TODO cycle power
        reloadRpWatchdogCounter(RCKID_RP2350_WATCHDOG_TIMEOUT_MS);
    }

    static void rebootToBootloader() {
        // TODO cycle power with bootloader select

        // disable the watchdog for debugging purposes - during the bootloader there will be no I2C reads, and when the device reboots, the first I2C read will reset the watchdog to a standard timeout
        rpWatchdogCounter_ = 0;
    }

    /** System tick is driven by TIM3's update interrupt. The timer is prescaled to 1MHz and reloads every 1MHz / SYSTEM_TICK_HZ ticks (5000 for 200Hz, fits the 16bit counter).
     */
    static void enableSystemTick(bool value = true) {
        static_assert(PUYA_PIN_BTN_1 == gpio::PA2, "Home button must be on PA2 (EXTI line 2)");
        if (value) {
            ASSERT(PUYA_CLOCKSPEED == cpu::ClockSpeed::MHz8);
            timer3::enable(1000000 / PUYA_SYSTEM_TICK_HZ, 8); // 8 MHz / 8 / 5000 == 200 Hz
            LL_TIM_EnableIT_UPDATE(TIM3);
            NVIC_SetPriority(TIM3_IRQn, 1);
            NVIC_EnableIRQ(TIM3_IRQn);
            // disable the home button interrupt as we are now reading entire button matrix via the system ticks
            NVIC_DisableIRQ(EXTI2_3_IRQn);
            LL_EXTI_DisableIT(LL_EXTI_LINE_2);
            LL_EXTI_DisableFallingTrig(LL_EXTI_LINE_2);
            LL_EXTI_ClearFlag(LL_EXTI_LINE_2);
            NVIC_ClearPendingIRQ(EXTI2_3_IRQn);
            initializeADC();
        } else {
            NVIC_DisableIRQ(TIM3_IRQn);
            LL_TIM_DisableIT_UPDATE(TIM3);
            timer3::disable();
            LL_TIM_ClearFlag_UPDATE(TIM3);
            NVIC_ClearPendingIRQ(TIM3_IRQn);
            // disable ADC, cancelling any measurement in progress
            disableADC();
            rgbOff();
            // ensure any pending system tick is cleared so that we do not run stale ticks in power off mode
            systemTick_ = false;
            // if we were in the middle of a long press counter detection when powering off, cancel it
            homeBtnLongPressCounter_ = 0;
            // reset buttons to control group reading so that the home button can be detected by the interrupt, and the control group is the first one read when ticks are enabled again
            gpio::high(PUYA_PIN_BTN_DPAD);
            gpio::high(PUYA_PIN_BTN_ABXY);
            gpio::low(PUYA_PIN_BTN_CTRL);
            cpu::delayMs(1); // give time to stabilize
            systemTickId_ = 0;
            // enable the home button interrupt so that we can wake up the device from power off state
            LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE2);
            LL_EXTI_EnableFallingTrig(LL_EXTI_LINE_2);
            LL_EXTI_ClearFlag(LL_EXTI_LINE_2);
            LL_EXTI_EnableIT(LL_EXTI_LINE_2);
            NVIC_SetPriority(EXTI2_3_IRQn, 1);
            NVIC_EnableIRQ(EXTI2_3_IRQn);
        }
    }

    /** Called from the TIM3 interrupt at SYSTEM_TICK_HZ when system tick is enabled.
     */
    static void systemTick() { 
        if (! systemTick_)
            return;
        // charging is active low, so the value is effectively negated
        if (ts_.charging() == gpio::read(PUYA_PIN_CHARGING)) {
            ts_.setCharging(! ts_.charging());
            // set power DC to on as well - it has to be if we are charging even if we did not hear from the ADC yet
            if (ts_.charging())
                ts_.setDcPower(true);
            LOG("Charging: " << ts_.charging());
            // TODO some rgb effects? 
        }
        systemTick_ = false;
        switch (systemTickId_++) {
            case 0:
                if (adcPrimed())
                    adcStartConversion();
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
        // if the watchdog has failed, power the device off immediately - but only do so if we are not in the debug mode
        if (! ts_.debugMode()) {
            cpu::DisableInterruptsGuard g_;
            if (rpWatchdogCounter_ > 0 && --rpWatchdogCounter_ == 0) {
                g_.release();
                LOG("watchdog");
                powerOff();
                // TODO some fancy RGB to tell us this happened
            }
        }
        // check if we have home button long press on our hands
        if (homeBtnLongPressCounter_ > 0 && --homeBtnLongPressCounter_ == 0)
            btnHomeLongPress();
        // TODO deal with charging checks

        // TODO if we are *not* in the On state, we should display charging notification if DC is on 
        // if (ts_.state() != mkIV::DeviceState::State::On && ts_.state() != mkIV::DeviceState::State::Off)
            
    }

    static void trySleep() {
        cpu::DisableInterruptsGuard g_;
        if (ts_.state() != mkIV::DeviceState::State::Off)
            return;
        LOG("sleep");
        serial::flush();
        cpu::sleep();
        LOG("  wakeup");
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
        // the second interrupt is enabled by initialize() once the watchdog is running
        rtc::enableSecondInterrupt(false);
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
        if (!secondTick_)
            return;
        secondTick_ = false;
        ts_.time.inc();
        ++ts_.uptime_;
        if (ts_.state() != mkIV::DeviceState::State::Off) {
            adcPrime(); // prime the ADC for a conversion start
        }
    }
    //@}

    /** I2C Communications with the RP2350
     
        Uses simplified DMA based protocol that allows for the following actions: 
        
        - master reads, always read from the beginning of the transferrable state as many bytes as the master wants. 
        - master writes, always write to the command buffer. If the command buffer is full, the write will be NACKed.
    */
    //@{

    static volatile inline uint32_t i2cRxBytes_ = 0;

    static void initializeI2C() {
        i2cRxBytes_ = 0;
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
        LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);
        // SCL & SDA as open drain alternate functions
        static_assert(PUYA_PIN_I2C_SCL == gpio::PB6 && PUYA_PIN_I2C_SDA == gpio::PB7, "I2C must be on PB6 (SCL) and PB7 (SDA), AF6");
        LL_GPIO_InitTypeDef cfg{};
        cfg.Pin = gpio::pinMask(PUYA_PIN_I2C_SCL) | gpio::pinMask(PUYA_PIN_I2C_SDA);
        cfg.Mode = LL_GPIO_MODE_ALTERNATE;
        cfg.Speed = LL_GPIO_SPEED_FREQ_HIGH;
        cfg.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
        cfg.Pull = LL_GPIO_PULL_UP;
        cfg.Alternate = LL_GPIO_AF6_I2C;
        LL_GPIO_Init(GPIOB, & cfg);
        // set the DMA channel 1 peripheral address to I2C1 data register
        LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_1, reinterpret_cast<uint32_t>(& I2C1->DR));
        LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_1);
        NVIC_SetPriority(DMA1_Channel1_IRQn, 0);
        NVIC_EnableIRQ(DMA1_Channel1_IRQn);
        NVIC_SetPriority(I2C1_IRQn, 0);
        NVIC_EnableIRQ(I2C1_IRQn);
        i2cReset();
        LOG("  i2c");
    }

    static void disableI2C() {
        LL_I2C_Disable(I2C1);
        LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_I2C1);
        LL_APB1_GRP2_DisableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
        LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_DMA1);
    }

    static void processI2CCommand() {
        if (i2cRxBytes_ == 0)
            return;
        if (cmd::getCommandSize(ts_.buffer[0]) != i2cRxBytes_) {
            LOG("Invalid size: exp " << cmd::getCommandSize(ts_.buffer[0]) << ", got " << i2cRxBytes_);
            // convert the incomplete size command to NOP
            ts_.buffer[0] = cmd::Nop::ID;
        }
        switch (ts_.buffer[0]) {
            case cmd::Nop::ID:
                break;
            case cmd::PowerOff::ID:
                switch (ts_.state()) {
                    case mkIV::DeviceState::State::PowerDown:
                    case mkIV::DeviceState::State::Alarm:
                        powerOff();
                        break;
                    case mkIV::DeviceState::State::On:
                        setState(mkIV::DeviceState::State::PowerDown);
                        break;
                    default:
                        break;
                }
                break;
            case cmd::PowerDownAck::ID:
                if (ts_.state() == mkIV::DeviceState::State::PowerDown) {
                    if (! powerDownAck_) {
                        powerDownAck_ = true;
                        reloadRpWatchdogCounter(RCKID_POWERDOWN_EXTENDED_TIMEOUT_MS);
                    }
                }
                break;
            case cmd::PowerOn::ID:
                if (ts_.state() == mkIV::DeviceState::State::Alarm)
                    setState(mkIV::DeviceState::State::On);
                break;
            case cmd::RebootRP::ID:
                if (ts_.state() == mkIV::DeviceState::State::On)
                    reboot();
                break;
            case cmd::BootloaderRP::ID:
                if (ts_.state() == mkIV::DeviceState::State::On)
                    rebootToBootloader();
                break;
            case cmd::SetAlarm::ID:
                UNIMPLEMENTED;
                break;
            case cmd::SetTime::ID:
                UNIMPLEMENTED;
                //ts_.time = cmd::SetTime::fromBuffer(ts_.buffer).value;
                // TODO 
                break;


        }
        i2cRxBytes_ = 0;
    }

    static void i2cReset() {
        LL_I2C_EnableReset(I2C1);
        LL_I2C_DisableReset(I2C1);
        LL_I2C_ConfigSpeed(I2C1, SystemCoreClock, RCKID_I2C_SPEED, LL_I2C_DUTYCYCLE_2);
        LL_I2C_SetOwnAddress1(I2C1, RCKID_AVR_I2C_ADDRESS << 1, 0);
        LL_I2C_EnableClockStretching(I2C1);
        LL_I2C_EnableIT_EVT(I2C1);
        LL_I2C_EnableIT_ERR(I2C1);
        LL_I2C_Enable(I2C1);
        // ACK is cleared by hardware while PE=0, so must be set after enabling
        LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_ACK);
    }

    /** Stops the DMA transfer, if any. If the DMA was enabled and receiving, a master write has just finished, so the number of received bytes is stored in i2cRxBytes_ for the main loop (0 bytes is just an I2C ping). Checking the enabled flag ensures stale DMA configuration from previous transactions is ignored.
     */
    static void i2cStopDMA() {
        if (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_1) && LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_CHANNEL_1) == LL_DMA_DIRECTION_PERIPH_TO_MEMORY) {
            ASSERT(i2cRxBytes_ == 0);
            i2cRxBytes_ = sizeof(ts_.buffer) - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_1);
        }
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
        // single DMAEN bit for both directions
        LL_I2C_DisableDMAReq_RX(I2C1);
    }

    /** Called by the DMA IRQ when complete, i.e. we have command buffer overrun and should nack everything master sends afterwards.
     */
    static void i2cDMAComplete() {
        // if we were receiving data, NACK anything else master sends
        if (LL_DMA_GetDataTransferDirection(DMA1, LL_DMA_CHANNEL_1) == LL_DMA_DIRECTION_PERIPH_TO_MEMORY)
            LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_NACK);
    }

    static void i2cIRQHandler() {
        // SR2 is read before SR1 so that we get the direction without clearing ADDR (cleared by reading SR1 followed by SR2), which keeps SCL stretched while we set up the DMA
        uint32_t sr2 = I2C1->SR2;
        uint32_t sr1 = I2C1->SR1;
        // if there are any errors, abandon whatever transaction is in progress and reset the I2C (disabling the DMA first discards any partially received command)
        if (sr1 & (I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_OVR)) {
            LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
            i2cStopDMA();
            i2cReset();
            return;
        }
        // address match, start of new transaction. Repeated start does not generate STOPF, so stop the previous transfer first (registering the command if it was a write), as the DMA can't be reconfigured while enabled
        if (sr1 & I2C_SR1_ADDR) {
            i2cStopDMA();
            // master read, initialize the DMA to send the bytes from the transferrable state, looping if master reads more
            if (sr2 & I2C_SR2_TRA) {
                LL_SYSCFG_SetDMARemap_CH1(LL_SYSCFG_DMA_MAP_I2C_TX);
                LL_DMA_ConfigTransfer(DMA1, LL_DMA_CHANNEL_1,
                    LL_DMA_DIRECTION_MEMORY_TO_PERIPH |
                    LL_DMA_MODE_CIRCULAR |
                    LL_DMA_PERIPH_NOINCREMENT |
                    LL_DMA_MEMORY_INCREMENT |
                    LL_DMA_PDATAALIGN_BYTE |
                    LL_DMA_MDATAALIGN_BYTE |
                    LL_DMA_PRIORITY_LOW
                );
                LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_1, reinterpret_cast<uint32_t>(& ts_));
                LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, sizeof(ts_));
                LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);
                LL_I2C_EnableDMAReq_RX(I2C1);
            // master write, NACK if we are already processing a command, otherwise initialize the DMA to receive the bytes into the command buffer
            } else if (i2cRxBytes_ != 0) {
                LL_I2C_AcknowledgeNextData(I2C1, LL_I2C_NACK);
            } else {
                LL_SYSCFG_SetDMARemap_CH1(LL_SYSCFG_DMA_MAP_I2C_RX);
                LL_DMA_ConfigTransfer(DMA1, LL_DMA_CHANNEL_1,
                    LL_DMA_DIRECTION_PERIPH_TO_MEMORY |
                    LL_DMA_PERIPH_NOINCREMENT |
                    LL_DMA_MEMORY_INCREMENT |
                    LL_DMA_PDATAALIGN_BYTE |
                    LL_DMA_MDATAALIGN_BYTE |
                    LL_DMA_PRIORITY_LOW
                );
                LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_1, reinterpret_cast<uint32_t>(ts_.buffer));
                LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, sizeof(ts_.buffer));
                LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);
                LL_I2C_EnableDMAReq_RX(I2C1);
            }
            // clears ADDR as SR1 has already been read, releasing SCL
            (void) I2C1->SR2;
        }
        // master read finished, master nacked the last byte it wanted, i.e. master read is finished 
        if (sr1 & I2C_SR1_AF) {
            LL_I2C_ClearFlag_AF(I2C1);
            i2cStopDMA();
            // reset the I2C peripheral for the next transaction (also gets rid of any stale byte in the DR)
            i2cReset();
            // if we are in the on state, reading a state resets the watchdog timeout automatically
            if (ts_.state() == mkIV::DeviceState::State::On)
                reloadRpWatchdogCounter(RCKID_RP2350_WATCHDOG_TIMEOUT_MS);
        }
        // master write finished, master sent STOP. Note how many bytes we received
        if (sr1 & I2C_SR1_STOPF) {
            LL_I2C_ClearFlag_STOP(I2C1);
            i2cStopDMA();
            // reset the I2C peripheral for the next transaction (also gets rid of any NACKed byte in the DR)
            i2cReset();
        }
    }
    //@}

    /** \name Buttons 

        The button matrix is 3 groups of 4 buttons. Namely the BTN_CTRL group selects the Home button and volume up & down side buttons, the BTN_ABXY group selects the A, B and Select & Start buttons and the BTN_DPAD group selects the dpad. 

        When idle, all button pins are in input mode, pulled up. Button groups all output logical high. When a button group is selected, only that group's pin goes to logical 0. We then read the four buttons and reading 0 means the button is pressed (i.e. connected to the button group). The diodes from buttons to group pins ensure that no ghosting happens (i.e. low from one button would through different group enter other button here.

        All button pins are digital and are read as part of system tick. 
     */
    //@{
    static inline uint32_t homeBtnLongPressCounter_ = 0;
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
        homeBtnLongPressCounter_ = 0;
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

        // if we are in debug mode, volume up & down button presses are interpreted as reboot and reboot to bootloader respctively
        if (ts_.debugMode()) {
            ASSERT(ts_.state() == mkIV::DeviceState::State::On);
            if (! ts_.button(Btn::VolumeUp) && btnVolumeUp)
                reboot();
            if (! ts_.button(Btn::VolumeDown) && btnVolumeDown)
                rebootToBootloader();
        }

        // if the home button is released, reset the long press counter and if we are in wake state go to off/offDC immediately (it was a false wakeup)
        if (! btnHome) {
            homeBtnLongPressCounter_ = 0;
            if (ts_.state() == mkIV::DeviceState::State::Wake) {
                setState(ts_.dcPower() ? mkIV::DeviceState::State::OffDC : mkIV::DeviceState::State::Off);
                // return early so that we do not hit the assert that we are not off below when switching the button matrix for next group read
                if (ts_.state() == mkIV::DeviceState::State::Off)
                    return;
            }
        }

        bool changed = ts_.setButton(Btn::Home, btnHome);
    
        // if btnHome has changed to pressed, arm the long press detection counter (let the actual detection decide what to do with it)
        if (changed &&  btnHome) {
            homeBtnLongPressCounter_ = RCKID_HOME_BUTTON_LONG_PRESS_MS / (1000 / PUYA_SYSTEM_TICK_HZ);
            // if we are off, detecting home button press should wake us up
            if (ts_.state() == mkIV::DeviceState::State::OffDC)
                setState(mkIV::DeviceState::State::Wake);
        }

        changed = ts_.setButton(Btn::VolumeUp, btnVolumeUp) || changed;
        changed = ts_.setButton(Btn::VolumeDown, btnVolumeDown) || changed;

        // move to the next group immediately to maximize the time between group mode & button read, print state change id debugging enabled. Note that this function is only expected to be called when we are not powered down (i.e. we have running ticks)
        ASSERT(ts_.state() != mkIV::DeviceState::State::Off);
        gpio::high(PUYA_PIN_BTN_CTRL);
        gpio::low(PUYA_PIN_BTN_ABXY);

        if (changed)
            LOG("CTRL: " << btnHome << " " << btnVolumeUp << " " << btnVolumeDown);
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

        if (changed)
            LOG("ABXY: " << btnA << " " << btnB << " " << btnSelect << " " << btnStart);
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

        if (changed)
            LOG("DPAD: " << btnLeft << " " << btnRight << " " << btnUp << " " << btnDown);
    }

    static void btnHomeLongPress() {
        // if we are in the wakeup state, we should exit the wakeup state and enter the on state
        if (ts_.state() == mkIV::DeviceState::State::Wake || ts_.state() == mkIV::DeviceState::State::Alarm) {
            setState(mkIV::DeviceState::State::On);
            // set debug mode according to the state of the volume down
            setDebugMode(ts_.button(Btn::VolumeDown));
        } else if (ts_.state() == mkIV::DeviceState::State::On) {
            // if we are in the on state, we should enter the power down state
            setState(mkIV::DeviceState::State::PowerDown);
        }
    }

    //@}

    /** \name ADC (voltage and temperature)

        The ADC is used to measure battery voltage and temperature, alternating every second between the two. This should be a compromise between the latency of the measurements and power consumption. On the Puya chips the ADC has to be started for every conversion. 

        Because the ADC does a few busy waits in its setup for stabilization, etc. The ADC measurement is split into stages which are triggered in the system ticks. Every second the ADC is primed. Then the system ticks will pick up the prime information at tick 0, and will configure & calibrate in tick 0, enable in tick 1 and start the conversion in tick 2, using the tick spacing as ADC stabilization delays. The adcMeasure function then simply checks if we have ADC result ready and if so, processes it accordingly. 

        The temperature is measured from the device and stored in the transferrable state as a 16bit integer with values corresponding to temperature in 0.1 degress centigrade, i.e. value of 265 means 26.5 C. 

        The voltage is sampled via a 1:1 voltage divder, giving the maximum range 2x VCC, i.e. 6.6V. The voltage itself is encoded in 0.01 V, i.e. value of 330 means 3.3V. 
        
     */
    //@{

    static_assert(PUYA_PIN_VCC_SENSE == gpio::PB0, "VCC sense must be PB0 for the ADC to work");

    // we are using small values because the adc channel values are large integers on puya so those values are free

    /** Puya's supply voltage, which is also the ADC reference voltage, in mV.
     */
    static constexpr int32_t ADC_VREF_MV = 3300;

    static constexpr uint32_t ADC_MEASURE_VCC = 0;
    static constexpr uint32_t ADC_MEASURE_TEMP = 1;
    static constexpr uint32_t ADC_DONE = 2;
    static constexpr uint32_t ADC_OFF = 3;
    static constexpr uint32_t ADC_PRIME = 4;

    static inline uint32_t adcResult_[2];
    static inline volatile uint32_t adcResultIndex_ = ADC_OFF;

    static void initializeADC() {
        // VCC sense is an ADC input
        gpio::setAsAnalog(PUYA_PIN_VCC_SENSE);
        adc::initialize();
        LL_ADC_REG_SetSequencerChannels(ADC1, adc::pinToChannel(PUYA_PIN_VCC_SENSE) | LL_ADC_CHANNEL_TEMPSENSOR);
        adc::enableInterrupt();
        LOG("  adc");
    }

    static void disableADC() {
        adc::disableInterrupt();
        adc::disable();
        adcResultIndex_ = ADC_OFF;
    }

    static bool adcPrimed() {
        return adcResultIndex_ == ADC_PRIME;
    }

    static void adcPrime() {
        ASSERT(adcResultIndex_ == ADC_OFF);
        adcResultIndex_ = ADC_PRIME;
        adc::enable();
    }

    static void adcStartConversion() {
        ASSERT(adcResultIndex_ == ADC_PRIME);
        adcResultIndex_ = ADC_MEASURE_VCC;
        adc::startConversion();
    }

    static void adcMeasure() {
        if (adcResultIndex_ != ADC_DONE)
            return;
        // process the vcc
        {
            // 1:1 divider, so full scale (4095) is 2x VREF, in 0.01V, rounded
            uint16_t vcc = static_cast<uint16_t>((adcResult_[0] * (ADC_VREF_MV * 2 / 10) + 2047) / 4095);
            // TODO smoothing of the values is important
            // store in ts_
            ts_.setVcc(vcc);
            // depending on the value, we might need state changes, i.e. if the value is above the DC threshold and we are in Off, change to OffDC. If on the other hand we are in the OffDC and the value drops below the threshold, we should go to Off.
            if (vcc >= RCKID_VUSB_THRESHOLD && ! ts_.dcPower())
                dcPowerConnected(true);
            else if (vcc < RCKID_VUSB_THRESHOLD && ts_.dcPower())
                dcPowerConnected(false);
            if (vcc < RCKID_POWER_ON_THRESHOLD && ts_.state() == mkIV::DeviceState::State::On)
                criticalBattery();
        }
        // process the temperature
        {
            // the factory calibration values were taken with 3.3V reference, scale the reading to it first
            int32_t raw = static_cast<int32_t>(adcResult_[1]) * ADC_VREF_MV / static_cast<int32_t>(TEMPSENSOR_CAL_VREFANALOG);
            int32_t cal1 = *TEMPSENSOR_CAL1_ADDR;
            int32_t cal2 = *TEMPSENSOR_CAL2_ADDR;
            // linear interpolation between the two calibration points, in 0.1 C
            int16_t temp = static_cast<int16_t>(TEMPSENSOR_CAL1_TEMP * 10 + (raw - cal1) * (TEMPSENSOR_CAL2_TEMP - TEMPSENSOR_CAL1_TEMP) * 10 / (cal2 - cal1));
            // store in ts_
            ts_.setTemp(temp);
        }
        adcResultIndex_ = ADC_OFF;
    }
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
    friend void I2C1_IRQHandler();
    friend void DMA1_Channel1_IRQHandler();
    friend void EXTI0_1_IRQHandler();
    friend void EXTI2_3_IRQHandler();
    friend void ADC_COMP_IRQHandler();

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
            //KidDo::ts_.setAlarmInterrupt(true);
            // TODO code for device wakeup
        }
    }

    void I2C1_IRQHandler() {
        KidDo::i2cIRQHandler();
    }

    void DMA1_Channel1_IRQHandler() {
        if (LL_DMA_IsActiveFlag_TC1(DMA1)) {
            LL_DMA_ClearFlag_TC1(DMA1);
            KidDo::i2cDMAComplete();
        }
    }

    /** Charging interrupt. The only thing we have to do is switch the state to OffDC. The system ticks will start ADC sampler for the voltage, and continue monitoring the charging status and the dc voltage to determine the rest of the transitions and state changes. 
     */
    void EXTI0_1_IRQHandler() {
        if (LL_EXTI_IsActiveFlag(LL_EXTI_LINE_1)) {
            LL_EXTI_ClearFlag(LL_EXTI_LINE_1);
            if (KidDo::ts_.state() == mkIV::DeviceState::State::Off)
                KidDo::setState(mkIV::DeviceState::State::OffDC);
        }
    }

    /** Home button interrupt - fired when from high to low (button press), we set state to Wake. 
     */
    void EXTI2_3_IRQHandler() {
        if (LL_EXTI_IsActiveFlag(LL_EXTI_LINE_2)) {
            LL_EXTI_ClearFlag(LL_EXTI_LINE_2);
            KidDo::setState(mkIV::DeviceState::State::Wake);
        }
    }

    /** ADC result ready
     */
    void ADC_COMP_IRQHandler() {
        if (LL_ADC_IsActiveFlag_EOC(ADC1)) {
            LL_ADC_ClearFlag_EOC(ADC1);
            ASSERT(KidDo::adcResultIndex_ < 2);
            KidDo::adcResult_[KidDo::adcResultIndex_] = LL_ADC_REG_ReadConversionData12(ADC1);
            ++KidDo::adcResultIndex_;
        }
    }

}

int main() {
    KidDo::initialize();
    while (true)
        KidDo::loop();
}