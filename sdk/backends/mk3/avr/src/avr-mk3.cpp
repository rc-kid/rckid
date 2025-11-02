/** RCKid MK III AVR firmware
 
    The AVR chip reads user inputs, controls the pwm backlight and rumbler peripherals, serves as a RTC and monitors the power. As the chip is always on, it is also used as simple device-bound data storage across cartridges and power cycles (modulo battery changes). 

    The AVR operates in two modes - when the device is powered on, the AVR acts as an I2C slave and only responds to RP2350 commands.

 */

#include <avr/sleep.h>
#include <avr/delay.h>
#include <avr/interrupt.h>

#include <platform.h>
#include <platform/peripherals/neopixel.h>
#include <platform/writer.h>
#include <platform/tinydate.h>

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


// those have to be included after the debugWrite & LOG declaration above so that we can use RCKid's logging macros
#include <backend_config.h>
#include <backend_internals.h>
#include "avr_state.h"
#include "avr_commands.h"
#include <rckid/effects.h>


/** AVR Pinout 
 
    - I2C is routed to B0 and B1, their default position. 
    - PWM pins (rumbler and backlight) are routed to TCB0 (alternate) and TCB1 respectively.
    - button matrix pins are default digital pins. We need iterrupt on the home button (BTN_1 of BTN_CTRL group) to wake up when powered on
    - AVR_TX is alternate position of serial TxD and can be used for debugging the firmware. Its is also the only free pin.
    - 

    Powered On:
    
    - monitor buttons
    - update RGB LEDs
    - update rumbler
    - respond to I2C commands
    - control RP reboot & bootloading

    - everything is digital, so we only need the ADC for temperature...
    - use system tick of 5ms, this gives us ability to read all buttons every frame 

    NOTE: There is an errata for attiny1616 and smaller chips that states HW bug where turning off RTC turns off PIT as well, which means those chips will *not* work with RCKid as we use RTC for the system tick and PIT for the timekeeping.
 */

#define AVR_PIN_AVR_INT         gpio::A1
#define AVR_PIN_VCC_SENSE       gpio::A2
#define AVR_PIN_PWM_RUMBLER     gpio::A3
#define AVR_PIN_5V_EN           gpio::A4
#define AVR_PIN_CHARGING        gpio::A5
#define AVR_PIN_BTN_1           gpio::A6
#define AVR_PIN_BTN_3           gpio::A7
#define AVR_PIN_I2C_SCL         gpio::B0
#define AVR_PIN_I2C_SDA         gpio::B1
#define AVR_PIN_ACCEL_INT       gpio::B4
#define AVR_PIN_BTN_CTRL        gpio::B5
#define AVR_PIN_BTN_ABXY        gpio::B6
#define AVR_PIN_BTN_2           gpio::B7
#define AVR_PIN_PWM_BACKLIGHT   gpio::C0
#define AVR_PIN_IOVDD_EN        gpio::C1
#define AVR_PIN_BTN_4           gpio::C2
#define AVR_PIN_RGB             gpio::C3
#define AVR_PIN_BTN_DPAD        gpio::C4
#define AVR_PIN_QSPI_SS         gpio::C5

using namespace rckid;

/** The RCKid firmware
 
    The AVR in RCKid acts as an IO controller (buttons, RGB, rumbler, backlight), power controller (on/off/flash, charging, battery monitoring) and always on RTC. The firmware is very simple with all work happening in the main loop and various interrupts simply setting flags to notify main loop about events. The chip also acts as an I2C slave and can be queried by the RP2350 about device status. Optionally the AVR allows debugging over USART which is shared with the interrupt pin to the RP2350. For more details see the subsystems below:

    - initialization & main loop, 
    - power subsystem
    - system ticks & clocks
    - communications (I2C) and master mode routines
    - button matrix
    - rumbler & backlight (PWM)
    - RGB LEDs

    One problem is observability of the AVR state. For general debugging, we have the TX line that can be used to print debug information to serial port, but this has timing effect on the system so may not be perfect. Furthermore it requires the serial monitor to be connected to the device, which is not always practical.

    An alternative is to use debug mode, which can be activated by holding the volume down button while powering the device. When in the debug mode, the rgb colors are turned on immediately after entering the wakeup mode and when powered on, the display brightness is set to 50% immediately. Furthermore the volume keys are not repoprted to the RP2350, but processed by the AVR so that pressing the power up button resets the RP chip, while pressing volume down resets the RP chip into bootloader mode. 
 */
class RCKid {
public:

    static void initialize() {
        // initialize the AVR chip
        initializeAVR();
        // set date to something meaningful
        state_.time.date.setYear(2026);
        state_.time.date.setMonth(1);
        state_.time.date.setDay(1);

        // TODO some initialization routine with checks, etc.
        LOG("\n\n\nSYSTEM RESET DETECTED (AVR): " << hex(RSTCTRL.RSTFR));

        // and start the device in powerOff mode, i.e. set sleep mode and start checking ctrl group interrupts
        stopSystemTicks();
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);

        // force enter debug mode right after power on
        // TODO remove this after testing 
        setPowerMode(POWER_MODE_WAKEUP);
        enterDebugMode();
        homeBtnLongPress();
        homeBtnLongPress_ = 0;
        // simulate we pressed the power on button
        state_.status.setControlButtons(true, false, false);
    }

    static void loop() {
        while (true) {
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
#if AVR_INT_IS_SERIAL_TX
            // wait for any TX transmissions before going to sleep 
            serial::waitForTx();
#endif
            // After everything was processed, go to sleep - we will wake up with the ACCEL or PMIC interrupts, or if in power off mode also by the HOME button interrupt
            cpu::sei();
            sleep_enable();
            sleep_cpu();
        }
    }

    /** Basic initialization of the AVR firmware. Sets clock frequency, enables RTC (including interrupt) and in case debugging over serial wire is used, enables the serial out on AVR_IRQ pin as well. 
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

    #if AVR_INT_IS_SERIAL_TX
        // initializes the AVR TX pin for serial debugging
        serial::setAlternateLocation(true);
        serial::initializeTx(RCKID_SERIAL_SPEED);
    #endif

    }

    /** \name Power & Sleep Controls

        The power is controlled via the power mode register, which when zero signals device off, in which case the AVR goes to deep sleep only to wake up when the accel or power interrupts are triggered, or the home button is pressed. When non-zero the register acts as a bitmask of possible reasons for the AVR to stay awake. This could be either the user-facing power on mode, when also the RP2350 is powered on, or can be when the device is charging (we need to indicate the charging status using the LEDs), or when waking up (need to calculate the power button press duration). 
     */
    //@{

    static constexpr uint8_t POWER_MODE_DC = 1;
    static constexpr uint8_t POWER_MODE_CHARGING = 2;
    static constexpr uint8_t POWER_MODE_WAKEUP = 4;
    static constexpr uint8_t POWER_MODE_ON = 8;
    static inline uint8_t powerMode_ = 0;
    
    static void setPowerMode(uint8_t mode) {
        if (powerMode_ & mode)
            return;
        // if we are transitionioning from complete off, start system ticks and set sleep mode to standby
        if (powerMode_ == 0) {
            LOG("systick start, sleep standby");
            //set_sleep_mode(SLEEP_MODE_STANDBY);
            set_sleep_mode(SLEEP_MODE_IDLE);
            startSystemTicks();
        }
        powerMode_ |= mode;
        // if turning on, clear any notification (power when off)
        switch (mode) {
            case POWER_MODE_ON:
                setNotification(RGBEffect::Off());
                break;
            case POWER_MODE_DC:
                state_.status.setVUsb(true);
                if (! (powerMode_ & POWER_MODE_ON)) 
                    setNotification(RGBEffect::Breathe(platform::Color::Green().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), RCKID_RGB_NOTIFICATION_SPEED));
                break;
            case POWER_MODE_CHARGING:
                state_.status.setCharging(true);
                if (! (powerMode_ & POWER_MODE_ON)) 
                    setNotification(RGBEffect::Breathe(platform::Color::Orange().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), RCKID_RGB_NOTIFICATION_SPEED));
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
        if (mode == POWER_MODE_ON && state_.status.debugMode())
            state_.status.setDebugMode(false);
        // if we are transitioning to complete off, stop system ticks and set sleep mode to power down
        if (powerMode_ == 0) {
            LOG("systick stop, sleep pwrdown");
            stopSystemTicks();
            set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        }
        switch (mode) {
            case POWER_MODE_ON:
                // if we are turning off, set notification according to other power modes
                if (powerMode_ & POWER_MODE_CHARGING)
                    setNotification(RGBEffect::Breathe(platform::Color::Orange().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), RCKID_RGB_NOTIFICATION_SPEED));
                else if (powerMode_ & POWER_MODE_DC)
                    setNotification(RGBEffect::Breathe(platform::Color::Green().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), RCKID_RGB_NOTIFICATION_SPEED));
                else
                    setNotification(RGBEffect::Off());
                break;
            case POWER_MODE_DC:
                state_.status.setVUsb(false);
                if (! (powerMode_ & POWER_MODE_ON))
                    setNotification(RGBEffect::Off());
                break;
            case POWER_MODE_CHARGING:
                state_.status.setCharging(false);
                if (! (powerMode_ & POWER_MODE_ON))
                    setNotification(RGBEffect::Breathe(platform::Color::Green().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), RCKID_RGB_NOTIFICATION_SPEED));
                break;
            default:
                // no action
                break;  
        }
    }

    /** Initializes the device power on state. 
     */
    static void powerOn() {
        LOG("power on");
        NO_ISR(
            // set power mode, which also ensures system ticks are running
            setPowerMode(POWER_MODE_ON);
            powerIOVDD(true);
            // initialize the PWM subsystem for baclight & rumbler
            initializePWM();
            setBacklightPWM(state_.brightness);
            ADC0.CTRLA |= ADC_RUNSTBY_bm;
        );
        for (int i = 0; i < NUM_RGB_LEDS; ++i)
            rgbEffect_[i] = RGBEffect::Off();
        setRumblerEffect(RumblerEffect::OK());

    }

    /** Initializes the device power off state. 
     */
    static void powerOff() {
        LOG("power off");
        NO_ISR(
            clearPowerMode(POWER_MODE_ON);
            powerIOVDD(false);
            disablePWM();
            // make the AVR_INT floating so that we do not leak any voltage to the now off RP2350
            gpio::outputFloat(AVR_PIN_AVR_INT);
            // clear IRQ
            irq_ = false;
        );
        // disable debug mode (only exists in power on mode)
        state_.status.setDebugMode(false);
    }

    static void lowBatteryWarning() {
        setNotification(RGBEffect::Breathe(platform::Color::Red().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), 8));
    }

    static void enterDebugMode() {
        LOG("Debug mode on");
        state_.status.setDebugMode(true);
        if (powerMode_ & POWER_MODE_ON) {
            if (state_.brightness < 128)
                setBacklightPWM(128);
            // TODO TODO TODO
            for (uint8_t i = 0; i < NUM_RGB_LEDS; ++i)
                rgbEffect_[i] = RGBEffect::Breathe(platform::Color::Purple().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), RCKID_RGB_NOTIFICATION_SPEED);
            rgbOn(true);
        }
    }

    static void leaveDebugMode() {
        LOG("Debug mode off");
        state_.status.setDebugMode(false);
        // turn the RGB LEDs off
        // TODO TODO TODO 
        for (int i = 0; i < NUM_RGB_LEDS; ++i)
            rgbEffect_[i] = RGBEffect::Off();
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
            LOG("VDD on");
        } else {
#if !AVR_INT_IS_SERIAL_TX            
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
            LOG("VDD off");
        }
    }

    /** Reboots the RP2350 chip. 
     
        Rebooting the RP is done by a simple power cycle on the IOVDD line. Signalled by red lights. 
     */
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
                        rgb_[0] = platform::Color::Cyan().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS);
                        rgb_[1] = platform::Color::Cyan().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS);
                        rgb_[2] = platform::Color::Cyan().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS);
                        rgb_[3] = platform::Color::Cyan().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS);
                        break;
                    case 1:
                        rgb_[RGB_LED_BTN_B] = platform::Color::Cyan().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS);
                        break;
                    case 2:
                        rgb_[RGB_LED_BTN_A] = platform::Color::Cyan().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS);
                        break;
                    case 3:
                        rgb_[RGB_LED_BTN_SELECT] = platform::Color::Cyan().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS);
                        break;
                    default:
                        rgb_[RGB_LED_BTN_START] = platform::Color::Cyan().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS);
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
                        rgb_[0] = platform::Color::Blue().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS);
                        rgb_[1] = platform::Color::Blue().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS);
                        rgb_[2] = platform::Color::Blue().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS);
                        rgb_[3] = platform::Color::Blue().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS);
                        break;
                    case 1:
                        rgb_[RGB_LED_BTN_B] = platform::Color::Blue().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS);
                        break;
                    case 2:
                        rgb_[RGB_LED_BTN_A] = platform::Color::Blue().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS);
                        break;
                    case 3:
                        rgb_[RGB_LED_BTN_SELECT] = platform::Color::Blue().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS);
                        break;
                    default:
                        rgb_[RGB_LED_BTN_START] = platform::Color::Blue().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS);
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
            //setNotification(RGBEffect::Breathe(platform::Color::Blue().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), 7));
            for (uint8_t i = 0; i < NUM_RGB_LEDS; ++i)
                rgbEffect_[i] = RGBEffect::Breathe(platform::Color::Blue().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), 7);
        );
    }

    //@}

    /** \name System ticks and clocks.
     
        System ticks are used only when the AVR is outside of the power off mode. They provide basic clock for RGB and rumbler effects (div by 3) and control the timing of reading button matrix and home button long press.

        The second tick is always on and keeps the RTC count. When not in power off mode it also alternates between measuring the temperatur and battery voltage. 

        Second tick is also responsible for checking the alarm and for resetting the daily budget. The budget it reset at midnight every day, but the app also keeps a countdown to next available reset to avoid resetting the budget too often in case the date & time are changed by the user.
     */
    //@{

    static inline volatile bool systemTick_ = false;
    static inline volatile bool secondTick_ = false;
    static inline uint8_t tickCounter_ = 0;
    static inline uint32_t budgetResetCountdown_ = 0;

    /** Starts the system tick on RTC with 5ms interval. 
     */
    static void startSystemTicks() {
        if (RTC.CTRLA & RTC_RTCEN_bm)
            return;
        LOG("systick - enable RTC");
        // initialize buttons matrix for CTRL row
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
    }

    static void stopSystemTicks() {
        // no harm disabling multiple times
        LOG("systick - disable RTC");
        while (RTC.STATUS & RTC_CTRLABUSY_bm);
        RTC.CTRLA = 0;
        // initialize buttons so that we are always reading control row when system ticks not used
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
        ++state_.uptime;
        state_.time.inc();
        if (state_.alarm.check(state_.time)) {
            // power the device on, if not on yet
            powerOn();
            // and set the IRQ to notify the RP2350
            NO_ISR(
                state_.status.setAlarmInt();
                setIrq();
            );
        }
        if (powerMode_ != 0) {
            if (powerMode_ & POWER_MODE_ON) {
                // tell RP to increase second
                NO_ISR(
                    state_.status.setSecondInt();
                    setIrq();
                );
                LOG("uptime " << state_.uptime);
            }
            // read either battery voltage, or temperature every other second using ADC0
            if ((state_.uptime & 1) == 0) {
                static_assert(AVR_PIN_VCC_SENSE == gpio::A2);
                startADC(ADC_MUXPOS_AIN2_gc);
            } else {
                startADC(ADC_MUXPOS_TEMPSENSE_gc);
            }
        }
        // and finally, reset the daily budget if needed. The hour check and the countdown ensure that we only reset the budget once per day at midnight, even if the time is changed by the user
        if (budgetResetCountdown_ > 0) {
            --budgetResetCountdown_;
        } else if (state_.time.time.hour() == 0) {
            state_.budget = state_.dailyBudget;
            budgetResetCountdown_ = 3600 * 24;
        }
    }
    //@}

    /** \name I2C Communications
     
        The I2C communication happens in two modes - slave, which is enabled when the device is in power on mode and acts simply as a I2C slave, and master mode that is temporarily enabled during power off modes when the device can interact with power management and sensors for various functionality. 

        In the slave mode, only the following operations are possible:

        - master read will always read the state starting from the beginning to the end (and technically beyond)
        - master write will always write stuff to the communications buffer (which is part of the state)

        After each master write, the contents of the comm buffer is interpreted as a command and executed. Only one command can be executed at a time. 

        Furthermore the AVR also communicates with the RP2350 via a dedicated AVR_INT line, which is normally floating, but will be pulled low by the AVR when there is a state change that the RP2350 should react to by reading the status. This happens when there are input changes, or sensor or power interrupts were received. 

     */
    //@{

    static inline TransferrableState state_;
    static inline volatile bool irq_ = false;

    static inline volatile bool i2cCommandReady_ = false;
    static inline uint8_t i2cTxIdx_ = 0;
    static inline uint8_t i2cRxIdx_ = 0;

    static constexpr uint8_t ACCEL_INT_REQUEST = 1;
    static constexpr uint8_t HOME_BTN_INT_REQUEST = 2;
    static constexpr uint8_t CHARGING_INT_REQUEST = 4;

    static inline volatile uint8_t intRequests_ = 0;

    static inline uint8_t ramPages_[1024];

    static void processIntRequests() {
        if (intRequests_ == 0)
            return;
        uint8_t irqs;
        NO_ISR(
            irqs = intRequests_;
            intRequests_ = 0;
        );
        if (powerMode_ & POWER_MODE_ON) {
            if (irqs & ACCEL_INT_REQUEST) {
                NO_ISR(
                    state_.status.setAccelInt();
                    setIrq();
                );
            }
            ASSERT(irqs & HOME_BTN_INT_REQUEST == 0);
            ASSERT(irqs & CHARGING_INT_REQUEST == 0);
        } else {
            // TODO deal with accel and power interrupts - this probably requires some I2C communcation to determine what is going on
            // if the power button is pressed, wake up - enter the debug mode depending on whether the volume down button is pressed
            if (irqs & HOME_BTN_INT_REQUEST) {
                // reset home button long press timeout
                homeBtnLongPress_ = RCKID_HOME_BUTTON_LONG_PRESS_FPS;
                // wakeup to start counting
                setPowerMode(POWER_MODE_WAKEUP);
                // tentatively set debug mode if we read volume down button pressed, it will be cleared if the button is released before entering the power on mode
                enterDebugMode();
            }
            // if the charging pin is high, it means VUSB is connected, and the battery is not charging. We set the USB to true so that the brief pulse of STAT high when the chip is powered on will ensure proper transitioning to USB power as well 
            if (irqs & CHARGING_INT_REQUEST) {
                if (gpio::read(AVR_PIN_CHARGING) == true) {
                    setPowerMode(POWER_MODE_DC);
                    clearPowerMode(POWER_MODE_CHARGING);
                // if the pin is low, and we are in USB power, it means we are charging
                } else if (state_.status.vusb()) {
                    setPowerMode(POWER_MODE_CHARGING);
                } else {
                    clearPowerMode(POWER_MODE_CHARGING);
                }
            }
        }
    }

    /** Sets the IRQ by pulling the AVR_IRQ pin low.
     
        NOTE expects interrupts disabled to work properly.
     */
    static void setIrq() {
        if (! (powerMode_ & POWER_MODE_ON))
            return;
        if (irq_)
            return;
        irq_ = true;
#if !AVR_INT_IS_SERIAL_TX        
        // only change the pin state if we are not using it for serial TX
        gpio::outputLow(AVR_PIN_AVR_INT);
#endif
    }

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
            TWI0.SDATA = ((uint8_t*) & state_)[i2cTxIdx_];
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
            // clear IRQ once we read the state 
            if (++i2cTxIdx_ == 3) {
                irq_ = false;
#if !AVR_INT_IS_SERIAL_TX                
                // only change the pin state if we are not using it for serial TX
                gpio::outputFloat(AVR_PIN_AVR_INT);
#endif
                state_.status.clearInterrupts();
            }
            // TODO send nack when done sending all state
        // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
        } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
            state_.buffer[i2cRxIdx_++] = TWI0.SDATA;
            //rgbs_[2] = platform::Color::Green().withBrightness(32);
            TWI0.SCTRLB = (i2cRxIdx_ == sizeof(state_.buffer)) ? TWI_SCMD_COMPTRANS_gc : TWI_SCMD_RESPONSE_gc;
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
        // process the commands
        LOG("Cmd: " << state_.buffer[0]);
        switch (state_.buffer[0]) {
            case cmd::Nop::ID:
                break;
            case cmd::PowerOff::ID:
                powerOff();
                break;
            case cmd::Sleep::ID:
                // TODO
                break;
            case cmd::ResetRP::ID:
                rebootRP();
                break;
            case cmd::BootloaderRP::ID:
                bootloaderRP();
                break;
            case cmd::ResetAVR::ID:
                // TODO
                break;
            case cmd::BootloaderAVR::ID:
                // TODO
                break;
            case cmd::DebugModeOn::ID:
                enterDebugMode();
                break;
            case cmd::DebugModeOff::ID:
                leaveDebugMode();
                break;
            case cmd::SetBrightness::ID: {
                uint8_t value = cmd::SetBrightness::fromBuffer(state_.buffer).value;
                LOG("Brightness: " << value);
                setBacklightPWM(value);
                state_.brightness = value;
                break;
            }
            case cmd::SetTime::ID: {
                state_.time = cmd::SetTime::fromBuffer(state_.buffer).value;
                break;
            }
            case cmd::SetAlarm::ID: {
                state_.alarm = cmd::SetAlarm::fromBuffer(state_.buffer).value;
                break;
            }
            case cmd::SetAudioSettings::ID: {
                state_.audio = cmd::SetAudioSettings::fromBuffer(state_.buffer).settings;
                break;
            }
            case cmd::SetBudget::ID: {
                state_.budget = cmd::SetBudget::fromBuffer(state_.buffer).seconds;
                break;
            }
            case cmd::SetDailyBudget::ID: {
                state_.dailyBudget = cmd::SetDailyBudget::fromBuffer(state_.buffer).seconds;
                break;
            }
            case cmd::DecBudget::ID: {
                if (state_.budget > 0)
                    --state_.budget;
                break;
            }
            case cmd::ResetBudget::ID: {
                state_.budget = state_.dailyBudget;
                budgetResetCountdown_ = 3600 * 24; // reset the countdown to 24 hours
                break;
            }
            case cmd::ReadFlashPage::ID: {
                uint16_t page = cmd::ReadFlashPage::fromBuffer(state_.buffer).page;
                // flash is mapped to 0x8000 for LD/ST instructions
                uint8_t * addr = 0x8000 + (page * 128);
                for (unsigned i = 0; i < 128; ++i)
                    state_.buffer[i] = *(addr + i);
                break;
            }
            case cmd::ReadEEPROMPage::ID: {
                static_assert(EEPROM_START == 0x1400); // from the datasheet
                uint16_t page = cmd::ReadEEPROMPage::fromBuffer(state_.buffer).page;
                uint8_t * addr = EEPROM_START + (page * 32);
                for (unsigned i = 0; i < 32; ++i)
                    state_.buffer[i] = *(addr + i);
                break;
            }

            case cmd::ReadRAMPage::ID: {
                uint16_t page = cmd::ReadRAMPage::fromBuffer(state_.buffer).page;
                uint16_t offset = page * 32;
                for (unsigned i = 0; i < 32; ++i)
                    state_.buffer[i] = ramPages_[offset + i];
                break;
            }

            case cmd::WriteFlashPage::ID: {
                // TODO unlock the flash update
                auto & c = cmd::WriteFlashPage::fromBuffer(state_.buffer);
                uint8_t * addr = 0x8000 + (c.page * 128);
                for (unsigned i = 0; i < 128; ++i)
                    *(addr++) = c.data[i];
                // and write the page
                _PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASEWRITE_gc);
                while (NVMCTRL.STATUS & (NVMCTRL_FBUSY_bm | NVMCTRL_EEBUSY_bm));
                break;
            }
            case cmd::WriteRAMPage::ID: {
                auto & c = cmd::WriteRAMPage::fromBuffer(state_.buffer);
                uint16_t offset = c.page * 32;
                for (unsigned i = 0; i < 32; ++i)
                    ramPages_[offset + i] = c.data[i];
                break;
            }
            /** Turns the RGB LEDs all off. 
             
                We don't simply set the LEDs off to allow the gradual transition to black to happen.
             */
            case cmd::RGBOff::ID: {
                for (int i = 0; i < NUM_RGB_LEDS; ++i)
                    rgbEffect_[i] = RGBEffect::Off();
                break;
            }
            /** Sets RGB effect for the given LED. 
             */
            case cmd::SetRGBEffect::ID: {
                auto & c = cmd::SetRGBEffect::fromBuffer(state_.buffer);
                rgbEffect_[c.index] = c.effect;
                rgbOn(true);
                break;
            }
            /** Sets all RGB effects at once. 
             */
            case cmd::SetRGBEffects::ID: {
                auto & c = cmd::SetRGBEffects::fromBuffer(state_.buffer);
                rgbEffect_[RGB_LED_BTN_B] = c.b;
                rgbEffect_[RGB_LED_BTN_A] = c.a;
                rgbEffect_[RGB_LED_DPAD_TOP_LEFT] = c.dpad;
                rgbEffect_[RGB_LED_DPAD_TOP_RIGHT] = c.dpad;
                rgbEffect_[RGB_LED_DPAD_BOTTOM_LEFT] = c.dpad;
                rgbEffect_[RGB_LED_DPAD_BOTTOM_RIGHT] = c.dpad;
                rgbEffect_[RGB_LED_BTN_SELECT] = c.sel;
                rgbEffect_[RGB_LED_BTN_START] = c.start;
                rgbOn(true);
                break;
            }
            /** Set only the DPAD led effects. 
             */
            case cmd::SetRGBEffectDPAD::ID: {
                auto & c = cmd::SetRGBEffectDPAD::fromBuffer(state_.buffer);
                rgbEffect_[RGB_LED_DPAD_TOP_LEFT] = c.topLeft;
                rgbEffect_[RGB_LED_DPAD_TOP_RIGHT] = c.topRight;
                rgbEffect_[RGB_LED_DPAD_BOTTOM_LEFT] = c.bottomLeft;
                rgbEffect_[RGB_LED_DPAD_BOTTOM_RIGHT] = c.bottomRight;
                rgbOn(true);
                break;
            }
            case cmd::Rumbler::ID: {
                auto & c = cmd::Rumbler::fromBuffer(state_.buffer);
                setRumblerEffect(c.effect);
                break;
            }
            case cmd::SetPin::ID: {
                auto & c = cmd::SetPin::fromBuffer(state_.buffer);
                state_.pin = c.pin;
                break;
            }
            case cmd::SetNotification::ID: {
                auto & c = cmd::SetNotification::fromBuffer(state_.buffer);
                setNotification(c.effect);;
                break;
            }
            default:
                ASSERT(false);
        }
        // and reset the command state so that we can read more commands 
        NO_ISR(
            i2cRxIdx_ = 0;
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

    static inline uint8_t homeBtnLongPress_ = 0;

    static void initializeButtons() {
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
        // reset tick counter to be in sync with the button sampling 
        tickCounter_ = 0;
        homeBtnLongPress_ = RCKID_HOME_BUTTON_LONG_PRESS_FPS;
    }

    static void homeBtnLongPress() {
        if (powerMode_ & POWER_MODE_ON) {
            powerOff();
        } else {
            ASSERT(powerMode_ & POWER_MODE_WAKEUP);
            powerOn();
            clearPowerMode(POWER_MODE_WAKEUP);
            // if we are in debug mode at this point, re-enable it again now that we have powered the IOVDD rail. This will turn on the LEDs and set backlight as well 
            if (state_.status.debugMode())
                enterDebugMode();
        }
    }

    static void readControlGroup() {
        bool changed = state_.status.setControlButtons(
            ! gpio::read(AVR_PIN_BTN_1), // home
            ! gpio::read(AVR_PIN_BTN_2), // volume up
            ! gpio::read(AVR_PIN_BTN_3)  // volume down
        );
        // only advance if we are running system ticks
        if (powerMode_ != 0) {
            gpio::high(AVR_PIN_BTN_CTRL);
            gpio::low(AVR_PIN_BTN_ABXY);
        }
        if (powerMode_ & POWER_MODE_ON) {
            if (changed) {
                LOG("CTRL: " << state_.status.btnHome() << " " << state_.status.btnVolumeUp() << " " << state_.status.btnVolumeDown());
                if (! state_.status.btnHome())
                    homeBtnLongPress_ = RCKID_HOME_BUTTON_LONG_PRESS_FPS;
                if (state_.status.debugMode() && ! state_.status.btnHome()) {
                    if (state_.status.btnVolumeUp()) {
                        rebootRP();
                        return;
                    } 
                    if (state_.status.btnVolumeDown()) {
                        bootloaderRP();
                        return;
                    }
                }
                NO_ISR(setIrq());
            }
        } else if (powerMode_ & POWER_MODE_WAKEUP) {
            // 
            if (! state_.status.btnHome())
                clearPowerMode(POWER_MODE_WAKEUP);
            if (state_.status.debugMode() && ! state_.status.btnVolumeDown())
                leaveDebugMode();
        } else {
            // we need to be in the DC mode, otherwise there is no way buttons are chcecked via control group
            ASSERT(powerMode_ & POWER_MODE_DC);
            // simulate the ISR routine to be picked in the main loop
            if (changed && state_.status.btnHome()) {
                NO_ISR(
                    intRequests_ |= HOME_BTN_INT_REQUEST;
                );
            }
        }
        // check if there has been long home button press - it's important we do this *after* the power on checks above as otherwise the long press can enter power on, but with wrong state 
        if (state_.status.btnHome()) {
            if (homeBtnLongPress_ > 0 && (--homeBtnLongPress_ == 0))
                homeBtnLongPress();
        }
    }

    static void readABXYGroup() {
        bool changed = state_.status.setABXYButtons(
            ! gpio::read(AVR_PIN_BTN_1), // a
            ! gpio::read(AVR_PIN_BTN_2), // b
            ! gpio::read(AVR_PIN_BTN_4), // sel
            ! gpio::read(AVR_PIN_BTN_3)  // start
        );
        gpio::high(AVR_PIN_BTN_ABXY);
        gpio::low(AVR_PIN_BTN_DPAD);
        if (changed) {
            LOG("ABXY: " << state_.status.btnA() << " " << state_.status.btnB() << " " << state_.status.btnSelect() << " " << state_.status.btnStart());
            NO_ISR(setIrq());
        }
    }

    static void readDPadGroup() {
        bool changed = state_.status.setDPadButtons(
            ! gpio::read(AVR_PIN_BTN_3), // left
            ! gpio::read(AVR_PIN_BTN_1), // right
            ! gpio::read(AVR_PIN_BTN_2), // up
            ! gpio::read(AVR_PIN_BTN_4)  // down
        );
        gpio::high(AVR_PIN_BTN_DPAD);
        gpio::low(AVR_PIN_BTN_CTRL);
        if (changed) {
            LOG("DPAD: " << state_.status.btnLeft() << " " << state_.status.btnRight() << " " << state_.status.btnUp() << " " << state_.status.btnDown());
            NO_ISR(setIrq());
        }
    }
    //@}

    /** \name ADC 
     
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
                state_.temp = static_cast<int16_t>(t);
                break;
            }
            case ADC_MUXPOS_AIN2_gc:
                // the battery voltage can be anything from 2.5 to 5V since the voltage of the AVR is fixed at 3v3, the VCC is measured through a voltage divider of 100k + 200k. With 10bit ADC this gives us 1023 being equal 4.95V and lsb of 4.838mV. 
                // this converts the 16bit value to volts x 100:
                value = (48 * value + 50) / 100;
                state_.status.setVcc(value);
                // if we are below VCC threshold, clear the DC and VUSB power modes, which also forces the device to go to sleep unless other power mode is enabled
                if (state_.status.vusb() && (value < RCKID_VUSB_THRESHOLD)) {
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
                if (state_.status.lowBattery() != (value < RCKID_LOW_BATTERY_THRESHOLD)) {
                    state_.status.setLowBattery(value < RCKID_LOW_BATTERY_THRESHOLD);
                    setIrq();
                }
                // emergency shutdown if battery too low
                if (value < RCKID_POWER_ON_THRESHOLD && (powerMode_ & POWER_MODE_ON)) {
                    // TODO notify & die, we might need a ring buffer for this to elliminate spurious shutdowns etc
                }
                break;
            default:
                UNREACHABLE;
        }
        // turn ADC off to save power
        ADC0.CTRLA = 0;
    }

    //@}

    /** \name PWM (Rumbler and Backlight)

        The PWM signals used for backlight and rumbler control are generated by the TCB0 and TCB1 respectively.

        Backlight is pulled low externally, setting the pin to 1 make the backlight work, hence the value is unchanged. By selecting TCA_CLK (from system tick) as clock source, we can run the PWM more effectively and save power.   
     */
    //@{

    static inline RumblerEffect rumblerEffect_;
    static inline RumblerEffect rumblerCurrent_;

    static void initializePWM() {
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
    }

    static void disablePWM() {
        TCB0.CTRLA = 0;
        TCB0.CTRLB = 0;
        gpio::outputFloat(AVR_PIN_PWM_BACKLIGHT);
        TCB1.CTRLA = 0;
        TCB1.CTRLB = 0;
        gpio::outputFloat(AVR_PIN_PWM_RUMBLER);
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

    /** Turns the rumbler off, disabling the motor and setting the rumbler effect to off. 
     */
    static void rumblerOff() {
        setRumblerPWM(0);
        rumblerEffect_ = RumblerEffect::Off();
        LOG("Rumbler off");
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


    /** \name RGB Effects
     
        The LEDs are powered from a 5V step-up generator that is turned off when the LEDs are not used to conserve power (each neopixel takes a bit more than 1mA even if not on at all).
     */
    //@{

    static constexpr unsigned NUM_RGB_LEDS = 8;

    static inline bool rgbOn_ = false;
    static inline RGBEffect notification_{RGBEffect::Off()};
    static inline uint8_t notificationTimeout_ = 0;
    static inline RGBEffect rgbEffect_[NUM_RGB_LEDS];
    static inline uint8_t rgbEffectTimeout_[NUM_RGB_LEDS] = {0};
    static inline platform::ColorStrip<NUM_RGB_LEDS> rgbTarget_;
    static inline platform::NeopixelStrip<NUM_RGB_LEDS> rgb_{AVR_PIN_RGB};

    static constexpr uint8_t RGB_TICKS_PER_SECOND = 66;
    static inline uint8_t rgbTicks_ = RGB_TICKS_PER_SECOND; 

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
        rgbTicks_ = RGB_TICKS_PER_SECOND;
    }

    /** Sets RGB notification, or turns it off if the effect is Off.  
     */
    static void setNotification(RGBEffect effect) {
        notification_ = effect;
        if (notification_.active()) {
            rgbOn(true);
        } 
        // set target to current top black to ensure smooth transition between the effects
        for (int i = 0; i < NUM_RGB_LEDS; ++i)
            rgbTarget_[i] = platform::Color::Black();
    }

    /** Clears the RGBs - disables effects and sets the current and target color to black for immediate switch to black color.
     */
    static void rgbClear() {
        for (uint8_t i = 0; i < NUM_RGB_LEDS; ++i) {
            rgbEffect_[i] = RGBEffect::Off();
            rgb_[i] = platform::Color::Black();
            rgbTarget_[i] = platform::Color::Black();
        }
        // and ensure the RGB state is propagated to the RGB LEDs
        if (rgbOn_)
            rgb_.update(true);        
    }

    static void rgbTick() {
        if (!rgbOn_)
            return;
        if (notification_.active()) {
            if (notificationTimeout_ > 0) {
                --notificationTimeout_;
                return;
            }
            notificationTimeout_ = notification_.skipTicks();
            // notifiations do not go away on a timeout, so no need to worry here
            // for notification, move all LEDs in sync
            bool done = true;
            for (int i = 0; i < NUM_RGB_LEDS; ++i)
                done = (! rgb_[i].moveTowards(rgbTarget_[i], notification_.changeDelta())) && done;
            if (done)
              for (int i = 0; i < NUM_RGB_LEDS; ++i)
                  rgbTarget_[i] = notification_.nextColor(rgbTarget_[i]);
            rgb_.update(true);
        } else {
            // if there has been second already, decrement effect durations
            if (--rgbTicks_ == 0) {
                rgbTicks_ = RGB_TICKS_PER_SECOND;
                for (int i = 0; i < NUM_RGB_LEDS; ++i) {
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
            for (int i = 0; i < NUM_RGB_LEDS; ++i) {
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
    }

    /** Debug function that simply enables the RGB leds and sets their color according to the three bytes given. Particularly useful for logging purposes. The bits are displayed in LSB first, in the following order
     
        DPAD_TOP_LEFT       1
        DPAD_TOP_RIGHT      2
        DPAD_BOTTOM_LEFT    4
        DPAD_BOTTOM_RIGHT   8
        BTN_B              16
        BTN_A              32
        BTN_SELECT         64
        BTN_START         128
     */
    static void rgbSetBits(uint8_t r, uint8_t g = 0, uint8_t b = 0) {
        rgbOn(true);
        rgbClear();
        for (uint8_t i = 0; i < 8; ++i) {
            uint8_t rr = ((r >> i) & 1) ? RCKID_RGB_LED_DEFAULT_BRIGHTNESS : 0; 
            uint8_t gg = ((g >> i) & 1) ? RCKID_RGB_LED_DEFAULT_BRIGHTNESS : 0;
            uint8_t bb = ((b >> i) & 1) ? RCKID_RGB_LED_DEFAULT_BRIGHTNESS : 0;
            switch (i) {
                case 0:
                    rgb_[RGB_LED_DPAD_TOP_LEFT] = platform::Color::RGB(rr, gg, bb);
                    break;
                case 1:
                    rgb_[RGB_LED_DPAD_TOP_RIGHT] = platform::Color::RGB(rr, gg, bb);
                    break;
                case 2:
                    rgb_[RGB_LED_DPAD_BOTTOM_LEFT] = platform::Color::RGB(rr, gg, bb);
                    break;
                case 3:
                    rgb_[RGB_LED_DPAD_BOTTOM_RIGHT] = platform::Color::RGB(rr, gg, bb);
                    break;
                case 4:
                    rgb_[RGB_LED_BTN_B] = platform::Color::RGB(rr, gg, bb);
                    break;
                case 5:
                    rgb_[RGB_LED_BTN_A] = platform::Color::RGB(rr, gg, bb);
                    break;
                case 6:
                    rgb_[RGB_LED_BTN_SELECT] = platform::Color::RGB(rr, gg, bb);
                    break;
                case 7:
                    rgb_[RGB_LED_BTN_START] = platform::Color::RGB(rr, gg, bb);
                    break;
            }
        }
        rgb_.update(true);
    }

    /** Fatal error routine which simply displays the argument value, line information and then enters infinite loop.
     */
    static void fatalError(uint16_t line, uint8_t arg = 0) {
        rgbSetBits(arg, line >> 8, line & 0xff);
        // TODO maybe this should instead just wait for long home button press, or something
        while (true) {
            cpu::wdtReset();
            cpu::delayMs(100);
        }
    }

    //@}

    /** \name Test routines
     
        The below routines can be used to verify the basic functionality of the AVR firmware one by one. Firmware methods used in production are used whenever possible.
     */
    //@{

    static void testRGB() {
        initializeAVR();
        cpu::delayMs(100);
        rgbOn(true);
        cpu::delayMs(100);
        rgb_[0] = platform::Color::Red();
        rgb_[1] = platform::Color::Green();
        rgb_[2] = platform::Color::Blue();
        rgb_[3] = platform::Color::Cyan();
        rgb_[4] = platform::Color::Yellow();
        rgb_[5] = platform::Color::White();
        rgb_.update(true);
        while (true) {
            cpu::wdtReset();
            cpu::delayMs(100);
        }
    }
    //@}

}; // class RCKid

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

/** Home button interrupt ISR.
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

/** Accel pin ISR.
 */
ISR(PORTB_PORT_vect) {
    static_assert(AVR_PIN_ACCEL_INT == gpio::B4);
    VPORTB.INTFLAGS = (1 << GPIO_PIN_INDEX(AVR_PIN_ACCEL_INT));
    // only do stuff if we are transitioning from low to high
    if (gpio::read(AVR_PIN_ACCEL_INT))
        RCKid::intRequests_ |= RCKid::ACCEL_INT_REQUEST;
}

/** VCC measurement in sleep node is ready. 
 */
ISR(ADC0_RESRDY_vect) {
    // don't change the flag, we use it, instead disable the interrupt
    ADC0.INTCTRL = 0;
    //ADC0.INTFLAGS = ADC_RESRDY_bm;
}

int main() {
    //RCKid::testRGB();

    RCKid::initialize();
    RCKid::loop();
}
