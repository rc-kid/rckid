/** AVR Firmware

    The AVR is responsible for reading the inputs, managing the RGB LEDs and the rumbler and for basic power management. Note that the current AVR firmware is greatly simplified from the actual V2 design where the AVR was integral part of the device and was in fact in charge of the power management. For mk III, the role of AVR has been greatly diminished to a mere I2C slave. 

    The AVR does the following:

    - keeps the clock (RTC - this is done by an always on RTC module in mk III)
    - turns 3V3 on/off based on the 

 */

#include <avr/sleep.h>
#include <avr/delay.h>
#include <avr/interrupt.h>

#include <platform.h>
#include <platform/peripherals/neopixel.h>
#include <platform/tinydate.h>

#include <platform/ringavg.h>

#include "../../backend_config.h"
#include "../../../../rckid/common.h"
#include "commands.h"



using namespace rckid;


/** RCKid AVR firmware
 
    The AVR is always on as long as sufficient battery power is present and is responsible for the following:

    - power management (turning on, turning off, monitoring charging, RP reset & bootloading)
    - parts of user input (home, vol up and vol down keys)
    - real time clock (both uptime and configurable time)
    - rumbler & backlight PWM
    - RGB lights
    - audio on/off, headphones detection 

 */
class RCKid {
public:

    /** Initializes the AVR. 
     
        This is executed on power on or avr reset. First figure out why we did reset (wdt, power, sw), then initialize all subsystems and enter the normal mode. 

        On power on, we can assume chip is in a known state and all pins are in highZ state, which translates 5V and 3V3 VCC rails powered off, no interference with charging and no bleeding.
     */
    static void initialize() {
        // enable 2 second watchdog so that the second tick resets it always with enough time to spare
        while (WDT.STATUS & WDT_SYNCBUSY_bm); // required busy wait
            _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_2KCLK_gc);      
        // set CLK_PER prescaler to 2, i.e. 10Mhz, which is the maximum the chip supports at voltages as low as 3.0V
        CCP = CCP_IOREG_gc;
        CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm;
        // initialize the RTC that fires every second for a semi-accurate real time clock keeping on the AVR and start counting
        RTC.CLKSEL = RTC_CLKSEL_INT32K_gc; // run from the internal 32.768kHz oscillator
        RTC.PITINTCTRL |= RTC_PI_bm; // enable the PIT interrupt
        while (RTC.PITSTATUS & RTC_CTRLBUSY_bm);
        RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc | RTC_PITEN_bm;

        // set sleep mode to powerdown (we only need RTC and GPIO Interrupts)
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);

        // initialize basic AVR subsystems
        initializeInputs();
        initializePWM();

        // delay so that voltages stabilize and so on
        cpu::delayMs(100); 

        // determine why the reset


        platform::Color c = platform::Color::RGB(32, 32, 32);
        if (RSTCTRL.RSTFR & RSTCTRL_WDRF_bm)
            c = platform::Color::RGB(32, 0, 0);
        if (RSTCTRL.RSTFR & RSTCTRL_BORF_bm)
            c = platform::Color::RGB(0, 0, 32);

        rgbOn();
        rgbs_[0] = c;
        rgbs_[1] = c;
        rgbs_[3] = c;
        rgbs_[4] = c;
        rgbs_[5] = c;
        rgbs_.update();
        cpu::delayMs(100);
        rgbOff();

        cpu::delayMs(100);

        // set the AVR state to sleep (to enforce full wakeup) and then go to power on mode immediately
        avrState_ = AVRState::Sleep;
        devicePowerOn();
    }

    /** The main loop implementation (including the loop). 
     */
    static void loop() {
        while (true) {
            cpu::wdtReset();
            // system tick for basic bookkeeping (1ms)
            systemTick();
            // check any user inputs
            inputsTick();
            // rgb tick for RGB animations (60 fps)
            rgbTick();
            // rumbler tick (60 fps)
            rumblerTick();
            // see if there were any I2C commands received and if so, execute
            processI2CCommand();
            // if we are not on, and there is an alarm, we should power on 
            if (avrState_ != AVRState::On && ts_.status.alarm())
                devicePowerOn();
            // sleep if we should, if not sleeping check if we have ADC measurement ready
            if (avrState_ == AVRState::Sleep) {
                measureVccInSleep();
                sleep_enable();
                sleep_cpu();
            } else {
                measureADC();
            }
        }
    }

    static inline volatile uint8_t systemTicksCountdown_;

    /** Ensures that the system ticks, i.e. TCA0 are active. */
    static void startSystemTicks() {
        if (TCA0.SINGLE.CTRLA & TCA_SINGLE_ENABLE_bm)
            return;
        TCA0.SINGLE.CTRLD = 0;
        TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
        TCA0.SINGLE.PER = 125;
        TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;
        //TCA0.SPLIT.LPER = 8; // ~1ms for the system tick
        //TCA0.SPLIT.HPER = 130; // ~ 60 fps for the RGB tick
        //TCA0.SPLIT.CTRLC = 0;
        //TCA0.SPLIT.CTRLB = 0;
        //TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV1024_gc | TCA_SPLIT_ENABLE_bm;        
        systemTicksCountdown_ = 60;
    }

    /** Ensures that the system tick timer (TCA0) is not active.
     */
    static void stopSystemTicks() {
        // no harm disabling multiple times
        TCA0.SINGLE.CTRLA = 0;
    }

    /** The system tick is only active when the device is powered on, powering on, or has DC voltage applied (i.e. the AVR is not sleeping). */
    static void systemTick() {
        if ((TCA0.SINGLE.INTFLAGS & TCA_SINGLE_OVF_bm) == 0)
            return;
        TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
        // input ticks happen at the same as system ticks
        inputsTick_ = true;
        // see if we should trigger the 60fps rgb & rumbler ticks
        if (--systemTicksCountdown_ == 0) {
            systemTicksCountdown_ = 60;
            rgbTick_ = true;
            rumblerTick_ = true;
        }
        // TODO do the system tick
    }

    /** Second tick happens inside IRQ. Simply increase uptime and the RTC. The second tick is called at all times and at all AVR states. It is also a periodic wakeup for the device when sleeping - but any other tasks (charge detection, alarms, etc.) are handled by the main loop instead. 
     */
    static void secondTick() __attribute__((always_inline)) {
        ++ts_.uptime;
        ts_.time.secondTick();
        // if we are sleeping, initiatethe ADC measurement for the VCC to determine if DC power has been connected
        if (avrState_ == AVRState::Sleep)
            vccMeasureTick_ = true;
        // check the alarm - if the alarm is set the main loop will wakeup
        if (ts_.time == ts.alarm)
            ts_.status.setAlarm(true);
    }

    /** \name Power Management
     
        This can be either sleep (the CPU is power off, periodically wakes up every second to increment RTC), in charging mode (when DC is present, the AVR is fully on and monitors charging, but RP power is off) and On, in which case the RP is powered on ans AVR is not sleeping. A separate mode - PowerOn - is entered when the home button is pressed from charging or sleep states (special mode is required because AVR is on and counts ticks as well as checks the VCC for terminating due to low battery level). 
     */
    //{

    /** Ring buffers with running average to prevent vcc and vbatt glitches. 
     */
    static inline RingAvg<uint8_t, 128> vcc_;
    static inline RingAvg<uint8_t, 128> vBatt_;

    /** State of the AVR MCU. 
    */
    enum class AVRState {
        Sleep, 
        Charging, 
        PoweringOn,
        On, 
    }; // AVRState

    static inline volatile AVRState avrState_ = AVRState::Sleep;

    /** Turns the device off. This means either going to sleep mode if running from batteries, or going to charging mode when powered from DC */
    static void devicePowerOff() {
        // turn off 3V3 if enabled
        if (avrState_ == AVRState::On)
            power3v3Off();
        // when powering off, we can go either Sleep, or Charging state, dependning on whether the device runs from battery, or from DC
        if (ts_.status.powerDC()) {
            avrState_ = AVRState::Charging;
        } else {
            enterSleep();
            avrState_ = AVRState::Sleep;
        }
        // clear user RGBs
        for (int i = 0; i < 6; ++i) {
            if (i == 2)
                continue;
            rgbEffects_[i] = RGBEffect::Off();
            rgbsTarget_[i] = platform::Color::Black();
            rgbs_[i] = platform::Color::Black();
        }
    }

    static void devicePoweringOn() {
        if (avrState_ == AVRState::Sleep)
            leaveSleep();
        avrState_ = AVRState::PoweringOn;
    }

    static void devicePowerOn() {
        if (avrState_ == AVRState::Sleep)
            leaveSleep();
        if (avrState_ != AVRState::On) {
            power3v3On();
            // and mark the state as On
            avrState_ = AVRState::On;
        }
    }

    static void enterSleep() {
        stopSystemTicks();
        // disable ADC
        ADC0.CTRLA = 0;
        // disable RGBs
        rgbOff();
        avrState_ = AVRState::Sleep;
    }

    /** Wakes up - enables  */
    static void leaveSleep() { // IRQ
        startSystemTicks();
        initializeADC();
        initializeInputs();
    }

    static void power3v3On() {
        cli();
        // enable 3V3
        gpio::outputHigh(AVR_PIN_3V3_ON);
        cpu::delayUs(1000);
        // release the SDA and SCL lines and make it look as a STOP condition
        gpio::outputFloat(AVR_PIN_SCL);
        cpu::delayUs(10);
        gpio::outputFloat(AVR_PIN_SDA);
        // re-enable I2C in slave mode
        i2c::initializeSlave(I2C_AVR_ADDRESS);
        TWI0.SCTRLA |= TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm;
        sei();
    }

    static void power3v3Off() {
        cli();
        // turn display off
        setBacklightPWM(0);
        // disable I2C
        i2c::disable();
        // pull SDA, then SCL low - START condition
        gpio::outputLow(AVR_PIN_SDA);
        cpu::delayUs(10);
        gpio::outputLow(AVR_PIN_SCL);
        // now disable the 3V3 power
        gpio::outputFloat(AVR_PIN_3V3_ON);
        // TODO turn RGBs off? 
        // TODO rumble off? 
        sei();
    }

    static void dcPowerPlugged() {
        // do nothing if we already have DC power
        if (ts_.status.powerDC())
            return;
        // set DC power on
        ts_.status.setPowerDC(true);
#ifdef RCKID_HAS_LIPO_CHARGER
        // if the device was asleep, wake up and enter charging mode, otherwise stay in the On state. 
        if (avrState_ == AVRState::Sleep) {
            leaveSleep();
            avrState_ = AVRState::Charging;
        }
        // TODO enable charging 
#endif
    }

    static void dcPowerUnplugged() {
        if (!ts_.status.powerDC())
            return;
        ts_.status.setPowerDC(false);
#ifdef RCKID_HAS_LIPO_CHARGER
        if (avrState_ == AVRState::Charging) 
            enterSleep();
#endif
    }

    /** Goes to the power off mode immediately while flashing some red colors. 
     
        Since this is called from the ADC0 done method, it will fire even when in the wakeup phase we detect a too low voltage. 
     */
    static void criticalBattery() {
        // turn off the 3v3 rail first
        power3v3Off();
        // flash the LEDs red three times
        rgbOn();
        for (int i = 0; i < 3; ++i) {
            rgbs_.fill(platform::Color::Red().withBrightness(RGB_LED_DEFAULT_BRIGHTNESS));
            rgbs_.update();
            cpu::delayMs(200);
            rgbs_.fill(platform::Color::Black());
            rgbs_.update();
            cpu::delayMs(200);
            cpu::wdtReset(); 
        }
        devicePowerOff();
    }

    /** Resets the device (not AVR). 
     */
    static void reset() {
        // power off 
        power3v3Off();
        rgbOn();
        rgbs_.fill(platform::Color::Black());
        // wait 1 second by and indicate this on the RGB leds 
        for (int i = 0; i < 6; ++i) {
            cpu::wdtReset();
            if (i == 2)
                continue;
            rgbs_[i] = platform::Color::Red().withBrightness(RGB_LED_DEFAULT_BRIGHTNESS);
            rgbs_.update();
            cpu::delayMs(200);
            cpu::wdtReset();
        }
        rgbs_.fill(platform::Color::Black());
        rgbs_.update();
        // power the device on 
        power3v3On();
    }

    /** Resets the device and enters the bootloader mode for the RP chip. 
     */
    static void resetBootloader() {
        power3v3Off();
        rgbOn();
        rgbs_.fill(platform::Color::Black());
        // pull QSPI_SS low to indicate bootloader
        gpio::outputLow(AVR_PIN_QSPI_SS);
        // wait 1 second by and indicate this on the RGB leds, se
        for (int i = 0; i < 6; ++i) {
            cpu::wdtReset();
            switch (i) {
                case 2:
                    continue;
                case 3:
                    power3v3On();
                    break;
            }
            rgbs_[i] = platform::Color::Green().withBrightness(RGB_LED_DEFAULT_BRIGHTNESS);
            rgbs_.update();
            cpu::delayMs(200);
            cpu::wdtReset();
        }
        // reset the QSPI_SS back to float
        gpio::outputFloat(AVR_PIN_QSPI_SS);
        // since we are in the bootloader mode now, indicate by breathing all keys in green
        rgbEffects_[0] = RGBEffect::Breathe(platform::Color::Green().withBrightness(RGB_LED_DEFAULT_BRIGHTNESS), 1);
        rgbEffects_[1] = RGBEffect::Breathe(platform::Color::Green().withBrightness(RGB_LED_DEFAULT_BRIGHTNESS), 1);
        rgbEffects_[3] = RGBEffect::Breathe(platform::Color::Green().withBrightness(RGB_LED_DEFAULT_BRIGHTNESS), 1);
        rgbEffects_[4] = RGBEffect::Breathe(platform::Color::Green().withBrightness(RGB_LED_DEFAULT_BRIGHTNESS), 1);
        rgbEffects_[5] = RGBEffect::Breathe(platform::Color::Green().withBrightness(RGB_LED_DEFAULT_BRIGHTNESS), 1);
        // and return - the RP is now in bootloader mode, so will not talk to the AVR via I2C, but we don't care and will keep the powered on mode anyways
    }


    static void measureVcc(uint16_t rawValue) {
        rawValue = 110 * 512 / rawValue;
        rawValue *= 2;
        // add the value to the ring buffer and set the ring buffer's current value to the transferrable state
        vcc_.addObservation(Status::voltageToRawStorage(rawValue));
        uint16_t value = Status::voltageFromRawStorage(vcc_.value());
        ts_.vcc = value;
        //debugShowNumber((uint8_t)avrState_);
        // update the dc power detection
        if (rawValue >= VOLTAGE_DC_POWER_THRESHOLD) {
            dcPowerPlugged();
        } else {
            dcPowerUnplugged();
            // if we are running on batteries, check if we should emit low voltage warning, or even turn the device off. Those are based off the VCC so that they work even if powered through USB-C cable as they are really a property of the chip voltage irrespective where it comes from
            if (avrState_ != AVRState::Sleep) {
                // only emit critical battery warning if we have accumulated enough vcc measurements
                if (vcc_.ready() && (value < VOLTAGE_CRITICAL_THRESHOLD))
                    criticalBattery();
            }
        }
        rgbUpdateSystemNotification();
    }

    static void measureVBatt(uint16_t vx100) {
        vBatt_.addObservation(Status::voltageToRawStorage(vx100));
        ts_.status.setVBatt(vBatt_.value());
    }

    static void measureTemp(int32_t rawValue) {
        ts_.status.setTemp(rawValue);
        // TODO if temperature is too large, cut of charging
    }
    //@}

    /** \name I2C communication interrupt handler. 
     
        The communication is rather simple - an I2C slave that when read from returns the state buffer and when written to, stores data in the state's comms buffer. The data will be interpreted as a command and performed after the stop condition is received.

        This mode simplifies the AVR part and prioritizes short communication burts for often needed data, while infrequent operations, such as full state and even EEPROM data reads take more time. 

        
     */
    //@{

    static inline TransferrableState ts_;

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
            TWI0.SDATA = ((uint8_t*) & ts_)[i2cTxIdx_];
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
            ++i2cTxIdx_;
            // TODO send nack when done sending all state
        // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
        } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
            ts_.buffer[i2cRxIdx_++] = TWI0.SDATA;
            //rgbs_[2] = platform::Color::Green().withBrightness(32);
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
        if (!i2cCommandReady_)
            return;
        // TODO process the commands
        switch (ts_.buffer[0]) {
            case cmd::Nop::ID:
                break;
            case cmd::PowerOff::ID:
                devicePowerOff();
                break;
            case cmd::ResetRP::ID:
                reset();
                break;
            case cmd::ResetAVR::ID:
                cpu::reset();
                // unreachable here
            case cmd::BootloaderRP::ID:
                resetBootloader(); 
                break;
            case cmd::BootloaderAVR::ID:
                // TODO
                break;
            case cmd::DebugModeOn::ID:
                ts_.extras.setDebugMode(false);
                break;
            case cmd::DebugModeOff::ID:
                ts_.extras.setDebugMode(true);
                break;
            case cmd::SetBrightness::ID: {
                uint8_t value = cmd::SetBrightness::fromBuffer(ts_.buffer).value;
                setBacklightPWM(value);
                ts_.brightness = value;
                break;
            }
            case cmd::SetTime::ID: {
                TinyDate t = cmd::SetTime::fromBuffer(ts_.buffer).value;
                ts_.time = t;
                break;
            }
            case cmd::SetAlarm::ID: {
                TinyDate t = cmd::SetAlarm::fromBuffer(ts_.buffer).value;
                ts_.alarm = t;
                break;
            }
            case cmd::ClearAlarm::ID: {
                ts_.alarm.clear();
                ts_.status.setAlarm(false);
                break;
            }
            case cmd::Rumbler::ID: {
                break; // TODO remove
                auto & c = cmd::Rumbler::fromBuffer(ts_.buffer);
                if (c.effect.cycles > 0 && c.effect.strength > 0) {
                    rumblerEffect_ = c.effect;
                    //--rumblerEffect_.cycles;
                    rumblerCurrent_ = rumblerEffect_;
                    rumblerCurrent_.timeOn = 0;
                    rumblerCurrent_.timeOff = 0;
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
                if (c.index == 2)
                    break;
                rgbEffects_[c.index] = c.effect;
                rgbOn();
                break;
            }
            case cmd::SetRGBEffects::ID: {
                auto & c = cmd::SetRGBEffects::fromBuffer(ts_.buffer);
                rgbEffects_[0] = c.b;
                rgbEffects_[1] = c.a;
                rgbEffects_[3] = c.dpad;
                rgbEffects_[4] = c.sel;
                rgbEffects_[5] = c.start;
                rgbOn();
                break;
            }
            default:
                // unknown command
                break;
        }
        cli();
        i2cRxIdx_ = 0;
        i2cCommandReady_ = false;
        sei();
    }

    //@}

    /** \name RGB LEDs
     
        The device has in 6 RGB LEDs in total - 5 under the top keys (DPad, A, B, Select and Start) and one above the display. The key RGBs are controlled by the RCKid apps, while the notification LED is controlled by the AVR firmware itself to display notifications & status. 

        The LEDs are powered from a 5V step-up generator that is turned off when the LEDs are not used to conserve power (each neopixel takes a bit more than 1mA even if not on at all).
     */
    //@{

    enum class SystemNotification : uint8_t {
        None, 
        User,
        BatteryWarning, 
        Charging,
        ChargingDone,
    };

    static inline volatile bool rgbOn_ = false;
    static inline volatile bool rgbTick_ = false;
    static inline volatile uint8_t rgbSecondTick_ = 60;
    static inline platform::NeopixelStrip<6> rgbs_{AVR_PIN_RGB}; 
    static inline platform::ColorStrip<6> rgbsTarget_;
    static inline RGBEffect rgbEffects_[6];

    static inline bool userNotification_ = false;
    static inline SystemNotification systemNotification_ = SystemNotification::None;


    static void rgbOn() {
        if (rgbOn_)
            return;
        gpio::outputHigh(AVR_PIN_5V_ON);
        gpio::setAsOutput(AVR_PIN_RGB);
        cpu::wdtReset();
        cpu::delayMs(100);
        rgbOn_ = true;
    }

    static void rgbOff() {
        if (!rgbOn_)
            return;
        rgbOn_ = false;
        gpio::outputFloat(AVR_PIN_RGB);
        gpio::outputFloat(AVR_PIN_5V_ON);
    }

    static void rgbTick() {
        if (!rgbTick_)
            return;
        rgbTick_ = false;
        if (!rgbOn_)
            return;
        // is there a second tick to process? 
        rgbSecondTick();
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
            rgbOff();
        else
            rgbs_.update(true);
    }

    static void rgbSecondTick() {
        if (--rgbSecondTick_ != 0)
            return;
        rgbSecondTick_ = 60;
        for (int i = 0; i < 6; ++i) {
            // see if the effect should end
            if (rgbEffects_[i].duration > 0) {
                if (--rgbEffects_[i].duration == 0) {
                    rgbEffects_[i].turnOff();
                }
            }
        }
    }

    static void rgbUpdateSystemNotification() {
        // highest priority - if we are running on battery and the battery level is low, show the low battery level warning
        if ((!ts_.status.powerDC()) && (ts_.vcc < VOLTAGE_WARNING_THRESHOLD) && vcc_.ready())
            return rgbSetSystemNotification(SystemNotification::BatteryWarning);
        // if we are running on DC power, display either charging done if not charging, or charging
        if (ts_.status.powerDC()) {     
            if (ts_.status.charging())
                return rgbSetSystemNotification(SystemNotification::Charging);
            else 
                return rgbSetSystemNotification(SystemNotification::ChargingDone);
        }
        // if there is user notification, now is the time
        if (userNotification_)
            return rgbSetSystemNotification(SystemNotification::User);
        // otherwise, there is no notification to show
        rgbSetSystemNotification(SystemNotification::None);
    }

    static void rgbSetSystemNotification(SystemNotification n) {
        if (systemNotification_ == n)
            return;
        systemNotification_ = n;
        switch (n)  {
            case SystemNotification::None:
                rgbEffects_[2] = RGBEffect::Off();
                return; // don't go to rgbOn (we might be the only reason for the RGBs to be on)
            case SystemNotification::User:
                rgbEffects_[2] = RGBEffect::Breathe(platform::Color::Yellow().withBrightness(RGB_LED_DEFAULT_BRIGHTNESS));
                break;
            case SystemNotification::BatteryWarning:
                rgbEffects_[2] = RGBEffect::Breathe(platform::Color::Red().withBrightness(RGB_LED_DEFAULT_BRIGHTNESS));
                break;
            case SystemNotification::Charging:
                rgbEffects_[2] = RGBEffect::Breathe(platform::Color::Blue().withBrightness(RGB_LED_DEFAULT_BRIGHTNESS));
                break;
            case SystemNotification::ChargingDone:
                rgbEffects_[2] = RGBEffect::Breathe(platform::Color::Green().withBrightness(RGB_LED_DEFAULT_BRIGHTNESS));
                break;
        }
        rgbOn();
    }

    //@}

    /** \name Rumbler & Backlight
     
        The PWM signals used for backlight and rumbler control are generated by the TCB0 and TCB1 respectively.

        Backlight is pulled low externally, setting the pin to 1 make the backlight work, hence the value is unchanged.  
     */
    //@{
    static inline RumblerEffect rumblerEffect_;
    static inline RumblerEffect rumblerCurrent_;
    static inline volatile bool rumblerTick_ = false;

    static void initializePWM() {
        // do not leak voltage and turn the pins as inputs
        static_assert(AVR_PIN_PWM_BACKLIGHT == A5); // TCB0 WO
        gpio::outputFloat(AVR_PIN_PWM_BACKLIGHT);
        TCB0.CTRLA = 0;
        TCB0.CTRLB = 0; 
        TCB0.CCMPL = 255;
        TCB0.CCMPH = 0; 
        static_assert(AVR_PIN_PWM_RUMBLER == A3); //TCB1 WO
        gpio::outputFloat(AVR_PIN_PWM_RUMBLER);
        TCB1.CTRLA = 0;
        TCB1.CTRLB = 0; 
        TCB1.CCMPL = 255;
        TCB1.CCMPH = 0; 
     }

    static void setBacklightPWM(uint8_t value) {
        if (value == 0) {
            TCB0.CTRLA = 0;
            TCB0.CTRLB = 0;
            gpio::outputFloat(AVR_PIN_PWM_BACKLIGHT);
            //allowSleepPowerDown(STANDBY_REQUIRED_BRIGHTNESS);
        } else if (value == 255) {
            TCB0.CTRLA = 0;
            TCB0.CTRLB = 0;
            gpio::outputHigh(AVR_PIN_PWM_BACKLIGHT);
            //allowSleepPowerDown(STANDBY_REQUIRED_BRIGHTNESS);
        } else {
            gpio::outputLow(AVR_PIN_PWM_BACKLIGHT);
            TCB0.CCMPH = value;
            TCB0.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
            TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm | TCB_RUNSTDBY_bm;
            //requireSleepStandby(STANDBY_REQUIRED_BRIGHTNESS);
        }
    }

    static void setRumblerPWM(uint8_t value) {
        if (value == 0) {
            TCB1.CTRLA = 0;
            TCB1.CTRLB = 0;
            gpio::outputFloat(AVR_PIN_PWM_RUMBLER);
            //allowSleepPowerDown(STANDBY_REQUIRED_RUMBLER);
        } else if (value == 255) {
            TCB1.CTRLA = 0;
            TCB1.CTRLB = 0;
            gpio::outputHigh(AVR_PIN_PWM_RUMBLER);
            //allowSleepPowerDown(STANDBY_REQUIRED_RUMBLER);
        } else {
            gpio::outputLow(AVR_PIN_PWM_RUMBLER);
            TCB1.CCMPH = value;
            TCB1.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
            TCB1.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm | TCB_RUNSTDBY_bm;
            //requireSleepStandby(STANDBY_REQUIRED_RUMBLER);
        }
    }

    static void rumblerTick() {
        if (!rumblerTick_)
            return;
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

    /** \name Analogue Measurements
     
        ADC0 is used to measure the analogue measurements - vcc, battery voltage and temperature.
     */
    //@{

    static inline volatile bool vccMeasureTick_ = false;
    static inline volatile bool vccMeasureReady_ = false;

    static void measureVccInSleep() {
        if (vccMeasureTick_) {
            vccMeasureTick_ = false;
            vccMeasureReady_ = false;
            // we'll be sleeping in standby mode until the ADC is finished
            set_sleep_mode(SLEEP_MODE_STANDBY); // to make sure the ADC runs
            // initialize the ADC to measure VCC
            initializeADC();
            // and enable the interrupt 
            ADC0.INTCTRL = ADC_RESRDY_bm;
        } else if (vccMeasureReady_) {
            vccMeasureReady_ = false;
            // go back to deep sleep - we are done wih the ADC for this second
            set_sleep_mode(SLEEP_MODE_PWR_DOWN);
            // get the value and turn ADC off
            uint16_t value = ADC0.RES / 32;
            ADC0.CTRLA = 0;
            // and run the mesurement logic
            measureVcc(value);
        }
    }

    static void initializeADC() {
        ADC0.CTRLA = 0;
        ADC0.INTCTRL = 0;
        // initialize ADC0 common properties without turning it on
        ADC0.CTRLB = ADC_SAMPNUM_ACC32_gc;
        ADC0.CTRLD = ADC_INITDLY_DLY32_gc;
        ADC0.SAMPCTRL = 31;
        // set voltage reference to 1v1 for temperature checking
        VREF.CTRLA &= ~ VREF_ADC0REFSEL_gm;
        VREF.CTRLA |= VREF_ADC0REFSEL_1V1_gc;
        // sample internal voltage reference using VDD for reference to determine VCC 
        ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
        ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;
        // start the ADC conversion
        ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc | ADC_RUNSTBY_bm;
        ADC0.COMMAND = ADC_STCONV_bm;
    }

    static void measureADC() {
        if ((ADC0.INTFLAGS & ADC_RESRDY_bm) == 0)
            return;
        // clear the flag
        ADC0.INTFLAGS = ADC_RESRDY_bm;
        uint16_t value = ADC0.RES / 32;
        uint8_t muxpos = ADC0.MUXPOS;
        // convert the raw measurement according to what we measured and prepare to measure the next one
        switch (muxpos) {
            case ADC_MUXPOS_INTREF_gc:
                // TODO this is V2 code, in V3 we have to measure VCC on real pin through ADC as the AVR chip is always running at 3V3
                measureVcc(value);
                // get ready for measuriong the battery voltage
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
                ADC0.MUXPOS = gpio::getADC0muxpos(AVR_PIN_VBATT);
                break;
            case gpio::getADC0muxpos(AVR_PIN_VBATT):
                value >>= 2; // go for 8bit precision, which should be enough
                value = (ts_.vcc / 2 * value)  / 128; 
                measureVBatt(value);
                // measure temp sense next
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_INTREF_gc | ADC_SAMPCAP_bm;
                ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;
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
                measureTemp(t);
                // sample internal voltage reference using VDD for reference to determine VCC next 
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
                ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;
                // TODO for V2 prepare for headphones instead, if audio is on
                break;
            }
            // TODO headphones in V2 share same pin, and are analog. This could change to digital in V3
            case gpio::getADC0muxpos(AVR_PIN_HEADPHONES):
                //ts_.state.setHeadphones(value < HEADPHONES_DETECTION_THRESHOLD);
                // sample internal voltage reference using VDD for reference to determine VCC next 
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
                ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;
                break;
        }
        // start the ADC conversion
        ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc | ADC_RUNSTBY_bm;
        ADC0.COMMAND = ADC_STCONV_bm;
    }
    //@}

    /** \name User Inputs
     
        TODO The user inputs are for version 2! 
     */
    //@{
    static inline volatile bool inputsTick_ = false;
    static inline volatile uint16_t btnHomeCounter_ = 0;
    static inline uint8_t inputsTicks_ = 0;

    static void initializeInputs() {
        // initialize the home button to input pullup and set its ISR
        gpio::setAsInputPullup(AVR_PIN_BTN_HOME);
        static_assert(AVR_PIN_BTN_HOME == B2); // otherwise the ISR won't work
        PORTB.PIN2CTRL |= PORT_ISC_FALLING_gc;


        // TODO this is for V2 - set the bank to vol up & down keys so that we can report on them as the V3 would 
        // pull all buttons up
        gpio::setAsInputPullup(AVR_PIN_BTN_1);
        gpio::setAsInputPullup(AVR_PIN_BTN_2);
        gpio::setAsInputPullup(AVR_PIN_BTN_3);
        gpio::setAsInputPullup(AVR_PIN_BTN_4);
        // force all button groups to high 
        gpio::outputHigh(AVR_PIN_BTN_DPAD);
        gpio::outputHigh(AVR_PIN_BTN_ABSELSTART);
        gpio::outputHigh(AVR_PIN_BTN_CTRL);
        // read the control row always
        gpio::low(AVR_PIN_BTN_CTRL);
        inputsTicks_ = 0;
    }

    /** When home button is pressed for the first time in a long press window check, enter the powerOn state if not powered on already and reset the long press counter. The rest is done by the main loop. 
     */
    static void btnHomeDown() __attribute__((always_inline)) { // IRQ
        if (btnHomeCounter_ == 0) {
            btnHomeCounter_ = BTN_HOME_LONG_PRESS_THRESHOLD;
            switch (avrState_) {
                case AVRState::Sleep:
                    leaveSleep();
                    avrState_ = AVRState::PoweringOn;
                    break;
                case AVRState::Charging:
                    avrState_ = AVRState::PoweringOn;
                    break;
                default:
                    break;
            }
        }
    }

    static void btnHomeTick() {
        // do nothing if we are not counting the long press
        if (btnHomeCounter_ == 0)
            return;
        // get the home button state and update the status accordingly
        bool state = ! gpio::read(AVR_PIN_BTN_HOME);
        ts_.status.setBtnHome(state);
        // if home button is pressed and long press countdown has been reached, either turn the device on if in PoweringOn state, or turn the device off if in On state
        if (state) {
            if (--btnHomeCounter_ == 0) {
                switch (avrState_) {
                    case AVRState::PoweringOn:
                        // see if we have also pressed the volume down key, which activates the debug mode
                        ts_.extras.setDebugMode(ts_.status.btnVolumeDown());
                        // and turn the device on 
                        devicePowerOn();
                        break;
                    case AVRState::On:
                        devicePowerOff();
                        break;
                    default:
                        // this should not happen
                        break;
                }
            }
        // otherwise, the home button has been released. This is no-op in On state, but if we are in the PoweringOn state, we should return back to either Sleep, or Charging based on the powerDC
        } else {
            if (avrState_ == AVRState::PoweringOn) {
                if (ts_.status.powerDC()) {
                    avrState_ = AVRState::Charging;
                } else {
                    enterSleep();
                    avrState_ = AVRState::Sleep;
                }
            } 
            // reset the long press counter
            btnHomeCounter_ = 0;
        }
    }

    static void inputsTick() {
        if (!inputsTick_)
            return;
        inputsTick_ = false;
        // check the home button long press separately
        btnHomeTick();


        inputsTicks_ = (inputsTicks_ + 1) % 3;


        switch (inputsTicks_) {
            case 0:
                // we are ready to read vol up & down, read, react and get ready to measure DPAD in next tick
                ts_.status.setVolumeKeys(!gpio::read(AVR_PIN_BTN_2), !gpio::read(AVR_PIN_BTN_3));
                // TODO do this only when in debug mode
                if (ts_.status.btnVolumeUp() && ts_.extras.debugMode()) {
                    reset();
                } else if (ts_.status.btnVolumeDown() && ts_.extras.debugMode()) {
                    resetBootloader();
                } else {
                    gpio::high(AVR_PIN_BTN_CTRL);
                    gpio::low(AVR_PIN_BTN_DPAD);
                }
                break;
            case 1:
                // we are ready to read dpad, update status and move to read A, B, Sel & Start in next tick
                ts_.status.setDPadKeys(
                    !gpio::read(AVR_PIN_BTN_2), // left
                    !gpio::read(AVR_PIN_BTN_4), // right
                    !gpio::read(AVR_PIN_BTN_1), // up
                    !gpio::read(AVR_PIN_BTN_3) // down
                );
                gpio::high(AVR_PIN_BTN_DPAD);
                gpio::low(AVR_PIN_BTN_ABSELSTART);
                break;
            case 2:
                // we are ready to read A B Sel Start, get ready to read vol up & down in next tick
                ts_.status.setABSelStartKeys(
                    !gpio::read(AVR_PIN_BTN_2), // a
                    !gpio::read(AVR_PIN_BTN_1), // b
                    !gpio::read(AVR_PIN_BTN_4), // sel
                    !gpio::read(AVR_PIN_BTN_3) // start
                );
                gpio::high(AVR_PIN_BTN_ABSELSTART);
                gpio::low(AVR_PIN_BTN_CTRL);
                break;
        }
    }

    //@}


    /** Displays an 8bit nunmber on the top plate buttons LEDs. 
     
        Since we only have 5 leds, two colors are used:
     */

    static void debugShowNumber(uint8_t value) {
        rgbOn();
        uint8_t i = RGB_LED_DEFAULT_BRIGHTNESS;
        rgbs_[1] = platform::Color::RGB((value & 1) ? i : 0, (value & 32) ? i : 0, 0);
        rgbs_[0] = platform::Color::RGB((value & 2) ? i : 0, (value & 64) ? i : 0, 0);
        rgbs_[3] = platform::Color::RGB((value & 4) ? i : 0, (value & 128) ? i : 0, 0);
        rgbs_[5] = platform::Color::RGB((value & 8) ? i : 0, 0, 0);
        rgbs_[4] = platform::Color::RGB((value & 16) ? i : 0, 0, 0);
        for (unsigned i = 0; i < 6; ++i) {
            if (i == 2)
                continue;
            rgbsTarget_[i] = rgbs_[i];
        }
        rgbs_.update();
    }


}; // class RCKid


/** Interrupt for a second tick from the RTC. We need the interrupt routine so that the CPU can wake up and increment the real time clock and uptime. 
 */
ISR(RTC_PIT_vect) {
    RTC.PITINTFLAGS = RTC_PI_bm; // clear the interrupt
    RCKid::secondTick();
}

/** I2C slave action.
 */
ISR(TWI0_TWIS_vect) {
    RCKid::i2cSlaveIRQHandler();
}

/** Home button press. This has to be handled by IRQ as pressing the button leaves the sleep state and starts the long press counter to power up the device.
 */
ISR(PORTB_PORT_vect) {
    static_assert(AVR_PIN_BTN_HOME == B2);
    VPORTB.INTFLAGS = (1 << 2);
    RCKid::btnHomeDown();
}

/** VCC measurement in sleep node is ready. 
 */
ISR(ADC0_RESRDY_vect) {
   ADC0.INTFLAGS = ADC_RESRDY_bm;
   RCKid::vccMeasureReady_ = true;
}


int main() {
    RCKid::initialize();
    RCKid::loop();
}
