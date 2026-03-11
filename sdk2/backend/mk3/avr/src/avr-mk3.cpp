/** AVR Firmware for RCKid
 
    This is the firmware for ATTiny3217 in RCKid mkIII, specifically SDK version 1.0 and up. The main difference from the previous mkIII versions is simpler execution model with fewer commands and firmware logic as per the SDK 1.0 refactoring.


 */
#include <avr/sleep.h>
#include <avr/delay.h>
#include <avr/interrupt.h>

#include <platform.h>
#include <platform/peripherals/neopixel.h>


#include "avr-commands.h"

/** \name Debugging support. 
 
    To ease firmware development, the AVR chip supports serial port (only TX) as alternate function of the AVR_INT pin. The functionality has to be enabled in the macro below as it is offby default. 
 */
//@{
inline Writer debugWrite() {
    return Writer(serial::write);
}
//@}


#define ASSERT(...) if (!(__VA_ARGS__)) { debugWrite() << "ASSERT " << __LINE__ << "\r\n"; }
#define UNREACHABLE do { debugWrite() << "Unreachable: " << __LINE__; while (true) {} } while (false)
#define UNIMPLEMENTED do { debugWrite() << "Unimplemented: " << __LINE__; while (true) {} } while (false)

#if RCKID_AVR_IS_SERIAL_TX
    #define LOG(...) debugWrite() << __VA_ARGS__ << "\r\n";
#else
    #define LOG(...)
#endif

using namespace rckid;

class RCKid {
public:

    static void initialize() {
        // initialize the AVR chip
        initializeAVR();
        initializeComms();
        // set date to something meaningful
        ts_.time.date.setYear(2026);
        ts_.time.date.setMonth(1);
        ts_.time.date.setDay(1);

        // TODO some initialization routine with checks, etc.
        LOG("\n\n\nSYSTEM RESET DETECTED (AVR): " << hex(RSTCTRL.RSTFR));

        // initialize the powerOff mode
        enableSystemTicks(false);
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);

        // turn the device on after powerup
        ts_.state.setDebugMode(false);
        setPowerMode(POWER_MODE_ON);
    }

    static void loop() {
        cpu::wdtReset();
        // do system tick, if system tick is active and tickCounter is 0, also do the effects tick, which is roughly at 66 frames per second
        if (systemTick() && tickCounter_ == 0) {
            rgbTick();
            rumblerTick();
        }
        secondTick();
        // see if there are ADC measurements to process
        measureADC();
        // see if there were any I2C commands received and if so, execute
        processI2CCommand();
        // process any interrupt requests
        processIntRequests();
#if RCKID_AVR_IS_SERIAL_TX
        // wait for any TX transmissions before going to sleep 
        serial::waitForTx();
#endif
        // After everything was processed, go to sleep - we will wake up with the ACCEL or PMIC interrupts, or if in power off mode also by the HOME button interrupt
        cpu::sei();
        sleep_enable();
        sleep_cpu();
    }

    /** Basic initialization of the AVR firmware. 
     
        Sets clock frequency, enables RTC (including interrupt) and in case debugging over serial wire is used, enables the serial out on AVR_IRQ pin as well.

        Exists as a separate function from initialize to allow basic AVR initialization for test programs, etc.
     */
    static void initializeAVR() {
        // enable 2 second watchdog so that the second tick resets it always with enough time to spare
        while (WDT.STATUS & WDT_SYNCBUSY_bm); // required busy wait
            _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_2KCLK_gc);      
        // set CLK_PER prescaler to 2, i.e. 10Mhz, which is the maximum the chip supports at voltages as low as 3.0V
        CCP = CCP_IOREG_gc;
        CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm;
        // enable external crystal oscillator for the RTC
        CCP = CCP_IOREG_gc;
        CLKCTRL.XOSC32KCTRLA = CLKCTRL_RUNSTDBY_bm | CLKCTRL_ENABLE_bm;
        // initialize the RTC that fires every second for a semi-accurate real time clock keeping on the AVR and start counting
        RTC.CLKSEL = RTC_CLKSEL_TOSC32K_gc; // run from the external 32.768kHz oscillator
        RTC.PITINTCTRL |= RTC_PI_bm; // enable the PIT interrupt
        while (RTC.PITSTATUS & RTC_CTRLBUSY_bm);
        RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc | RTC_PITEN_bm;

    #if RCKID_AVR_IS_SERIAL_TX
        // initializes the AVR TX pin for serial debugging
        serial::setAlternateLocation(true);
        serial::initializeTx(RCKID_AVR_SERIAL_SPEED);
    #endif

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

    static bool isPowerModeOff() { return powerMode_ == 0; }
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
            // When powering on, we need to enable the IOVDDD ocnverters to power the cartridge and RP2350. We also initialize the PWM for backlight & rumbler and enable the ADC in standby mode so that we can sleep while taking the measurements. Finally we need to reset the RGB effects as the RGB leds are under the apolication control when powered on
            case POWER_MODE_ON:
                // TODO does this have to be in NO_ISR?
                NO_ISR(
                    powerIOVDD(true);
                    enablePWM(true);
                    ADC0.CTRLA |= ADC_RUNSTBY_bm;
                );
                // clear home button long press, reset power off timeout
                homeBtnLongPress_ = 0;
                powerOffTimeout_ = 0;
                // if we are in debug mode at this point, re-enable it again now that we have powered the IOVDD rail. This will turn on the LEDs and set backlight as well 
                if (ts_.state.debugMode())
                    enterDebugMode();
                // reset the RGB effects
                rgbSetEffectForAll(RGBEffect::Off());
                // rumble to indicate power on
                setRumblerEffect(RCKID_RUMBLER_EFFECT_POWER_ON);
                // no wakeup in power on, obviously
                clearPowerMode(POWER_MODE_WAKEUP);
                break;
            // wakeup power mode - reset the home button long press detection and enable debug mode tentatively, which allows us to disable debug mode if we detect volume button not pressed in the wakeup power mode
            case POWER_MODE_WAKEUP:
                startHomeButtonLongPress();
                ts_.state.setDebugMode(true);
                break;
            // clear the critical battery flag when DC power is connected, flash RGB LEDs if the device is not on (when on, the RGBs are controlled by the applications). No need to update state (the VCC value alone determines the DC mode as it did when calling this function)
            case POWER_MODE_DC:
                criticalBattery_ = false;
                if (! (powerMode_ & POWER_MODE_ON)) 
                    rgbSetEffectForAll(RGBEffect::Breathe(platform::Color::Green().withBrightness(RCKID_RGB_BRIGHTNESS), RCKID_RGB_NOTIFICATION_SPEED));
                break;
            // update the state to indicate charging, flash the RGBs if not powered on
            case POWER_MODE_CHARGING:
                ts_.state.setCharging(true);
                if (! (powerMode_ & POWER_MODE_ON)) 
                    rgbSetEffectForAll(RGBEffect::Breathe(platform::Color::Orange().withBrightness(RCKID_RGB_BRIGHTNESS), RCKID_RGB_NOTIFICATION_SPEED));
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
                NO_ISR(
                    powerIOVDD(false);
                    ts_.state.setDebugMode(false);
                    enablePWM(false);
                    // make the AVR_INT floating so that we do not leak any voltage to the now off RP2350
                    gpio::outputFloat(AVR_PIN_AVR_INT);
                    // clear IRQ
                    clearIrq();
                );

                // if we are turning off, set notification according to other power modes
                if (powerMode_ & POWER_MODE_CHARGING)
                    rgbSetEffectForAll(RGBEffect::Breathe(platform::Color::Orange().withBrightness(RCKID_RGB_BRIGHTNESS), RCKID_RGB_NOTIFICATION_SPEED));
                else if (powerMode_ & POWER_MODE_DC)
                    rgbSetEffectForAll(RGBEffect::Breathe(platform::Color::Green().withBrightness(RCKID_RGB_BRIGHTNESS), RCKID_RGB_NOTIFICATION_SPEED));
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
                if (! (powerMode_ & POWER_MODE_ON)) 
                    rgbSetEffectForAll(RGBEffect::Breathe(platform::Color::Green().withBrightness(RCKID_RGB_BRIGHTNESS), RCKID_RGB_NOTIFICATION_SPEED));
                break;
            // no action otherwise
            default:
                break;  
        }
        // if we have transitioned to complete off, stop system ticks and set sleep mode to power down
        if (powerMode_ == 0) {
            LOG("systick stop, sleep pwrdown");
            rgbSetEffectForAll(RGBEffect::Off());
            enableSystemTicks(false);
            set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        }
    }

    /** Turns the IOVDD on or off. 
     
        Because the IOVDD powers the pullups on the I2C lines, we want to ensure the always on devices connected to the bus (mainly the accelerometer) will see defined state by issuing START condition immediately before power off, followed by shorting the SDA and SCL lines to GND and by issuing STOP condition and releasing the I2C lanes rigfht after power on.
     */
    static void powerIOVDD(bool enable) {
        if (enable) {
            gpio::outputFloat(AVR_PIN_AVR_INT);
            gpio::outputHigh(AVR_PIN_IOVDD_EN);
            // wait a bit and then release the I2C pins by issuing STOP condition
            cpu::delayMs(5);
            gpio::setAsInput(AVR_PIN_I2C_SCL);
            cpu::delayUs(100);
            gpio::setAsInput(AVR_PIN_I2C_SDA);
            // start the I2C slave as we will be contacted by the RP2350 shortly
            i2c::initializeSlave(RCKID_AVR_I2C_ADDRESS);
            TWI0.SCTRLA |= TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm;
            LOG("IOVDD on");
        } else {
#if !RCKID_AVR_IS_SERIAL_TX            
            // clear the interrupt line to ensure we are not bleeding voltage through it
            gpio::outputFloat(AVR_PIN_AVR_INT);
#endif
            // capture I2C lines and issue START condition so that the accelerometer's I2C does not float
            i2c::disable();
            TWI0.SCTRLA = 0;
            gpio::outputLow(AVR_PIN_I2C_SDA);
            cpu::delayUs(100);
            gpio::outputLow(AVR_PIN_I2C_SCL);
            // drive the pin low to ensure the regulator turns off immediately
            gpio::outputLow(AVR_PIN_IOVDD_EN);
            LOG("IOVDD off");
        }
    }

    static void rebootRP() {
        LOG("RP reboot...");
        NO_ISR(
            powerIOVDD(false);
            // do RGB red countdown effect in a busy loop to give the voltages time to settle, the countdown lasts for approximatekly 1 second
            rgbOn(true);
            rgbClear();
            for (unsigned i = 0; i < 5; ++i) {
                cpu::wdtReset();
                switch (i) {
                    case 0:
                        rgb_[0] = platform::Color::Cyan().withBrightness(RCKID_RGB_BRIGHTNESS);
                        rgb_[1] = platform::Color::Cyan().withBrightness(RCKID_RGB_BRIGHTNESS);
                        rgb_[2] = platform::Color::Cyan().withBrightness(RCKID_RGB_BRIGHTNESS);
                        rgb_[3] = platform::Color::Cyan().withBrightness(RCKID_RGB_BRIGHTNESS);
                        break;
                    case 1:
                        rgb_[RGB_LED_BTN_B] = platform::Color::Cyan().withBrightness(RCKID_RGB_BRIGHTNESS);
                        break;
                    case 2:
                        rgb_[RGB_LED_BTN_A] = platform::Color::Cyan().withBrightness(RCKID_RGB_BRIGHTNESS);
                        break;
                    case 3:
                        rgb_[RGB_LED_BTN_SELECT] = platform::Color::Cyan().withBrightness(RCKID_RGB_BRIGHTNESS);
                        break;
                    default:
                        rgb_[RGB_LED_BTN_START] = platform::Color::Cyan().withBrightness(RCKID_RGB_BRIGHTNESS);
                        break;
                }
                rgb_.update();
                cpu::delayMs(200);
            }
            rgbClear();
            rgb_.update();
            powerIOVDD(true);
        );
    }

    /** Reboots the RP2350 chip into bootloader mode. 
     
        This is done by pulling the QSPI_CS pin low from the AVR during IOVDD power cycle, which signals to the RP to enter bootloader. Signalled by green lights, while in bootloader mode, all LEDs flash with green.
        
        NOTE that as the QSPI_CS pin is available on the cartridge as well it is technically possible to implement a real HW bootloader button on the cartridge itself similar to the RP Pico boards, but the AVR controlled reset & pull down of the QSPI line is more user friendly.
     */
    static void bootloaderRP() {
        LOG("RP bootloader...");
        NO_ISR(
            powerIOVDD(false);
            // pull QSPI_SS low to indicate bootloader
            gpio::outputLow(AVR_PIN_QSPI_SS);
            // reset the RGB LEDs
            rgbOn(true);
            rgbClear();
            // do a one second countdown with enabling power to the VDD rail in the middle so that the QSPI_CS low can be picked up
            for (unsigned i = 0; i < 5; ++i) {
                cpu::wdtReset();
                switch (i) {
                    case 0:
                        rgb_[0] = platform::Color::Blue().withBrightness(RCKID_RGB_BRIGHTNESS);
                        rgb_[1] = platform::Color::Blue().withBrightness(RCKID_RGB_BRIGHTNESS);
                        rgb_[2] = platform::Color::Blue().withBrightness(RCKID_RGB_BRIGHTNESS);
                        rgb_[3] = platform::Color::Blue().withBrightness(RCKID_RGB_BRIGHTNESS);
                        break;
                    case 1:
                        rgb_[RGB_LED_BTN_B] = platform::Color::Blue().withBrightness(RCKID_RGB_BRIGHTNESS);
                        break;
                    case 2:
                        rgb_[RGB_LED_BTN_A] = platform::Color::Blue().withBrightness(RCKID_RGB_BRIGHTNESS);
                        break;
                    case 3:
                        rgb_[RGB_LED_BTN_SELECT] = platform::Color::Blue().withBrightness(RCKID_RGB_BRIGHTNESS);
                        break;
                    default:
                        rgb_[RGB_LED_BTN_START] = platform::Color::Blue().withBrightness(RCKID_RGB_BRIGHTNESS);
                        break;
                }
                rgb_.update();
                if (i == 2)
                    powerIOVDD(true);
                cpu::delayMs(300);
            }
            // reset the QSPI_SS back to float
            gpio::outputFloat(AVR_PIN_QSPI_SS);
            // since we are in the bootloader mode now, indicate by breathing all keys in green
            rgbSetEffectForAll(RGBEffect::Breathe(platform::Color::Blue().withBrightness(RCKID_RGB_BRIGHTNESS), 7));
        );
    }

    static void enterDebugMode() {
        LOG("Debug mode on");
        ts_.state.setDebugMode(true);
        if (isPowerModeOn()) {
            setBacklightPWM(RCKID_DEFAULT_BRIGHTNESS);
            rgbSetEffectForAll(RGBEffect::Breathe(platform::Color::Purple().withBrightness(RCKID_RGB_BRIGHTNESS), RCKID_RGB_NOTIFICATION_SPEED));
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
            // no harm disabling multiple times and makes the code safer
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
        // check the power off timeout, if we are turning off, do not count it as a system tick and do not check the buttons
        if (powerOffTimeout_ > 0 && --powerOffTimeout_ == 0) {
            LOG("Power off timeout expired, powering off...");
            clearPowerMode(POWER_MODE_ON);
            return false;
        }
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
        if (ts_.wakeupCounter > 0 && --ts_.wakeupCounter == 0) {
            NO_ISR(
                ts_.state.setWakeUpInterrupt(true);
                setIrq();
            );
            ts_.state.setDebugMode(false);
            setPowerMode(POWER_MODE_ON);
        }
    }
    //@}

    /** \name I2C Communication
     
        - maybe have transferrable state that returns state, date time, all of user bytes
        - reset at the end of tx to state_ 
        - then have command sthat do things 
     */
    //@{

    static inline volatile bool i2cCommandReady_ = false;
    static inline uint8_t const * i2cTxAddr_ = nullptr;
    static inline uint8_t i2cTxBytes_ = 0;
    static inline uint8_t i2cRxBytes_ = 0;
    static inline uint8_t i2cBuffer_[16];

    static void initializeComms() {
        // make sure we'll start reading the transferrable state first
        i2cTxAddr_ = reinterpret_cast<uint8_t *>(& ts_);
        i2cRxBytes_ = 0;
        i2cTxBytes_ = 0;
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
            TWI0.SDATA = i2cTxAddr_[i2cTxBytes_++];
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
        // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
        } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
            i2cBuffer_[i2cRxBytes_++] = TWI0.SDATA;
            TWI0.SCTRLB = (i2cRxBytes_ == sizeof(i2cBuffer_)) ? TWI_SCMD_COMPTRANS_gc : TWI_SCMD_RESPONSE_gc;
        // master requests slave to write data, reset the sent bytes counter, initialize the actual read address from the read start and reset the IRQ
        } else if ((status & I2C_START_MASK) == I2C_START_TX) {
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc + TWI_SCMD_RESPONSE_gc;
            i2cTxBytes_ = 0;
        // master requests to write data itself. ACK if there is no pending I2C message, NACK otherwise. The buffer is reset to 
        } else if ((status & I2C_START_MASK) == I2C_START_RX) {
            TWI0.SCTRLB = (! i2cCommandReady_) ? TWI_SCMD_RESPONSE_gc : TWI_ACKACT_NACK_gc;
        // sending finished, reset the tx address and when in recording mode determine if more data is available
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_TX) {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            i2cTxAddr_ = reinterpret_cast<uint8_t *>(& ts_);
        // receiving finished, inform main loop we have message waiting if we have received at laast one byte (0 bytes received is just I2C ping)
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_RX) {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            if (i2cRxBytes_ > 0)
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
                i2cTxAddr_ = reinterpret_cast<uint8_t *>(& ts_);
                break;
            case cmd::GetVersion::ID:
                i2cTxAddr_ = reinterpret_cast<uint8_t const *>(& ts_.version);
                break;
            case cmd::GetTime::ID:
                i2cTxAddr_ = reinterpret_cast<uint8_t *>(& ts_.time);
                break;
            case cmd::SetTime::ID:
                ts_.time = cmd::SetTime::fromBuffer(i2cBuffer_).value;
                break;
            case cmd::ReadStorage::ID:
                i2cTxAddr_ = ts_.storage + cmd::ReadStorage::fromBuffer(i2cBuffer_).offset;
                break;
            case cmd::WriteStorage::ID: {
                auto & c = cmd::WriteStorage::fromBuffer(i2cBuffer_);
                uint8_t numBytes = i2cRxBytes_ - sizeof(c.offset) - 1;
                if (c.offset + numBytes <= sizeof(ts_.storage))
                    memcpy(ts_.storage + c.offset, c.data, i2cRxBytes_ - sizeof(c.offset) - 1);
                else 
                    // since we have no error notion mechanism, just go unreachable if the bounds check fails so that in product builds it will just be ignored
                    UNREACHABLE;
                break;
            }
            case cmd::ReadEEPROM::ID:
                UNIMPLEMENTED;
                break;
            case cmd::WriteEEPROM::ID:
                UNIMPLEMENTED;
                break;
            case cmd::PowerOff::ID:
                clearPowerMode(POWER_MODE_ON);
                break;
            case cmd::PowerOffAck::ID:
                // reset the power off timeout to give the RP2350 more time to shut down gracefully before we cut the power
                powerOffTimeout_ = RCKID_POWEROFF_TIMEOUT_FPS;
                break;
            case cmd::Sleep::ID:
                UNIMPLEMENTED;
                break;
            case cmd::WakeUp::ID: {
                auto & c = cmd::WakeUp::fromBuffer(i2cBuffer_);
                ts_.wakeupCounter = c.countdownSeconds;
                ts_.wakeupReason = c.reason;
                break;
            }
            case cmd::RebootRP::ID:
                rebootRP();
                break;
            case cmd::BootloaderRP::ID:
                bootloaderRP();
                break;
            case cmd::RebootAVR::ID:
                UNIMPLEMENTED;
                break;
            case cmd::BootloaderAVR::ID:
                UNIMPLEMENTED;
                break;
            case cmd::SetDebugMode::ID:
                if (cmd::SetDebugMode::fromBuffer(i2cBuffer_).value)
                    enterDebugMode();
                else
                    // TODO clear the debug more RGBs?
                    ts_.state.setDebugMode(false);
                break;
            case cmd::SetBrightness::ID:
                setBacklightPWM(cmd::SetBrightness::fromBuffer(i2cBuffer_).value);
                break;
            case cmd::SetRGBEffectAll::ID:
                rgbSetEffectForAll(cmd::SetRGBEffectAll::fromBuffer(i2cBuffer_).effect);
                break;
            case cmd::SetRGBEffect::ID: {
                auto & c = cmd::SetRGBEffect::fromBuffer(i2cBuffer_);
                rgbSetEffect(c.ledIndex, c.effect);
                break;
            }
            case cmd::SetRumblerEffect::ID: {
                auto & c = cmd::SetRumblerEffect::fromBuffer(i2cBuffer_);
                setRumblerEffect(c.effect);
                break;
            }
            default:
                // unknown command, ignore for now
                LOG("Unknown cmd: " << i2cBuffer_[0]);
                break;
        }
        // and reset the command state so that we can read more commands 
        NO_ISR(
            i2cRxBytes_ = 0;
            i2cCommandReady_ = false;
        );
    }
    //@}

    /** \name Interrupts
     
        TODO
     */
    //@{
    static constexpr uint8_t ACCEL_INT_REQUEST = 1;
    static constexpr uint8_t HOME_BTN_INT_REQUEST = 2;
    static constexpr uint8_t CHARGING_INT_REQUEST = 4;
    static inline volatile uint8_t intRequests_ = 0;

    static inline volatile bool irq_ = false;

    static void setIrq() {
        // atm we are not using the IRQ line, instead RP2350 polls periodically
    }

    static void clearIrq() {
        // atm we are not using the IRQ line, instead RP2350 polls periodically
    }

    static void processIntRequests() {
        if (intRequests_ == 0)
            return;
        uint8_t irqs;
        NO_ISR(
            irqs = intRequests_;
            intRequests_ = 0;
        );
        // if the accelerometer IRQ is on, we simply pass it to the RP2350 via the state. It's done here and not in the ISR routine in case in the future we want to do some processing of the accel data on AVR as well.
        if (irqs & ACCEL_INT_REQUEST) {
            if (isPowerModeOn()) {
                NO_ISR(
                    ts_.state.setAccelInterrupt(true);
                    setIrq();
                );
                // TODO should the interrupt be cleared for the accelerometer?
            }
        }
        // charging request can be serviced in all power modes. based on the charging pin status we set the power modes appropriately, leaving the power mode functions to deal with the actual status changes. If charging is enabled, then the DC mode must by definition be enabled as well and we ensure this by setting it on here. This is important because when in powerOff mode, we do not detect DC power directly via VCC measurements (they are ADC).
        if (irqs & CHARGING_INT_REQUEST) {
            bool charging = ! gpio::read(AVR_PIN_CHARGING);
            if (charging) {
                setPowerMode(POWER_MODE_DC);
                setPowerMode(POWER_MODE_CHARGING);
            } else {
                clearPowerMode(POWER_MODE_CHARGING);
            }
        }
        // finally the home button press, which can only happen if the device is in powered down state. If critical battery flag is on, we cannot power the device on and only display the critical battery warning flashes, exitting prematurely. Otherwise we enter the wakeup power mode, which starts the button long press detection to transition to power on state if successful.
        if (irqs & HOME_BTN_INT_REQUEST) {
            ASSERT(isPowerModeOff());
            if (criticalBattery_) {
                criticalBattery();
                return;
            }
            setPowerMode(POWER_MODE_WAKEUP);
        }
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
            setIrq();

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
        ASSERT(! isPowerModeOff());
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
        ASSERT(! isPowerModeOff());
        gpio::high(AVR_PIN_BTN_DPAD);
        gpio::low(AVR_PIN_BTN_CTRL);
        if (changed) {
            LOG("DPAD: " << ts_.state.button(Btn::Left) << " " << ts_.state.button(Btn::Right) << " " << ts_.state.button(Btn::Up) << " " << ts_.state.button(Btn::Down));
            setIrq();
        }
    }

    static void startHomeButtonLongPress() {
        homeBtnLongPress_ = RCKID_HOME_BUTTON_LONG_PRESS_FPS;
    }

    static void checkHomeButtonLongPress() {
        if (homeBtnLongPress_ == 0)
            return;
        if (! ts_.state.button(Btn::Home)) {
            homeBtnLongPress_ = 0;
            return;
        }
        if (--homeBtnLongPress_ != 0)
            return;
        // if we are in the wakeup mode, long press mens transition to power on mode, so set power mode to ON, disable wakeup mode. We also enter debug mode if that is the case
        if (isPowerModeWakeup()) {
            setPowerMode(POWER_MODE_ON);
        // otherwise, if we are in power on mode, long press means transition to power off, so here we simply set the power off interrupt and timeout. 
        } else if (isPowerModeOn()) {
            powerOffTimeout_ = RCKID_POWEROFF_ACK_TIMEOUT_FPS;
            ts_.state.setPowerOffInterrupt(true);
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


    static inline bool rgbOn_ = false;
    static inline RGBEffect rgbEffect_[RCKID_NUM_RGB_LEDS];
    static inline uint8_t rgbEffectTimeout_[RCKID_NUM_RGB_LEDS] = {0};
    static inline platform::ColorStrip<RCKID_NUM_RGB_LEDS> rgbTarget_;
    static inline platform::NeopixelStrip<RCKID_NUM_RGB_LEDS> rgb_{AVR_PIN_RGB};
    static inline uint8_t rgbTicks_ = RCKID_RGB_TICKS_PER_SECOND; 

    static void rgbOn(bool value) {
        if (rgbOn_ == value)
            return;
        if (value) {
            LOG("RGB on");
            gpio::outputHigh(AVR_PIN_5V_EN);
            gpio::setAsOutput(AVR_PIN_RGB);
        } else {
            LOG("RGB off");
            rgbClear();
            gpio::outputFloat(AVR_PIN_RGB);
            gpio::outputFloat(AVR_PIN_5V_EN);
        }
        rgbOn_ = value;
        rgbTicks_ = RCKID_RGB_TICKS_PER_SECOND;
    }

    /** Clears the RGBs - disables effects and sets the current and target color to black for immediate switch to black color.
     */
    static void rgbClear() {
        for (uint8_t i = 0; i < RCKID_NUM_RGB_LEDS; ++i) {
            rgbEffect_[i] = RGBEffect::Off();
            rgb_[i] = platform::Color::Black();
            rgbTarget_[i] = platform::Color::Black();
        }
        // and ensure the RGB state is propagated to the RGB LEDs
        if (rgbOn_)
            rgb_.update(true);        
    }

    static void rgbSetEffectForAll(RGBEffect effect) {
        for (uint8_t i = 0; i < RCKID_NUM_RGB_LEDS; ++i) {
            rgbEffect_[i] = effect;
        }
        if (effect.active())
            rgbOn(true);
    }

    static void rgbSetEffect(uint8_t led, RGBEffect effect) {
        ASSERT(led < RCKID_NUM_RGB_LEDS);
        rgbEffect_[led] = effect;
        if (effect.active())
            rgbOn(true);
    }

    static void rgbTick() {
        if (!rgbOn_)
            return;
        // if there has been second already, decrement effect durations
        if (--rgbTicks_ == 0) {
            rgbTicks_ = RCKID_RGB_TICKS_PER_SECOND;
            for (int i = 0; i < RCKID_NUM_RGB_LEDS; ++i) {
                // see if the effect should end
                if (rgbEffect_[i].duration > 0) {
                    if (--rgbEffect_[i].duration == 0) {
                        rgbEffect_[i].turnOff();
                    }
                }
            }
        }
        // for all LEDs, move them towards their target at speed given by their effect
        bool turnOff = true;
        for (int i = 0; i < RCKID_NUM_RGB_LEDS; ++i) {
            if (rgbEffectTimeout_[i] > 0) {
                --rgbEffectTimeout_[i];
                turnOff = false;
                continue;
            }
            rgbEffectTimeout_[i] = rgbEffect_[i].skipTicks();
            bool done = ! rgb_[i].moveTowards(rgbTarget_[i], rgbEffect_[i].changeDelta());
            // if the current transition is done, make next effect transition
            if (done) {
                rgbTarget_[i] = rgbEffect_[i].nextColor(rgbTarget_[i]);
                if (rgbEffect_[i].active())
                    turnOff = false;
            } else {
                turnOff = false;
            }
        }
        // if all the LEDs are off, turn the 5V rail off to save power, otherwise update the LEDs
        if (turnOff)
            rgbOn(false);
        else
            rgb_.update(true);
    }

    static void criticalBattery() {
        criticalBattery_ = true;
        NO_ISR(
            rgbOn(true);
            cpu::delayMs(50);
            rgbClear();
            cpu::delayMs(100);
            for (uint8_t i = 0; i < 3; ++i) {
                cpu::wdtReset();
                for (uint8_t j = 0; j < RCKID_NUM_RGB_LEDS; ++j)
                    rgb_[j] = platform::Color::Red().withBrightness(RCKID_RGB_BRIGHTNESS);
                rgb_.update();
                cpu::delayMs(100);
                rgbClear();
                cpu::delayMs(200);
            }
            rgbOn(false);
        );
    }
    //@}


    /** \name PWM (Rumbler and Backlight)

        The PWM signals used for backlight and rumbler control are generated by the TCB0 and TCB1 respectively.

        Backlight is pulled low externally, setting the pin to 1 make the backlight work, hence the value is unchanged. By selecting TCA_CLK (from system tick) as clock source, we can run the PWM more effectively and save power.   
     */
    //@{
    static inline RumblerEffect rumblerEffect_;
    static inline RumblerEffect rumblerCurrent_;

    static void enablePWM(bool value) {
        if (value) {
            // do not leak voltage and turn the pins as inputs
            static_assert(AVR_PIN_PWM_BACKLIGHT == gpio::C0); // TCB0 WO, alternate position
            PORTMUX.CTRLD |= PORTMUX_TCB0_bm;
            gpio::outputFloat(AVR_PIN_PWM_BACKLIGHT);
            TCB0.CTRLA = 0;
            TCB0.CTRLB = 0; 
            TCB0.CCMPL = 255;
            TCB0.CCMPH = 0; 
            static_assert(AVR_PIN_PWM_RUMBLER == gpio::A3); //TCB1 WO
            gpio::outputFloat(AVR_PIN_PWM_RUMBLER);
            TCB1.CTRLA = 0;
            TCB1.CTRLB = 0; 
            TCB1.CCMPL = 255;
            TCB1.CCMPH = 0; 
            // both interrupts are allowed to run in standby mode
            TCB0.CTRLA |= TCB_RUNSTDBY_bm;
            TCB1.CTRLA |= TCB_RUNSTDBY_bm;
        } else {
            TCB0.CTRLA = 0;
            TCB0.CTRLB = 0;
            gpio::outputFloat(AVR_PIN_PWM_BACKLIGHT);
            TCB1.CTRLA = 0;
            TCB1.CTRLB = 0;
            gpio::outputFloat(AVR_PIN_PWM_RUMBLER);
        }
    }

    static void setBacklightPWM(uint8_t value) {
        if (value == 0) {
            TCB0.CTRLA = 0;
            TCB0.CTRLB = 0;
            gpio::outputFloat(AVR_PIN_PWM_BACKLIGHT);
        } else if (value == 255) {
            TCB0.CTRLA = 0;
            TCB0.CTRLB = 0;
            gpio::outputHigh(AVR_PIN_PWM_BACKLIGHT);
        } else {
            gpio::outputLow(AVR_PIN_PWM_BACKLIGHT);
            TCB0.CCMPH = value;
            TCB0.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
            //TCB0.CTRLA = TCB_CLKSEL_CLKTCA_gc | TCB_ENABLE_bm | TCB_RUNSTDBY_bm;
            TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm | TCB_RUNSTDBY_bm;
        }
    }

    static void setRumblerPWM(uint8_t value) {
        if (value == 0) {
            TCB1.CTRLA = 0;
            TCB1.CTRLB = 0;
            gpio::outputFloat(AVR_PIN_PWM_RUMBLER);
        } else if (value == 255) {
            TCB1.CTRLA = 0;
            TCB1.CTRLB = 0;
            gpio::outputHigh(AVR_PIN_PWM_RUMBLER);
        } else {
            gpio::outputLow(AVR_PIN_PWM_RUMBLER);
            TCB1.CCMPH = value;
            TCB1.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
            //TCB1.CTRLA = TCB_CLKSEL_CLKTCA_gc | TCB_ENABLE_bm | TCB_RUNSTDBY_bm;
            TCB1.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm | TCB_RUNSTDBY_bm;
        }
        LOG("rumbler on " << value);
    }

    static void setRumblerEffect(RumblerEffect effect) {
        if (effect.cycles > 0 && effect.strength > 0) {
            rumblerEffect_ = effect;
            rumblerCurrent_ = rumblerEffect_;
            rumblerCurrent_.timeOn = 0;
            rumblerCurrent_.timeOff = 0;
        } else {
            rumblerEffect_ = RumblerEffect::Off();
            rumblerCurrent_ = RumblerEffect::Off();
            setRumblerPWM(0);
        LOG("Rumbler off");
        }
    }

    static void rumblerTick() {
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

    static inline TransferrableState ts_;

}; // RCKid

/** Interrupt for a second tick from the RTC. We need the interrupt routine so that the CPU can wake up and increment the real time clock and uptime. 
 */
ISR(RTC_PIT_vect) {
    RTC.PITINTFLAGS = RTC_PI_bm; // clear the interrupt
    RCKid::secondTick_ = true;
}

/** RTC CNT interrupt which we use for overflow only for the system tick at ~5ms (200Hz).
 */
ISR(RTC_CNT_vect) {
    RTC.INTFLAGS = RTC_OVF_bm; // clear the interrupt
    RCKid::systemTick_ = true;
}

/** I2C slave action.
 */
ISR(TWI0_TWIS_vect) {
    RCKid::i2cSlaveIRQHandler();    
}

/** Accel pin ISR.
 */
ISR(PORTB_PORT_vect) {
    static_assert(AVR_PIN_ACCEL_INT == gpio::B4);
    VPORTB.INTFLAGS = (1 << GPIO_PIN_INDEX(AVR_PIN_ACCEL_INT));
    // only do stuff if we are transitioning from low to high
    if (gpio::read(AVR_PIN_ACCEL_INT))
        RCKid::intRequests_ |= RCKid::ACCEL_INT_REQUEST;
}

/** ADC measurement done. The interrupt is necessary to ensure that the AVR wakes up from sleep.
 */
ISR(ADC0_RESRDY_vect) {
    // don't change the flag, we use it, instead disable the interrupt
    ADC0.INTCTRL = 0;
    //ADC0.INTFLAGS = ADC_RESRDY_bm;
}

/** GPIO interrupts. 
 
    We use those when the device is powered off to ensure that we wake up when either charging starts (this is a way to detect DC mode enabled as well, but unlike the charging pin, we cannot simply add ISR to it as the change is not across the 0-1 threshold), or the home button is pressed. 
 */
ISR(PORTA_PORT_vect) {
    static_assert(AVR_PIN_BTN_1 == gpio::A6);
    static_assert(AVR_PIN_CHARGING == gpio::A5);
    if (VPORTA.INTFLAGS & (1 << GPIO_PIN_INDEX(AVR_PIN_BTN_1))) {
        VPORTA.INTFLAGS = (1 << GPIO_PIN_INDEX(AVR_PIN_BTN_1));
        // only do stuff if we were tranistioning from high to low (button press)
        if (gpio::read(AVR_PIN_BTN_1) == false)
            RCKid::intRequests_ |= RCKid::HOME_BTN_INT_REQUEST;
    }
    if (VPORTA.INTFLAGS & (1 << GPIO_PIN_INDEX(AVR_PIN_CHARGING))) {
        VPORTA.INTFLAGS = (1 << GPIO_PIN_INDEX(AVR_PIN_CHARGING));
        RCKid::intRequests_ |= RCKid::CHARGING_INT_REQUEST;
    }
}

/** Initialization and main loop, simply calls to the RCKid's class init & loop functions. 
 */
int main() {
    RCKid::initialize();
    while (true)
        RCKid::loop();
}
