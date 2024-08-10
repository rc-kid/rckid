#include <avr/sleep.h>
#include <avr/delay.h>
#include <avr/interrupt.h>

#include <platform.h>
#include <platform/peripherals/ina219.h>
#include <platform/peripherals/neopixel.h>

#include <platform/utils/ringavg.h>

#include "rckid/common/config.h"
#include "rckid/common/state.h"

/** Displays AVR status and events on an external SSD1306 OLED display. Note this flag is only for debug purposes when writing own AVR code and that the AVR will hang & be reset via wdt if this is enabled and the oled display is not attached. 
*/
#define RCKID_AVR_DEBUG_OLED_DISPLAY

// TODO do I need timeouts for 3v3 and 5v powr on? 
// TODO increase tick to 2.5ms (?)
// TODO increase ADC sampling to have less errors
// TODO make vcc ring avg work even if not enough measurements yet
// TODO when systemtick is called, there seems to be reset of a sort - not sure what that is though

#if (defined RCKID_AVR_DEBUG_OLED_DISPLAY)
#include <platform/peripherals/ssd1306.h>
#endif

/** Rwrite the ASSERT macro to cause AVR error upon failure so that they get reported via the standard error ways.
 */
#undef ASSERT
#define ASSERT(id, ...) do { if (!(__VA_ARGS__)) error(AVR_ERROR_ASSERT, id); } while(false)

#define BREAKPOINT(id) error(AVR_ERROR_BREAKPOINT, id);

using namespace rckid;

/** RCKid AVR Firmware
 
    Gathers the user inputs, controls the LEDs and audio, tracks time and manages the power modes (sleep, charging, etc.). 
    
    Since the load is rather small, instead of using interrupts and risk data races, etc., handles as much as possible in the main loop. All functions defined inside the class are expected to be only executed from the main loop, the ISR code is only used for resetting the long press detection for the home button. 
     
 */
class RCKid {
public:

    /** Power-on routine. 
     
        Initializes the device state and peripherals. Assumes known power-on state with no voltage bleeding to any of the 3V3 rail systems such as QSPI (for bootloader activation), display (for reading framebuffer) and headphones. 
     */
    static void initialize() __attribute__((always_inline)) {
        // enable 2 second watchdog so that the second tick resets it always with enough time to spare
        while (WDT.STATUS & WDT_SYNCBUSY_bm); 
        _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_2KCLK_gc);      
        // set CLK_PER prescaler to 2, i.e. 8Mhz, which is the maximum the chip supports at voltages as low as 3.0V
        CCP = CCP_IOREG_gc;
        CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm;
        // initialize the RTC that fires every second for a semi-accurate real time clock keeping on the AVR and start counting
        RTC.CLKSEL = RTC_CLKSEL_INT32K_gc; // run from the internal 32.768kHz oscillator
        while (RTC.PITSTATUS & RTC_CTRLBUSY_bm);
        RTC.PITINTCTRL |= RTC_PI_bm; // enable the PIT interrupt
        RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc | RTC_PITEN_bm;
        // initialize the home button to input pullup and set its ISR
        gpio::setAsInputPullup(AVR_PIN_BTN_HOME);
        static_assert(AVR_PIN_BTN_HOME == B2); // otherwise the ISR won't work
        PORTB.PIN2CTRL |= PORT_ISC_FALLING_gc;
        resetErrors();
        /*
        for (int i = 0; i < 10; ++i) {
            //rgbs_[2] = platform::Color::RGB(32, 0, 0);
            //rgbs_.update(true);
            cpu::delayMs(1000);
            cpu::wdtReset();
        } */
        // determine the reset cause and set errors accordingly
        if (RSTCTRL.RSTFR & RSTCTRL_WDRF_bm)
            error(AVR_ERROR_WDT);
        if (RSTCTRL.RSTFR & RSTCTRL_BORF_bm)
            error(AVR_ERROR_BOD);
        // make sure interrupts are enabled
        sei();
        // start the chip in debug mode
        ts_.device.setDeviceMode(DeviceMode::PowerOff); 
        powerAVRSetActive();

        powerOn(/* forceDebug */ true);
        
    }

    /** The main loop. 
     
        
     */
    static void loop() __attribute__((always_inline)) {
        while (true) {
            cpu::wdtReset(); 
            i2cSlaveIRQHandler();
            if (ts_.device.deviceBusy()) {
                i2cProcessCommand();
            }
            if (VPORTB.INTFLAGS & (1 << 2)) {
                static_assert(AVR_PIN_BTN_HOME == B2);
                VPORTB.INTFLAGS = (1 << 2); // clearthe interrupt
                btnHomeDown();
            }
            if (ADC0.INTFLAGS = ADC_RESRDY_bm) {
                ADC0.INTFLAGS = ADC_RESRDY_bm; // clear the interrupt
                adcDone();
            }
            
            if (RTC.PITINTFLAGS & RTC_PI_bm) {
                RTC.PITINTFLAGS = RTC_PI_bm; // clear the interrupt
                secondTick();
            } 
            if (TCA0.SINGLE.INTFLAGS & TCA_SINGLE_OVF_bm) {
                TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; // clear the interrupt
                systemTick();
            }
            // now that we have performed all the tasks, check if we should go to sleep
            switch (ts_.device.deviceMode()) {
                case DeviceMode::Sleep:
                case DeviceMode::PowerOff:
                    // ensure interrupts are enabled - in case they were disabled for whatever reason, otherwise we would never wake up
                    sei();
                    // and sleep
                    sleep_enable();
                    sleep_cpu();
                    break;
                default:
                    break;
            }
        }
    }

private:

    static void secondTick() {
        ++ts_.uptime;
        ts_.time.secondTick();
        // if in sleep or poweroff mode, read the VCC so that we can determine DC power status or battery & charging health
        switch (ts_.device.deviceMode()) {
            case DeviceMode::Sleep:
            case DeviceMode::PowerOff:
                measureVCC();
                // since ADC is disabled in power down mode, set sleep mode to Standby
                set_sleep_mode(SLEEP_MODE_STANDBY);
                break;
            default:
                break;
        }
        // if the 5v rail is on, run the second tick for RGB effects. This should only ever happen in normal, debug, or charging modes
        if (power5v())
            rgbSecondTick();
    }

    static inline uint8_t tick_ = 0;

    static void systemTick() {
        ++tick_;
        switch (tick_ & 3) {
            case 0: {
                // enable power to dpad buttons for next tick and start measuring VCC
                enableDPADButtons();
                measureVCC();
                // since we have a tick worth 2ms, checking at every fourth tick gives us ~100Hz rate, which is rather useful for home button testing and effects
                bool home = !gpio::read(AVR_PIN_BTN_HOME);
                ts_.device.setBtnHome(!gpio::read(AVR_PIN_BTN_HOME));
                btnHomeCheckLongPress();
                break;
            }
            case 1: {
                // measure DPAD buttons from previous iteration, enable power to ABXY buttons for next tick and start measuring battery voltage
                measureDPADButtons();
                enableABXYButtons();
                measureVBatt();
                break;
            }
            case 2: {
                // measure ABXY buttons and enable power for volume buttons for the next tick
                measureABXYButtons();
                enableVolumeButtons();
                // if audio is active, check headphones
                if (ts_.device.audioEnabled())
                    measureHeadphones();
                break;
            }
            case 3: {
                // measure the volume buttons, don't enable other matrix parts, as they will be enabled in the next tick
                measureVolumeButtons();
                // ans start measuring the temperature
                measureTemperature();
                // if debug mode is active check the volume buttons for the reset & bootloader actions
                if (ts_.device.deviceMode() == DeviceMode::Debug) {
                    if (ts_.device.btnVolUp())
                        powerRP2040Reset();
                    else if (ts_.device.btnVolDown())
                        powerRP2040Bootloader();
                }
                break;
            }
        }
        // Run the LEDs on 50Hz (every 8 ticks) if the 5v rail is enabled
        if (tick_ % 7 == 0 && power5v())
            rgbTick();
        // if the rumbler is active, do rumbler tick as well, i.e. every 2ms
        if (rumblerCurrent_.active())
            pwmRumbleTick();
        // check charging status if dc power is enabled (otherwise the value on charging pin might flicker)
        if (ts_.device.dcPower()) {
            bool charging = !gpio::read(AVR_PIN_CHARGING);
            if (ts_.device.charging() != charging) {
                ts_.device.setCharging(charging);
                rgbUpdateSystemEffect();
            }
        }
    }

    /** \name Errors & Debugging
     
        Since the AVR does not have any free pins that could easily be used for debugging purposes, error handling is a bit complicated and can happen in three flavors:

        1) normal mode, in which the device state's deviceError field is set to 1 everytime the AVR chip encounters an error and the error is logged into a error list area, which consists of the number of errors observed and last N errors and their arguments (each error has its own number). The RP2040 can then use I2C commands to read the errors and display them in user friendly format. 

        2) debug mode, which works the same as normal mode, but if an error is reported, the latest error will be displayed using the keyboard LEDs. See the LED error format below for more details

        3) I2C display, in which case an I2C attached OLED display is expected to be attached to the I2C bus and is used to display various debugging information, including errors. 

        The error buffer length is 32, which means it is good for 16 last errors (this is the same size as the I2C comms buffer, i.e. the entire error buffer can be transferred in single I2C command). To make the communication simpler, whenever new error is raised, the error buffer is implicitly copied to the comms buffer so that simply reading from AVR enough bytes will return the errors. Since the buffer in 33 bytes, the first byte is used to contain the lower byte of number of errors so far.   

        # LED Error Format

        When an AVR error is displayed on the LEDs in debug mode, the system LED above the screen is solid red, while the LEDs under keys show the binary representation of the error and its argument via three colors:

        LED (key)| Red       | Green     | Blue
        ---------|-----------|-----------|------------
        DPad     | ERR4 (16) | ARG4 (16) | ARG7 (128)
        B        | ERR3 (8)  | ARG3 (8)  | ARG6 (64)
        A        | ERR2 (4)  | ARG2 (4)  | ARG5 (32)
        Select   | ERR1 (2)  | ARG1 (2)  |
        Start    | ERR0 (1)  | ARG0 (1)  |

     */
    //@{

    static inline uint16_t numErrors_ = 0;
    static inline uint8_t errorBufferIdx_ = 0;
    static inline uint8_t errorBuffer_[32];

#if (defined RCKID_AVR_DEBUG_OLED_DISPLAY)
    static inline platform::SSD1306 oled_;
#endif

    static void resetErrors() {
        numErrors_ = 0;
        errorBufferIdx_ = 0;
        for (uint8_t i = 0; i < 32; ++i)
            errorBuffer_[i] = AVR_ERROR_NONE;
    }

    static void error(uint8_t error, uint8_t arg = 0) {
        // store the error to the buffer
        ++numErrors_;
        errorBuffer_[errorBufferIdx_] = error;
        errorBuffer_[errorBufferIdx_ + 1] = arg;
        errorBufferIdx_ = (errorBufferIdx_ + 2) & 31;
        // set the device error flag
        ts_.device.setDeviceError(true);
        // copy the error buffer to the comms buffer immediately
        errorCopyBuffer();
        // if in debug mode, display the error on the RGBs
        if (ts_.device.deviceMode() == DeviceMode::Debug || true) {
            rgbSetEffect(3, RGBEffect::Solid(platform::Color::RGB(
                (error & 16) ? RGB_COLOR_BRIGHTNESS : 0,
                (arg & 16) ? RGB_COLOR_BRIGHTNESS : 0,
                (arg & 128) ? RGB_COLOR_BRIGHTNESS : 0
            ), 255));
            rgbSetEffect(0, RGBEffect::Solid(platform::Color::RGB(
                (error & 8) ? RGB_COLOR_BRIGHTNESS : 0,
                (arg & 8) ? RGB_COLOR_BRIGHTNESS : 0,
                (arg & 64) ? RGB_COLOR_BRIGHTNESS : 0
            ), 255));
            rgbSetEffect(1, RGBEffect::Solid(platform::Color::RGB(
                (error & 4) ? RGB_COLOR_BRIGHTNESS : 0,
                (arg & 4) ? RGB_COLOR_BRIGHTNESS : 0,
                (arg & 32) ? RGB_COLOR_BRIGHTNESS : 0
            ), 255));
            rgbSetEffect(4, RGBEffect::Solid(platform::Color::RGB(
                (error & 2) ? RGB_COLOR_BRIGHTNESS : 0,
                (arg & 2) ? RGB_COLOR_BRIGHTNESS : 0,
                0
            ), 255));
            rgbSetEffect(5, RGBEffect::Solid(platform::Color::RGB(
                (error & 1) ? RGB_COLOR_BRIGHTNESS : 0,
                (arg & 1) ? RGB_COLOR_BRIGHTNESS : 0,
                0
            ), 255));
            // and make sure the system LED is solid red
            rgbUpdateSystemEffect();
            //cpu::delayMs(50);
            /*
            for (uint8_t i =0; i < 255; ++i) {
                rgbTick();
                cpu::wdtReset();
            } */
        }
        // and if we have the OLED display available, tell about the error
#if (defined RCKID_AVR_DEBUG_OLED_DISPLAY)
        // TODO - convert the OLED display to writer API
#endif

        /*
        while (true) {
            cpu::wdtReset();
        }
        */
    }

    static void errorCopyBuffer() {
        uint8_t idx = 0;
        ts_.buffer[0] = (numErrors_ & 0xff);
        for (uint8_t i = 1; i < 33; ++i) {
            ts_.buffer[i] = errorBuffer_[idx];
            idx = (idx + 1) & 31;
        }
        // ensure that we are done & if there was an outstanding command during the error, make sure it will not be processed (we'd be processing the number of errors instead)
        ts_.device.setDeviceBusy(false);
    }
    //@}

    /** \name Power management & device modes

        The AVR itself can be in two states - active, or sleeping. In the active state, the AVR chip does not sleep and a 2ms tick signal is generated that controls reading & controlling peripherals, power data and user inputs. In the sleeping state, the AVR sleeps and periodically wakes up every second to check VCC and determine if an action should be taken (transition to active state, or complete power off). 

        On top of the AVR states, there are 4 basic device states:

        - normal, in which the RP2040 is powered on and controls the device. AVR must be in active state and monitors the power & user inputs
        - debug which is like normal, but with tweaks useful during development such as error observability and volume key shortcuts for rp2040 reset and bootloader
        - sleep in which the RP2040 sleeps, but the 3v3 rail is on so that sensors can continue operation and the RP2040 can resume wherever it left. 
        - power off, when the 3v3 rail is off completely

        In sleep and power off the AVR is only active if DC power is applied so that it can monitor battery charging.   
     */
    //@{
    static bool powerAVRActive() {
        return TCA0.SINGLE.CTRLA & TCA_SINGLE_ENABLE_bm;
    }

    static void powerAVRSetActive() {
        // initialize ADC0
        adcInitialize();
        // enable 2ms system tick timer (TCA)
        TCA0.SINGLE.PER = 250;
        TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm | TCA_SINGLE_CLKSEL_DIV64_gc;
        // restart the ticks so that on next tick we will wrap around to 0 and start reading DPAD and VCC
        initializeButtonMatrix();
        tick_ = 3; 
        // TODO initialize I2C master if we have INA219 and start talking to it 
    }

    static void powerAVRTrySleep() {
        // we must *never* sleep if the DC power is active so that the charging can be effectively monitored
        if (ts_.device.dcPower())
            return;
        // disable all systems we can
        adcDisable();
        // always ensure that 5v rail is off (with AVR sleeping, we have no way of controling the leds anyways so we'd just be bleeding power)
        rgbDisable();
        // disable the system tick timer
        TCA0.SINGLE.CTRLA = 0;
        TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
        // disable I2C master
        TWI0.MCTRLA = 0;
    }

    static void powerOn(bool forceDebug = false) {
        // power on should only be called from sleep and power off modes
        ASSERT(0x00, ts_.device.deviceMode() == DeviceMode::Sleep || ts_.device.deviceMode() == DeviceMode::PowerOff);
        ASSERT(0x01, powerAVRActive());
        // initialize the PWM (backlight & rumbler)
        initializePWM();
        // turn on the 3V3 rail
        gpio::outputHigh(AVR_PIN_3V3_ON);
        // initialize the I2C slave operation and enable its wakeup interrupts
        i2cInitialize();
        // determine if we should enter the debug mode instead
        if (forceDebug || ts_.device.btnSel()) {
            ts_.device.setDeviceMode(DeviceMode::Debug);
            pwmBacklight(128);
            // TODO rumble to singal we have started the RP2040 chip correctly? 
        } else {
            ts_.device.setDeviceMode(DeviceMode::Normal);
        }
    }

    static void sleep() {
        // sleep can only be entered from normal and debug modes
        ASSERT(0x03, ts_.device.deviceMode() == DeviceMode::Normal || ts_.device.deviceMode() == DeviceMode::Debug);
        // mark the state in device status
        ts_.device.setDeviceMode(DeviceMode::Sleep);
        // disable I2C slave (noone to call us)
        TWI0.SCTRLA = 0;
        // disable PWM too
        pwmDisable();
        // put AVR to sleep as well if we are not running in DC mode, otherwise remain active to monitor the charging
        powerAVRTrySleep();
    }

    static void powerOff() {
        // power off can be applied from all states
        ASSERT(0x04, ts_.device.deviceMode() == DeviceMode::Normal || ts_.device.deviceMode() == DeviceMode::Debug || ts_.device.deviceMode() == DeviceMode::Sleep);
        // mark the state in device status
        ts_.device.setDeviceMode(DeviceMode::PowerOff);
        // cut power to 3v3 rail
        power3v3(false);
        // disable I2C slave (noone to call us)
        TWI0.SCTRLA = 0;
        // disable PWM too
        pwmDisable();
        // put AVR to sleep as well if we are not running in DC mode
        powerAVRTrySleep();
    }

    static void powerRP2040Reset() {
        ASSERT(0x05, powerAVRActive());
        cli();
        i2cDisable();
        power3v3(false);
        power5v(true);
        rgbs_.fill(platform::Color::Black());
        for (int i = 0; i < 6; ++i) {
            rgbs_[i < 2 ? i : i + 1] = platform::Color::Red().withBrightness(RGB_COLOR_BRIGHTNESS);
            rgbs_.update();
            cpu::delayMs(200);
            cpu::wdtReset();
        }
        // disable the LEDs and then check if we need to update the system LED 
        rgbDisable();
        rgbUpdateSystemEffect();
        // turn the RP2040 on
        power3v3(true);
        // set backlight to decent value so that we can observe the restart
        // TODO maybe do this only in debug mode
        pwmBacklight(128);
        // and enable interrupts
        i2cInitialize();
        sei();
    }

    static void powerRP2040Bootloader() {
        ASSERT(0x06, powerAVRActive());
        platform::Color color = platform::Color::Green().withBrightness(RGB_COLOR_BRIGHTNESS);
        cli();
        i2cDisable();
        power3v3(false);
        power5v(true);
        rgbs_.fill(platform::Color::Black());
        rgbs_[0] = color;
        rgbsTarget_[0] = color;
        rgbs_.update();
        cpu::wdtReset();
        cpu::delayMs(200);
        rgbs_[1] = color;
        rgbsTarget_[1] = color;
        rgbs_.update();
        cpu::wdtReset();
        cpu::delayMs(200);
        gpio::outputLow(AVR_PIN_QSPI_SS);
        rgbs_[3] = color;
        rgbsTarget_[3] = color;
        rgbs_.update();
        cpu::wdtReset();
        cpu::delayMs(200);
        power3v3(true);
        rgbs_[4] = color;
        rgbsTarget_[4] = color;
        rgbs_.update();
        cpu::wdtReset();
        cpu::delayMs(200);
        rgbs_[5] = color;
        rgbsTarget_[5] = color;
        rgbs_.update();
        cpu::wdtReset();
        cpu::delayMs(150);
        gpio::outputFloat(AVR_PIN_QSPI_SS);
        cpu::wdtReset();
        cpu::delayMs(50);
        // keep the green lights on, the RP2040 app that has just been uploaded should turn them off
        rgbEffects_[0] = RGBEffect::Breathe(color);
        rgbEffects_[1] = RGBEffect::Breathe(color);
        rgbEffects_[3] = RGBEffect::Breathe(color);
        rgbEffects_[4] = RGBEffect::Breathe(color);
        rgbEffects_[5] = RGBEffect::Breathe(color);
        // and re-enable interrupts
        i2cInitialize();
        sei();
   }

    static bool power3v3() {
        return gpio::read(AVR_PIN_3V3_ON);
    }

    static void power3v3(bool value) {
        if (value)
            gpio::outputHigh(AVR_PIN_3V3_ON);
        else
            gpio::outputFloat(AVR_PIN_3V3_ON);
    }

    static bool power5v() {
        return gpio::read(AVR_PIN_5V_ON);
    }

    static void power5v(bool value) {
        if (value) {
            gpio::outputHigh(AVR_PIN_5V_ON);
            gpio::setAsOutput(AVR_PIN_RGB);
        } else {
            gpio::outputFloat(AVR_PIN_RGB);
            gpio::outputFloat(AVR_PIN_5V_ON);
        }
    }

    static void powerEnableCharging() {
        gpio::outputLow(AVR_PIN_CHARGE_EN);
    }

    static void powerDisableCharging() {
        gpio::outputFloat(AVR_PIN_CHARGE_EN);
    }

    static void powerVccUpdate(uint16_t value) {
        vcc_.addObservation(DeviceState::voltageToRawStorage(value));
        ts_.estate.setVccRaw(vcc_.value());
        if (!ts_.device.dcPower() && value > VCC_DC_POWER_THRESHOLD) {
            ts_.device.setDCPower(true);
            powerEnableCharging();
            // make sure the AVR is active if DC power is enabled
            powerAVRSetActive();
            rgbUpdateSystemEffect();
        }
        if (ts_.device.dcPower() && ts_.estate.vcc() < VCC_DC_POWER_THRESHOLD) {
            ASSERT(0x07, powerAVRActive() == true);
            powerDisableCharging();
            if (ts_.device.deviceMode() == DeviceMode::Sleep || ts_.device.deviceMode() == DeviceMode::PowerOff) {
                powerAVRTrySleep();
                ASSERT(0x08, powerAVRActive() == false);
            }
            rgbUpdateSystemEffect();
        }
        return;
        // see we should show low battery warning, or even turn the device off (from normal, debug, or even sleep modes)
        if (ts_.device.deviceMode() == DeviceMode::Normal || ts_.device.deviceMode() == DeviceMode::Debug) {
            if (vcc_.value() <= VCC_CRITICAL_THRESHOLD)
                powerOff();
            if (vcc_.value() <= VCC_WARNING_THRESHOLD)
                rgbUpdateSystemEffect();
        }  else if (ts_.device.deviceMode() == DeviceMode::Sleep) {
            if (vcc_.value() <= VCC_CRITICAL_THRESHOLD)
                powerOff();
        }
    }

    static void powerVBattUpdate(uint16_t value) {
        vBatt_.addObservation(DeviceState::voltageToRawStorage(value));
        ts_.device.setVBattRaw(vBatt_.value());
        // if this is the lipo charger version, as a precaution, check if the battery voltage is greater than charge cutoff and if so, terminate the charging
#ifdef RCKID_HAS_LIPO_CHARGER
        if (vBatt_.ready() && ts_.device.charging()) {
            value = ts_.device.vBatt();
            if (value >= RCKID_VBATT_CHARGE_CUTOFF_VOLTAGE) {
                error(AVR_ERROR_VBATT_TOO_HIGH);
                powerDisableCharging();
            }
        }
#endif
    }

    static void powerTempUpdate(uint16_t value) {
        ts_.estate.setTemp(value);
        // If we have lipo charger, and are currently charging & temperature is too high, abort the charging for safety 
#ifdef RCKID_HAS_LIPO_CHARGER
        if (ts_.device.charging() && value >= RCKID_VBATT_CUTOFF_TEMPERATURE) {
            error(AVR_ERROR_TEMP_TOO_HIGH);
            powerDisableCharging();
        }
#endif
    }

    //@}

    /** \name Home Button


     */
    /** Starts the long press countdown for the home button. 

        Since the  home button long presses has special meaning, its detection is handled by the AVR and this method is called by the GPIO ISR on home button press. If not counting already (for a simple debouncing), resets the long press detection counter. 

        If the long press would mean to power the device on (i.e. we are in PowerOff device mode), enters the Wakeup mode so that we can accurately time the press duration (in other wakeup modes this is not necessary as the accurate measurement is available). 
     */
    /** Called when the home button is released before the long press countdown ellapses. Resets the counter and if device was in wakeup mode, returns to power off mode. 
     */
    /** Home button long press check & action. 
     
        Checks if the long press countdown ellaped and if so performs the action, which is either to power off, or power on in normal, or in debug mode (if select button is pressed). 
     */
    //@{

    static inline uint16_t btnHomeCounter_ = 0;
    
    static void btnHomeDown() {
        if (RCKid::btnHomeCounter_ == 0) {
            RCKid::btnHomeCounter_ = BTN_HOME_LONG_PRESS_THRESHOLD;
            // we need the AVR to be active so that we can countdown the long press
            if (!powerAVRActive())
                powerAVRSetActive();
        }
    }

    static void btnHomeCheckLongPress() {
        // if button conter is zero, we are not tracking the long press and there is nothing to be done
        if (btnHomeCounter_ == 0)
            return;
        // if the home button is not pressed, cancel the long press. This is no-op in the normal or debug state, but in sleep & power off state, we try to put avr to sleep state 
        if (!ts_.device.btnHome()) {
            btnHomeCounter_ = 0;
            if (ts_.device.deviceMode() != DeviceMode::Normal && ts_.device.deviceMode() != DeviceMode::Debug)
                powerAVRTrySleep();
        }
        // otherwise decrement the counter and if we reached 0, perform the long press actiob
        if (--btnHomeCounter_ == 0)
            btnHomeLongPress();
    }

    static void btnHomeLongPress() {
        switch (ts_.device.deviceMode()) {
            case DeviceMode::Normal:
            case DeviceMode::Debug:
                powerOff();
                break;
            case DeviceMode::Sleep:
            case DeviceMode::PowerOff:
                powerOn();
                break;
            default:
                // TODO error
                break;
        }
    }
    //@}

    /** \name Button Matrix 

        Bus matrix has three rows (DPad, ABXY, which relly is A B and Select & Start) and Control (volume up & down keys).  
     
     
     */
    //@{

    static void initializeButtonMatrix() {
        // pull all buttons up
        gpio::setAsInputPullup(AVR_PIN_BTN_1);
        gpio::setAsInputPullup(AVR_PIN_BTN_2);
        gpio::setAsInputPullup(AVR_PIN_BTN_3);
        gpio::setAsInputPullup(AVR_PIN_BTN_4);
        // force all button groups to high 
        gpio::outputHigh(AVR_PIN_BTN_DPAD);
        gpio::outputHigh(AVR_PIN_BTN_ABXY);
        gpio::outputHigh(AVR_PIN_BTN_CTRL);
    }

    static void disableButtonMatrix() {
        gpio::setAsInputPullup(AVR_PIN_BTN_1);
        gpio::setAsInputPullup(AVR_PIN_BTN_2);
        gpio::setAsInputPullup(AVR_PIN_BTN_3);
        gpio::setAsInputPullup(AVR_PIN_BTN_4);
        gpio::setAsInputPullup(AVR_PIN_BTN_DPAD);
        gpio::setAsInputPullup(AVR_PIN_BTN_ABXY);
        gpio::setAsInputPullup(AVR_PIN_BTN_CTRL);
    }

    static void enableDPADButtons() {
        gpio::low(AVR_PIN_BTN_DPAD);
    }

    static void measureDPADButtons() {
        ts_.device.setDPadKeys(
            !gpio::read(AVR_PIN_BTN_2), // left
            !gpio::read(AVR_PIN_BTN_4), // right
            !gpio::read(AVR_PIN_BTN_1), // up
            !gpio::read(AVR_PIN_BTN_3) // down
        );
        gpio::high(AVR_PIN_BTN_DPAD);
    }

    static void enableABXYButtons() {
        gpio::low(AVR_PIN_BTN_ABXY);
    }

    static void measureABXYButtons() {
        ts_.device.setABXYKeys(
            !gpio::read(AVR_PIN_BTN_2), // a
            !gpio::read(AVR_PIN_BTN_1), // b
            !gpio::read(AVR_PIN_BTN_4), // sel
            !gpio::read(AVR_PIN_BTN_3) // start
        );
        gpio::high(AVR_PIN_BTN_ABXY);
    }

    static void enableVolumeButtons() {
        gpio::low(AVR_PIN_BTN_CTRL);
    }

    static void measureVolumeButtons() {
        ts_.device.setVolumeKeys(
            !gpio::read(AVR_PIN_BTN_2), // vol up
            !gpio::read(AVR_PIN_BTN_3) // vol down
        );
        gpio::high(AVR_PIN_BTN_CTRL);
    }

    //@}

    /** \name Device state and communications
     */
    //@{

    // Transferrable state containing everyting that can be send to the RP2040 via I2C. 
    static inline TransferrableState ts_;

    static inline uint8_t i2cTxId_ = 0;
    static inline uint8_t i2cRxId_ = 0;

    static inline void i2cInitialize() {
        cli();
        i2cTxId_ = 0;
        i2cRxId_ = 0;
        ts_.device.setDeviceBusy(false);
        i2c::initializeSlave(AVR_I2C_ADDRESS);
        // enable smart mode, enable TWI and enable STOP condition detection 
        // TODO do we really want to enable stop detection at this point? 
        TWI0.SCTRLA = TWI_PIEN_bm | TWI_SMEN_bm | TWI_ENABLE_bm;
        //TWI0.SCTRLA |= TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm;
        //TWI0.SCTRLA |= TWI_PIEN_bm | TWI_SMEN_bm;
        sei();

        // TODO deal with master too
    }

    static inline void i2cInitializeMaster() {
        // TODO
    }

    static inline void i2cDisable() {
        TWI0.SCTRLA = 0;
        TWI0.MCTRLA = 0;
    }

    /** The I2C interrupt handler. 

        DIF APIF CLKHOLD RXACK COLL BUSERR DIR AP

         1                                  1        tx to master
         1                                  0        rx to master
             1                              1  1     tx start
             1                              0  1     rx start
             1                              1  0     tx stop
             1                              0  0     rx stop

        CLKHOLD is 1 when the clock is stretched (always in the iterrupt), gets cleared by its own. RXACK is the last ACK/NACK from master, can be safely ignored for now?

        what is master does not want more and? 

        COLL is bus collision (could not set SDA high) - how to recover
        BUSERR is bus error (how to recover)
     
     */
    static inline uint8_t i2cIrqId_ = 0;
    static inline void i2cSlaveIRQHandler() /*__attribute__((always_inline)) */ {
        error(0, TWI0.SSTATUS);
        // this is from https://www.microchip.com/en-us/application-notes/an2634, uses smart mode, but sometimes leaves SDA low still, does not use interrupts
        if (TWI0.SSTATUS & (TWI_BUSERR_bm | TWI_COLL_bm)) {
            // BUS ERROR - reset the I2C 
            // COLLISION - since we do not use fancy I2C features, collision is to be handled the same way a bus error would
            error(AVR_ERROR_I2C_SLAVE, TWI0.SSTATUS);
            TWI0.SSTATUS = (TWI_BUSERR_bm | TWI_COLL_bm);
            i2cInitialize();
            // TODO maybe this is not needed? 
            return; 
        }
        if (TWI0.SSTATUS & TWI_APIF_bm) {
            // START - first disable the stop conditioon check (we will enable it when at least one byte is received from the master. If master wishes to read, always return ACK and contiue. If master wishes to write, return ACK if we are not busy, otherwise return NACK (the message would overwrite the currently processed command buffer. 
            if (TWI0.SSTATUS & TWI_AP_bm) {
                //TWI0.SCTRLA &= ~ TWI_PIEN_bm; 
                if (TWI0.SSTATUS & TWI_DIR_bm) {
                    i2cTxId_ = 0;
                    TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
                } else {
                    if (ts_.device.deviceBusy()) {
                        // TODO Microchip example only sends COMPTRANS - not NACK? 
                        TWI0.SCTRLB = TWI_ACKACT_NACK_gc | TWI_SCMD_COMPTRANS_gc;
                    } else {
                        ASSERT(0x09, i2cRxId_ == 0);
                        TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
                    }
                }
            // STOP - clear the stop condition detection flag, clear the APIF flag if we were receiving, set device busy flag to true to tell the main loop we have I2C command to process
            } else {
                //TWI0.SCTRLA &= ~TWI_PIEN_bm;
                // TODO why? is it smart mode related? 
                //TWI0.SSTATUS = TWI_APIF_bm;            
                // i guess yes, finished w/o sending new command                     
                TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
                if (i2cRxId_ != 0) {
                    ASSERT(0x10, ts_.device.deviceBusy() == false);
                    ts_.device.setDeviceBusy(true);
                }
            }
        }
        if (TWI0.SSTATUS & TWI_DIF_bm) {
            // SLAVE TRANSMIT - simply transmit next byte from the memory. Since we use uint8_t as the index, this automatically wraps at 256 bytes so if master is accepting slave can keep sending forever. However detect if master sent NACK after last byte, meaning no more bytes and terminate the transaction 
            if (TWI0.SSTATUS & TWI_DIR_bm) {
                if ((i2cTxId_ > 0) && (TWI0.SSTATUS & TWI_RXACK_bm)) {
                    TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
                } else {
                    TWI0.SDATA = ((uint8_t*)&ts_)[i2cTxId_];
                    ++i2cTxId_;
                    // smart mode will take care of the command
                }
            // SLAVE RECEIVE - when receiving from master, enable the stop condition detection so that we can set the busy flag and process the command. Then read the byte and store it to the comms buffer, wrapping around the buffer's size if too many data has been writen 
            } else {
                //TWI0.SCTRLA |= TWI_PIEN_bm;
                ts_.buffer[i2cRxId_] = TWI0.SDATA;
                if (++i2cRxId_ == sizeof(ts_.buffer))
                    i2cRxId_ = 0;
                // smart mode will fill in the command
            }
        }
    }
#ifdef MP3_PLAYER
    #define I2C_DATA_MASK (TWI_DIF_bm | TWI_DIR_bm) 
    #define I2C_DATA_TX (TWI_DIF_bm | TWI_DIR_bm)
    #define I2C_DATA_RX (TWI_DIF_bm)
    #define I2C_START_MASK (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm)
    #define I2C_START_TX (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm)
    #define I2C_START_RX (TWI_APIF_bm | TWI_AP_bm)
    #define I2C_STOP_MASK (TWI_APIF_bm | TWI_DIR_bm)
    #define I2C_STOP_TX (TWI_APIF_bm | TWI_DIR_bm)
    #define I2C_STOP_RX (TWI_APIF_bm)

    //digitalWrite(DEBUG_PIN, HIGH);
    uint8_t status = TWI0.SSTATUS;
    // sending data to accepting master is on our fastpath as is checked first
    if ((status & I2C_DATA_MASK) == I2C_DATA_TX) {
        TWI0.SDATA = i2cTxId_; //((uint8_t*)&ts_)[i2cTxId_];
        ++i2cTxId_;
        TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
    // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
    } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
        ts_.buffer[i2cRxId_] = TWI0.SDATA;
        if (++i2cRxId_ == sizeof(ts_.buffer))
            i2cRxId_ = 0;
        TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
    // master requests slave to write data, determine what data to send, send ACK if there is data to be transmitted, NACK if there is no data to send
    } else if ((status & I2C_START_MASK) == I2C_START_TX) {
        i2cTxId_ = 0;
        TWI0.SCTRLB = TWI_ACKACT_ACK_gc + TWI_SCMD_RESPONSE_gc;
    // master requests to write data itself. ACK if the buffer is empty (we do not support multiple commands in same buffer), NACK otherwise.
    } else if ((status & I2C_START_MASK) == I2C_START_RX) {
        if (i2cRxId_ == 0)
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
        else
            TWI0.SCTRLB = TWI_ACKACT_NACK_gc; // TODO should this be also SCMD_COMPTRANS? 
    // when a transmission finishes we must see if another transmission required and re-raise the irq flag for ESP. While recording this means we need to see if there is another 32 bytes available yet
    } else if ((status & I2C_STOP_MASK) == I2C_STOP_TX) {
        TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
    } else if ((status & I2C_STOP_MASK) == I2C_STOP_RX) {
        ts_.device.setDeviceBusy(true);
        TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
    } else {
        error(I2C_ERROR_SLAVE, status);
    }
    //digitalWrite(DEBUG_PIN, LOW);

#endif





        #ifdef FOOBAR



        //my attempt with interrupts & no smart mode, do everything by the book, from the v1 bootloader
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
        // sending data to accepting master is on our fastpath as is checked first, the next byte in the buffer is sent, wrapping the index on 256 bytes 
        if ((status & I2C_DATA_MASK) == I2C_DATA_TX) {
            TWI0.SDATA = i2cTxId_; //((uint8_t*)&ts_)[i2cTxId_];
            ++i2cTxId_;
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
        // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
        } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
            ts_.buffer[i2cRxId_] = TWI0.SDATA;
            if (++i2cRxId_ == sizeof(ts_.buffer))
                i2cRxId_ = 0;
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
        // master requests slave to write data, simply say we are ready as we always send the buffer
        } else if ((status & I2C_START_MASK) == I2C_START_TX) {
            i2cTxId_ = 0;
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
        // master requests to write data itself
        } else if ((status & I2C_START_MASK) == I2C_START_RX) {
            // TODO send NACK if we are busy
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
        // sending finished, there is nothing to do 
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_TX) {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        // receiving finished, if we are in command mode, process the command, otherwise there is nothing to be done 
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_RX) {
            ts_.device.setDeviceBusy(true);
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        // nothing to do, or error - not sure what to do, perhaps reset the bootloader? 
        } else if (status & TWI_BUSERR_bm) {
            error(AVR_ERROR_I2C_SLAVE, status);
        } else {
            //error(AVR_ERROR_I2C_SLAVE, status);
        }

        // TODO error is DIR_EN + RXACK (nacked slave write to master)
        // not sure what to do really - maybe go back to IRQ? 


        uint8_t status = TWI0.SSTATUS & ~(TWI_RXACK_bm | TWI_CLKHOLD_bm);
        if (status | TWI_BUSERR_bm) {
            TWI0.SSTATUS |= TWI_BUSERR_bm;
            status &= ~ TWI_BUSERR_bm;
            //BREAKPOINT(0xff);
            //i2c::initializeSlave(AVR_I2C_ADDRESS);
            //return;
        }

        ++i2cIrqId_;
        //if (i2cIrqId_ == 1)
        //    BREAKPOINT(status);
        switch (status) {
            case (TWI_DIF_bm | TWI_DIR_bm | TWI_AP_bm):
                TWI0.SDATA = i2cTxId_; //((uint8_t*)&ts_)[i2cTxId_];
                TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
                ++i2cTxId_;
                break;
            case (TWI_DIF_bm | TWI_AP_bm):
                ts_.buffer[i2cRxId_] = TWI0.SDATA;
                TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc; // TODO no need to send ackact? 
                if (++i2cRxId_ == sizeof(ts_.buffer))
                    i2cRxId_ = 0;
                break;
            case (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm): 
                i2cTxId_ = 0;
                TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
                break;
            case (TWI_APIF_bm | TWI_AP_bm):
                TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
                //TWI0.SCTRLB = ts_.device.deviceBusy() ? (TWI_ACKACT_bm | TWI_SCMD_COMPTRANS_gc) : TWI_SCMD_RESPONSE_gc;
                break;
            case (TWI_APIF_bm | TWI_DIR_bm):
                // TODO nothing to do now, but in multi-master mode, notify the main thread to talk to the current sensor now
                TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
                break;
            case (TWI_APIF_bm):
                if (i2cRxId_ != 0)
                    ts_.device.setDeviceBusy(true);
                TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
                break;
            default:
            // I';m getting buserror, not sure why?  -- see the order of statuses I am getting and go from there
                error(AVR_ERROR_I2C_SLAVE, status);
        }
        TWI0.SSTATUS = TWI_APIF_bm | TWI_DIF_bm;

#endif
        /*
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
        // on the fastpath, deal with data transfers. Sending data to accepting master is the easiest, simply send the current byte from the transferrable state and increment the index. It is up to the master to ensure valid lengths are read from AVR. Receiving data stores the byte to the comms buffer. If too much data has been send, simply wrap around the buffer
        if (status & TWI_DIF_bm) {
            if (status & TWI_DIR_bm) { // transfer
                TWI0.SDATA = i2cTxId_; //((uint8_t*)&ts_)[i2cTxId_];
                TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
                ++i2cTxId_;
            } else { // receive
                ts_.buffer[i2cRxId_] = TWI0.SDATA;
                TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc; // TODO no need to send ackact? 
                if (++i2cRxId_ == sizeof(ts_.buffer))
                    i2cRxId_ = 0;
            }
            return;
        }
        // then deal with the start condition. If master requests slave to write data, reset the tx offset to the beginning of transferrable state and acknowledge. For master sending data to slave, check that we are not busy and if not, start receiving data. 
        if ((status & (TWI_APIF_bm | TWI_AP_bm)) == (TWI_APIF_bm | TWI_AP_bm)) {
            if (status & TWI_DIR_bm) { // start of transfer (slave sends to master)
                i2cTxId_ = 0;
                TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
            } else { // start of receive (master sends to slave)
                TWI0.SCTRLB = ts_.device.deviceBusy() ? (TWI_ACKACT_bm | TWI_SCMD_COMPTRANS_gc) : TWI_SCMD_RESPONSE_gc;
            }
            return;
        }
        // deal with stop conditions. Master done receiving data does not do anything, while master being done writing data to slave sets the device busy if some bytes were transmitted so that the received command can be processed
        if ((status & (TWI_APIF_bm | TWI_AP_bm)) == TWI_APIF_bm) {
            if (status & TWI_DIR_bm) { // stop slave sending to master
                // TODO nothing to do now, but in multi-master mode, notify the main thread to talk to the current sensor now
                TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            } else { // stops master sending to slave
                if (i2cRxId_ != 0)
                    ts_.device.setDeviceBusy(true);
                TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            }
            return;
        }
        // deal with bus error 
        /   *
        if (status & TWI_BUSERR_bm) {
            TWI0.SSTATUS |= TWI_BUSERR_bm;
            return;
        }
        error(AVR_ERROR_I2C_SLAVE, status);
        */


    /*
        // sending data to accepting master is on the fastpath as this is what we do the most. Simply send next byte, it is up to the master to only read as much or as little as it needs
        if ((status & I2C_DATA_MASK) == I2C_DATA_TX) {
        // if we are receiving, store the byte to the buffer and move to next byte. If too much data has been sent, simply wrap around the buffer, leave it up to the master to send the appropriate number of bytes
        } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
            // TODO note somewhere that there has been an error? 
        // master requests slave to write data. Always start writing at the beginning of the transferrable state and always accept the read.  
        } else if ((status & I2C_START_MASK) == I2C_START_TX) {
        // master requests to send data. NACK if we are busy (i.e. still processing previously sent command), otherwise ACK - the i2c tx index has already been reset before we cleared the busy flag. 
        } else if ((status & I2C_START_MASK) == I2C_START_RX) {
            TWI0.SCTRLB = ts_.device.deviceBusy() ? TWI_ACKACT_NACK_gc : TWI_SCMD_RESPONSE_gc;
        // sending to master finished
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_TX) {
            // TODO nothing to do now, but in multi-master mode, notify the main thread to talk to the current sensor now
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        // receiving finished, check if we received at least one byte, and if yes, set busy flag to true so that the main loop can process the received command
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_RX) {
            if (i2cRxId_ != 0)
                ts_.device.setDeviceBusy(true);
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
        } else if (status & TWI_BUSERR_bm) {
            TWI0.SSTATUS |= TWI_BUSERR_bm;
        } else {
            // TODO do error properly
            // report the error 
            // status = 32
            error(AVR_ERROR_I2C_SLAVE, status);
            // and recover
        }
        */
    //}

    static inline void i2cProcessCommand() {
        // process the recived command
        switch (static_cast<cmd::Id>(ts_.buffer[0])) {
            case cmd::Nop::ID:
                break;
            case cmd::PowerOff::ID:
                powerOff();
                break;
            case cmd::Sleep::ID:
                sleep();
                break;
            case cmd::ResetRP::ID:
                powerRP2040Reset();
                break;
            case cmd::ResetAVR::ID:
                cpu::reset();
                // unreachable here
            case cmd::BootloaderRP::ID:
                powerRP2040Bootloader();
                break;
            case cmd::BootloaderAVR::ID:
                // TODO bootloader
            case cmd::DebugModeOn::ID:
                ts_.device.setDeviceMode(DeviceMode::Debug);
                pwmBacklight(128);
                break;
            case cmd::DebugModeOff::ID:
                ts_.device.setDeviceMode(DeviceMode::Normal);
                pwmBacklight(ts_.estate.brightness());
                break;
            case cmd::AudioOn::ID:
                ts_.device.setAudioEnabled(true);
                // TODO should this be input?
                gpio::outputFloat(AVR_PIN_HEADPHONES);
                break;
            case cmd::AudioOff::ID:
                ts_.device.setAudioEnabled(true);
                gpio::outputLow(AVR_PIN_HEADPHONES);
                break;
            case cmd::SetBrightness::ID: {
                uint8_t value = cmd::SetBrightness::fromBuffer(ts_.buffer).value;
                pwmBacklight(value);
                ts_.estate.setBrightness(value);
                break;
            }
            case cmd::SetTime::ID: {
                TinyDate t = cmd::SetTime::fromBuffer(ts_.buffer).value;
                ts_.time = t;
                break;
            }
            case cmd::GetNumAVRErrors::ID:
                ts_.buffer[0] = numErrors_ & 0xff;
                ts_.buffer[1] = numErrors_ >> 8;
                break;
            case cmd::GetAVRErrors::ID:
                errorCopyBuffer();
                break;
            case cmd::ClearAVRDeviceError::ID:
                ts_.device.setDeviceError(false);
                break;
            case cmd::ResetAVRErrors::ID:
                resetErrors();
                break;
            case cmd::Rumbler::ID: {
                auto & c = cmd::Rumbler::fromBuffer(ts_.buffer);
                pwmRumble(c.effect);
                break;
            }
            case cmd::RGBOff::ID:
                // just set all rgb effects to off, this will be picked up by rgbTick and the 5V power will be cutoff
                rgbSetEffect(0, RGBEffect::Off());
                rgbSetEffect(1, RGBEffect::Off());
                // 2 is system RGB effect that should not be user accessible
                rgbSetEffect(3, RGBEffect::Off());
                rgbSetEffect(4, RGBEffect::Off());
                rgbSetEffect(5, RGBEffect::Off());
                break;
            case cmd::SetRGBEffect::ID: {
                auto & c = cmd::SetRGBEffect::fromBuffer(ts_.buffer);
                rgbEffects_[c.index] = c.effect;
                power5v(true);
                break;
            }
            case cmd::SetRGBEffects::ID: {
                auto & c = cmd::SetRGBEffects::fromBuffer(ts_.buffer);
                rgbSetEffect(0, c.b);
                rgbSetEffect(1, c.a);
                // 2 is system RGB effec that should not be user accessible
                rgbSetEffect(3, c.dpad);
                rgbSetEffect(4, c.sel);
                rgbSetEffect(5, c.start);
                break;
            }
            default:
                error(AVR_ERROR_UNKNOWN_COMMAND, ts_.buffer[0]);
                break;
        }
        // when done, first reset the tx index and then clear the busy flag so that we can process other commands
        i2cRxId_ = 0;
        ts_.device.setDeviceBusy(false);
    }

    //@}

    /** \name Analog measurements
     
        Used to measure voltages (VCC and VBATT) and temperature, as well as checking the presence of headphones. We could in theory go digital, but since the VAudio is 3V, digital is a bit of a stretch and sampling the voltage on the headphones detect is safer option. Note that the headphones detect pin is also used to forcefully disable the speaker in which case the pin is configured as output pin and headphone detection does not work. 

        ADC0 is used for all measurements with 32 samples per measure and delay of 32 ADC cycles, both for better accuracy. With ADC clock prescaler of 8, at 8MHz this gives 1MHz for ADC clock ticks, one measurement takes 448 ADC cycles, so if a measurement starts at system tick, there is plenty time to finish it before the next system tick. When the ADC ready interrupt occurs, the flag is picked by the main loop and the ADC result ready function is called, which based on the muxpos for current measurement processes the value. 

        The VCC measurement is used to determine if DC power is plugged in, while the VBATT and Temperature measurements are monitoring the charging and can terminate if iether gets over what's deemed safe values. 
        
        The VCC and VBatt measurements fluctuate a lot, so we use a ring avg summary for 128 values to smooth the results a bit. This works very well when we take measurements roughly every 8ms (4 measurements taken in sequence at 2ms tick intervals), but is much harder in power off and standby modes where the measurement frequency is actually 1 second, ie filling the ring and being ready to answer would take more than 2 minutes.  
     */
    //@{

    /** Ring buffers with running average to prevent vcc and vbatt glitches. 
     */
    static inline RingAvg<uint8_t, 128> vcc_;
    static inline RingAvg<uint8_t, 128> vBatt_;

    static void adcInitialize() {
        ADC0.CTRLB = ADC_SAMPNUM_ACC32_gc;
        ADC0.CTRLD = ADC_INITDLY_DLY32_gc;
        ADC0.SAMPCTRL = 31;
        // set voltage reference to 1v1 for temperature checking
        VREF.CTRLA &= ~ VREF_ADC0REFSEL_gm;
        VREF.CTRLA |= VREF_ADC0REFSEL_1V1_gc;
    }

    static void adcDisable() {
        // turn the ADC off and clear any result so that we do not get spurious readouts
        ADC0.CTRLA = 0;
        ADC0.INTFLAGS = ADC_RESRDY_bm;
    }

    static void measureVCC() {
        // sample internal voltage reference using VDD for reference to determine VCC 
        ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
        ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;
        ADC0.COMMAND = ADC_STCONV_bm;
    }

    static void measureVBatt() {
        // sample the battery voltage using VDD ref 
        ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
        ADC0.MUXPOS = gpio::getADC0muxpos(AVR_PIN_VBATT);
        ADC0.COMMAND = ADC_STCONV_bm;
    }

    /** NOTE: this should only be used when the audio is enabled, otherwise the headphones pin in configured as output to force the speaker to be off. 
     */
    static void measureHeadphones() {
        // sample headphones
        ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
        ADC0.MUXPOS = gpio::getADC0muxpos(AVR_PIN_HEADPHONES);
        ADC0.COMMAND = ADC_STCONV_bm;
    }

    static void measureTemperature() {
        ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_INTREF_gc | ADC_SAMPCAP_bm;
        ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;
        ADC0.COMMAND = ADC_STCONV_bm;
    }

    static void adcDone() __attribute__((always_inline)) {
        uint16_t value = ADC0.RES / 32;
        uint8_t muxpos = ADC0.MUXPOS;
        switch (muxpos) {
            case ADC_MUXPOS_INTREF_gc: {
                // convert the sampled value to VCC * 100 (i.e. 10mV resolution)
                value = 110 * 512 / value;
                value = value * 2;
                // let the power management subsystem decide what to do
                powerVccUpdate(value);
                break;
            }
            case gpio::getADC0muxpos(AVR_PIN_VBATT): {
                // convert the battery reading to voltage. The battery reading is relative to vcc, which we already have
                // VBATT = VCC * VBATT / 255 
                // but 500 * 255 is too high to fit in uint16, so we divide VCC by 2 and then divide by 128 instead
                value >>= 2; // go for 8bit precision, which should be enough
                value = (ts_.estate.vcc() / 2 * value)  / 128; 
                // let the power management subsystem decide what to do
                powerVBattUpdate(value);
                break;
            }
            case gpio::getADC0muxpos(AVR_PIN_HEADPHONES): {
                ts_.device.setHeadphones(value < HEADPHONES_DETECTION_THRESHOLD);
                break;
            }
            case ADC_MUXPOS_TEMPSENSE_gc: {
                // clauclate the temperature (code from microchip's example)
                int8_t sigrow_offset = SIGROW.TEMPSENSE1; 
                uint8_t sigrow_gain = SIGROW.TEMPSENSE0;
                int32_t t = value - sigrow_offset; // Result might overflow 16 bit variable (10bit+8bit)
                t *= sigrow_gain;
                // temp is now in kelvin range, to convert to celsius, remove -273.15 (x256)
                t -= 69926;
                // and now loose precision to 0.5C (x10, i.e. -15 = -1.5C)
                value = (t >>= 7) * 5;
                // let the power management system decide what to do 
                powerTempUpdate(value);
                break;
            }
            default:
                UNREACHABLE;
                break;
        }
        // turn the ADC off to conserve power
        ADC0.CTRLA = 0;
        // if we are in sleep of poweroff mode, we can now go to deep sleep since the ADC is finished
        switch (ts_.device.deviceMode()) {
            case DeviceMode::Sleep:
            case DeviceMode::PowerOff:
                // since ADC is disabled in power down mode, set sleep mode to Standby
                set_sleep_mode(SLEEP_MODE_PWR_DOWN);
                break;
            default:
                break;
        }
    }
    //@}

    /** \name PWM  
     
        The PWM signals used for backlight and rumbler control are generated by the TCB0 and TCB1 respectively. Both PWM pins are pulled low externally and are active high (MOSFET low side switches). 
     */
    //@{

    static inline RumblerEffect rumblerEffect_;
    static inline RumblerEffect rumblerCurrent_;

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

    static void pwmDisable() {
        TCB0.CTRLA = 0;
        static_assert(AVR_PIN_PWM_BACKLIGHT == A5); // TCB0 WO
        gpio::outputFloat(AVR_PIN_PWM_BACKLIGHT);
        TCB1.CTRLA = 0;
        static_assert(AVR_PIN_PWM_RUMBLER == A3); //TCB1 WO
        gpio::outputFloat(AVR_PIN_PWM_RUMBLER);
    }

    static void pwmBacklight(uint8_t value) {
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
            TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm | TCB_RUNSTDBY_bm;
        }
    }

    static void pwmRumble(uint8_t value) {
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
            TCB1.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm | TCB_RUNSTDBY_bm;
       }
    }

    static void pwmRumble(RumblerEffect const & effect) {
        if (effect.cycles > 0 && effect.strength > 0) {
            rumblerEffect_ = effect;
            //--rumblerEffect_.cycles;
            rumblerCurrent_ = rumblerEffect_;
            rumblerCurrent_.timeOn = 0;
            rumblerCurrent_.timeOff = 0;
        } else {
            rumblerEffect_ = RumblerEffect::Off();
            rumblerCurrent_ = RumblerEffect::Off();
        }
    }

    static void pwmRumbleTick() {
        if (rumblerCurrent_.strength != 0) {
            if (rumblerCurrent_.timeOn > 0) {
                if (--rumblerCurrent_.timeOn > 0) 
                    return;
                else 
                    pwmRumble(0);
            }
            if (rumblerCurrent_.timeOff > 0) {
                if (--rumblerCurrent_.timeOff > 0)
                    return;
            }
            if (rumblerCurrent_.cycles != 0) {
                --rumblerEffect_.cycles;
                rumblerCurrent_ = rumblerEffect_;
                if (rumblerCurrent_.timeOn > 0)
                    pwmRumble(rumblerCurrent_.strength);
                return;
            }
            // we are done
            rumblerEffect_ = RumblerEffect::Off();
            rumblerCurrent_ = RumblerEffect::Off();
        }
    }
    //@}

    /** \name RGB LEDs  
     
        RCKid has in total 6 addressable RGB LEDs, 5 under buttons (dpad, A, B, Selct & Start can all be individually addressed) and one LED above the display for notifications. The keypad LEDs are user controllable from RP2040 via I2C commands, while the notification LED is controlled by the AVR. 

        The notification LED has the following modes:

        - red pulses mean low battery level
        - blue pulses mean DC power plugged in & device is charging
        - green pulses mean DC power plugged in & the device is fully charged
        - red continuous means AVR error (only in debug mode)


     */
    //@{

    static inline platform::NeopixelStrip<6> rgbs_{AVR_PIN_RGB}; 
    static inline platform::ColorStrip<6> rgbsTarget_;
    static inline RGBEffect rgbEffects_[6];

    static void rgbDisable() {
        power5v(false);
        for (uint8_t i = 0; i < 6; ++i) {
            rgbs_[i] = platform::Color::Black();
            rgbsTarget_[i] = platform::Color::Black();
            rgbEffects_[i] = RGBEffect::Off();
        }
    }

    static void rgbTick() {
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
        // force the update first (the LEDs may have some memory even after power down)
        rgbs_.update(true);
        // if all the LEDs are off, turn the 5V rail off to save power, otherwise update the LEDs
        if (turnOff)
            power5v(false);
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

    static void rgbSetEffect(uint8_t led, RGBEffect const & effect) {
        // if we are setting effect that's already there, don't do anything except update the duration to new value so that we don't make the effect flicker
        if (rgbEffects_[led].isSameAs(effect)) {
            rgbEffects_[led].duration = effect.duration;
            return;
        }
        rgbEffects_[led] = effect;
        // if the effect is visible and the RGB powerline is not on, turn the power on now
        if (effect.kind != RGBEffect::Kind::Off && !power5v())
            power5v(true);
        // the other case (i.e. all leds are off now and the power rail should be turned off) is handled by the rgb ticks
    }

    static void rgbUpdateSystemEffect() {
        if (ts_.device.deviceError())
            rgbSetEffect(2, RGB_ERROR_EFFECT);
        // TODO remove this
        //else if (ts_.device.deviceMode() == DeviceMode::Debug)
        //    rgbSetEffect(2, RGBEffect::Breathe(platform::Color::Red(), 8));
        else if (ts_.device.charging())
            rgbSetEffect(2, RGB_CHARGING_EFFECT);
        else if (ts_.device.dcPower())
            rgbSetEffect(2, RGB_CHARGING_DONE_EFFECT);
        else if (ts_.estate.vcc() < VCC_WARNING_THRESHOLD)
            rgbSetEffect(2, RGB_LOW_BATTERY_EFFECT);
        else
            rgbSetEffect(2, RGBEffect::Off());
    }

    //@}
}; // RCKid

// we need the interrupt to wake the chip up from sleep modes, but the actual ISR does nothing
ISR(RTC_PIT_vect) {}

// we need the interrupt to wake up the chip from sleep modes
static_assert(AVR_PIN_BTN_HOME == B2);
ISR(PORTB_PORT_vect) {}

int main() {
    RCKid::initialize();
    RCKid::loop();
} 


#ifdef FOOBAR

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

    /** Ring buffers with running average to prevent vcc and vbatt glitches. 
     */
    static inline RingAvg<uint8_t, 128> vcc_;
    static inline RingAvg<uint8_t, 128> vBatt_;

    /** Counter to measure the current consumed by the device by the INA219 to guarantee we will keep measuring even if RP2040 has issues talking to the AVR, which generally triggers the measurement otherwise to limit I2C bus congestion.
     */
    static inline uint8_t iMeasureCounter_ = 0;

#if (RCKID_INA219_I2C_ADDRESS != 0)
    static inline platform::INA219 ina_{RCKID_INA219_I2C_ADDRESS};
#endif

    static inline platform::NeopixelStrip<6> rgbs_{AVR_PIN_RGB}; 
    static inline platform::ColorStrip<6> rgbsTarget_;
    static inline RGBEffect rgbEffects_[6];
    static inline volatile bool rgbTick_ = false;
    // backup for user specified notification LED effect in case system effect is active
    static inline RGBEffect systemEffectBackup_;
    // if active, user specified effect for notification LED is in the systemEffectBackup and the effect proper is controlled by the system (low voltage or charging)
    static inline bool systemEffectActive_ = false;

    static inline RumblerEffect rumblerEffect_;
    static inline RumblerEffect rumblerCurrent_;
    static inline volatile bool rumblerTick_ = false;

    /** Determines if the AVR needs to sleep in standby mode alone, or if the power-down mode can be used. A non-zero value requires standby sleep mode. 
     */
    static inline uint8_t standbyRequired_ = 0;
    static constexpr uint8_t STANDBY_REQUIRED_BRIGHTNESS = 1;
    static constexpr uint8_t STANDBY_REQUIRED_RUMBLER = 2;
    static constexpr uint8_t STANDBY_REQUIRED_ADC0 = 4;
    
    /** Reset mode, which is decided in the interrupt handler for the buttons, but should be executed from the main loop due to the long wait times which should not happen in IRQ handlers. 
     */
    static inline uint8_t resetMode_ = 0;
    static constexpr uint8_t RESET_MODE_RP_RESET = 1;
    static constexpr uint8_t RESET_MODE_RP_BOOTLOADER = 2;

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
        while (WDT.STATUS & WDT_SYNCBUSY_bm); 
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
        /*
        rgbEffects_[0] = RGBEffect::Rainbow(0, 50, 1, 128);
        rgbEffects_[1] = RGBEffect::Rainbow(75, 40, 1, 128);
        rgbEffects_[3] = RGBEffect::Rainbow(180, 30, 1, 128);
        rgbEffects_[4] = RGBEffect::Rainbow(110, 20, 1, 128);
        rgbEffects_[5] = RGBEffect::Rainbow(230, 10, 1, 128);
        */
        platform::Color x = platform::Color::White();
        switch (ts_.error) {
            case AVR_POWER_ON:
                // leave white
                break;        
            case AVR_ERROR_WDT:
                x = platform::Color::Purple();    
                break;
            case AVR_ERROR_BOD:
                x = platform::Color::Blue();
                break;
            default:
                x = platform::Color::Red();
        }
        x = x.withBrightness(32);
        rgbEffects_[0] = RGBEffect::Solid(x, 1);
        rgbEffects_[1] = RGBEffect::Solid(x, 1);
        rgbEffects_[3] = RGBEffect::Solid(x, 1);
        rgbEffects_[4] = RGBEffect::Solid(x, 1);
        rgbEffects_[5] = RGBEffect::Solid(x, 1);
        rgbs_.update();

    }

    /** The main loop. 
     
        Making the AVR code really simple, the main loop simply runs and services any interrupts, while sleeping between the runs to conserve as much power as possible. 
     */
    static void loop() {
#if (defined RCKID_AVR_DEBUG_OLED_DISPLAY)
        oled_.clear32();
#endif
        while (true) {
            cpu::wdtReset();
            // if there is I2C message, process
            if (i2cCommandReady_)
                processI2CCommand();
#if (RCKID_INA219_I2C_ADDRESS != 0)
            if (power3v3Active() && iMeasureCounter_ == 0) {
                ts_.state.setCurrent(ina_.current()); // ina_.initialize(platform::INA219::Gain::mv_40, 10);
                iMeasureCounter_ = RCKID_CURRENT_SENSE_TIMEOUT_TICKS;
            }
#endif
            if (rgbTick_)
                rgbTick();
            if (rumblerTick_)
                rumblerTick();
            if (resetMode_ != 0) {
                switch (resetMode_) {
                    case RESET_MODE_RP_RESET:
                        rpReset();
                        break;
                    case RESET_MODE_RP_BOOTLOADER:
                        rpBootloader();
                        break;
                    default:
                        // TODO invalid reset mode
                        break;
                }
                resetMode_ = 0;
            }

            // make sure interrupts are enabled or we won't wake up, the appropriate sleep mode has already been set by the various peripheral interactions so we can happily go to sleep here as long as the current mode allows (normal, debug and bootloader mode don't work with sleep at all as we need the I2C active)
            // TODO perhaps we can extend the sleep mode to some of those modes as well by having a flag if we have an active I2C transaction
            sei();
            sleep_enable();
            switch (ts_.state.deviceMode()) {
                case DeviceMode::Normal:
                case DeviceMode::Debug:
                case DeviceMode::Bootloader:
                    break;
                default:
                    sleep_cpu();
            }
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
            case DeviceMode::Debug:
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
                if (ts_.state.btnSel())
                    ts_.state.setDeviceMode(DeviceMode::Debug);
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
                // clear RGB effects, no need to clear #2 as it gets handled by the charging if enabled
                rgbEffects_[0] = RGBEffect::Off();
                rgbEffects_[1] = RGBEffect::Off();
                rgbEffects_[3] = RGBEffect::Off();
                rgbEffects_[4] = RGBEffect::Off();
                rgbEffects_[5] = RGBEffect::Off();
                rgbs_.fill(platform::Color::RGB(0,0,0));
                rgbsTarget_.fill(platform::Color::RGB(0,0,0));
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
            // if the 5V rail is on, do RGB tick roughly 30 times per second
            if (power5vActive() && (tick_ % 32) == 0)
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
            if (ts_.state.dcPower()) {
                bool charging = !gpio::read(AVR_PIN_CHARGING);
                if (ts_.state.charging() != charging) {
                    ts_.state.setCharging(charging);
                    setSystemEffect(RGBEffect::Breathe(
                        charging ? platform::Color::RGB(0, 0, 16) : platform::Color::RGB(0, 16, 0),
                        1)
                    );
                }
            }
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
                        // sample headphones
                        ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
                        ADC0.MUXPOS = gpio::getADC0muxpos(AVR_PIN_HEADPHONES);
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
                    if (ts_.device.debugMode()) {
                        if (ts_.device.btnVolUp())
                            resetMode_ = RESET_MODE_RP_RESET;
                        else if (ts_.device.btnVolDown())
                            resetMode_ = RESET_MODE_RP_BOOTLOADER;
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
            oled_.write(64, 1, ts_.state.current(), ' ');
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
                vcc_.addObservation(State::voltageToRawStorage(value));
                ts_.state.setVccRaw(vcc_.value());
                if (systemTicksActive()) {
                    if (vcc_.ready()) {
                        value = ts_.state.vcc();
                        // check power events
                        if (ts_.state.deviceMode() != DeviceMode::PowerOff) {
                            if (value < VCC_CRITICAL_THRESHOLD)
                                criticalBattery();
                            else if (value < VCC_WARNING_THRESHOLD && (systemEffectActive_ == false))
                                setSystemEffect(RGBEffect::Breathe(platform::Color::RGB(16, 0, 0), 1));
                        }
                        if (value > VCC_DC_POWER_THRESHOLD)
                            dcPowerPlugged();
                        else if (value < VCC_DC_POWER_THRESHOLD)
                            dcPowerUnplugged();
                    }
                } else {
                    if (value >= VCC_DC_POWER_THRESHOLD) {
                        dcPowerPlugged();
                        vcc_.reset();
                    }
                }
                break;
            case gpio::getADC0muxpos(AVR_PIN_VBATT):
                // convert the battery reading to voltage. The battery reading is relative to vcc, which we already have
                // VBATT = VCC * VBATT / 255 
                // but 500 * 255 is too high to fit in uint16, so we divide VCC by 2 and then divide by 128 instead
                value >>= 2; // go for 8bit precision, which should be enough
                value = (ts_.state.vcc() / 2 * value)  / 128; 
                vBatt_.addObservation(State::voltageToRawStorage(value));
                ts_.state.setVBattRaw(vBatt_.value());
                if (vBatt_.ready()) {
                    value = ts_.state.vBatt();
#ifdef RCKID_HAS_LIPO_CHARGER
                    if (ts_.state.charging() && value >= RCKID_VBATT_CHARGE_CUTOFF_VOLTAGE)
                        disableCharging();
#endif
                }
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
        rgbs_.fill(platform::Color::Black());
        for (int i = 0; i < 5; ++i) {
            rgbs_[i < 2 ? i : i + 1] = platform::Color::Red().withBrightness(32);
            rgbs_.update();
            cpu::delayMs(200);
            cpu::wdtReset();
        }
        power3v3(true);
        power5v(false);
        setBacklightPWM(128);
    }

    static void rpBootloader() {
        // disable i2c
        i2c::disable();

        setBacklightPWM(0);
        cli();
        power3v3(false);
        power5v(true);
        rgbs_.fill(platform::Color::Black());
        rgbs_[0] = platform::Color::Green().withBrightness(32);
        rgbsTarget_[0] = platform::Color::Green().withBrightness(32);
        rgbs_.update();
        cpu::wdtReset();
        cpu::delayMs(200);
        rgbs_[1] = platform::Color::Green().withBrightness(32);
        rgbsTarget_[1] = platform::Color::Green().withBrightness(32);
        rgbs_.update();
        cpu::wdtReset();
        cpu::delayMs(200);
        gpio::outputLow(AVR_PIN_QSPI_SS);
        rgbs_[3] = platform::Color::Green().withBrightness(32);
        rgbsTarget_[3] = platform::Color::Green().withBrightness(32);
        rgbs_.update();
        cpu::wdtReset();
        cpu::delayMs(200);
        power3v3(true);
        rgbs_[4] = platform::Color::Green().withBrightness(32);
        rgbsTarget_[4] = platform::Color::Green().withBrightness(32);
        rgbs_.update();
        cpu::wdtReset();
        cpu::delayMs(200);
        rgbs_[5] = platform::Color::Green().withBrightness(32);
        rgbsTarget_[5] = platform::Color::Green().withBrightness(32);
        rgbs_.update();
        cpu::wdtReset();
        cpu::delayMs(150);
        gpio::outputFloat(AVR_PIN_QSPI_SS);
        cpu::wdtReset();
        cpu::delayMs(50);
        // keep the green lights on, the RP2040 app that has just been uploaded should turn them off
        rgbEffects_[0] = RGBEffect::Breathe(platform::Color::Green().withBrightness(32), 1);
        rgbEffects_[1] = RGBEffect::Breathe(platform::Color::Green().withBrightness(32), 1);
        rgbEffects_[3] = RGBEffect::Breathe(platform::Color::Green().withBrightness(32), 1);
        rgbEffects_[4] = RGBEffect::Breathe(platform::Color::Green().withBrightness(32), 1);
        rgbEffects_[5] = RGBEffect::Breathe(platform::Color::Green().withBrightness(32), 1);
        sei();
        // re-enable slave
        i2c::initializeSlave(AVR_I2C_ADDRESS);
        TWI0.SCTRLA |= TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm;
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
        // enable charging notification light
        setSystemEffect(RGBEffect::Breathe(platform::Color::RGB(0,0,16), 1));
        // set output low to enable charging
        gpio::outputLow(AVR_PIN_CHARGE_EN);
        // set charging on
        ts_.state.setCharging(true); // we assume this
        if (!systemTicksActive())
            startSystemTicks();
#endif
    }

    static void dcPowerUnplugged() {
        // don't do anything if already unplugged
        if (!ts_.state.dcPower())
            return;
        // otherwise change state and disable the charger monitoring circuitry
        ts_.state.setDCPower(false);
#ifdef RCKID_HAS_LIPO_CHARGER
        if (ts_.state.deviceMode() == DeviceMode::PowerOff) {
            stopSystemTicks();
            power5v(false);
        }
        ts_.state.setCharging(false);
        // disable charging
        gpio::outputFloat(AVR_PIN_CHARGE_EN);
        // disable the effect
        setSystemEffect(RGBEffect::Off());
#endif
    }

    /** Goes to the power off mode immediately while flashing some red colors. 
     
        Since this is called from the ADC0 done method, it will fire even when in the wakeup phase we detect a too low voltage. 
     */
    static void criticalBattery() {
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

    /** Programmatically disables the charger. 
     
        As per the charger's datasheet, section 5.2.2, charging can be terminated by applying logic 1 to the charger's prog pin. 
     */
    static void disableCharging() {
        //gpio::outputFloat(AVR_PIN_CHARGE_EN);
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
            allowSleepPowerDown(STANDBY_REQUIRED_BRIGHTNESS);
        } else if (value == 255) {
            TCB0.CTRLA = 0;
            TCB0.CTRLB = 0;
            gpio::outputHigh(AVR_PIN_PWM_BACKLIGHT);
            allowSleepPowerDown(STANDBY_REQUIRED_BRIGHTNESS);
        } else {
            gpio::outputLow(AVR_PIN_PWM_BACKLIGHT);
            TCB0.CCMPH = value;
            TCB0.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
            TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm | TCB_RUNSTDBY_bm;
            requireSleepStandby(STANDBY_REQUIRED_BRIGHTNESS);
        }
    }

    static void setRumblerPWM(uint8_t value) {
        if (value == 0) {
            TCB1.CTRLA = 0;
            TCB1.CTRLB = 0;
            gpio::outputFloat(AVR_PIN_PWM_RUMBLER);
            allowSleepPowerDown(STANDBY_REQUIRED_RUMBLER);
        } else if (value == 255) {
            TCB1.CTRLA = 0;
            TCB1.CTRLB = 0;
            gpio::outputHigh(AVR_PIN_PWM_RUMBLER);
            allowSleepPowerDown(STANDBY_REQUIRED_RUMBLER);
        } else {
            gpio::outputLow(AVR_PIN_PWM_RUMBLER);
            TCB1.CCMPH = value;
            TCB1.CTRLB = TCB_CNTMODE_PWM8_gc | TCB_CCMPEN_bm;
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

    static void setSystemEffect(RGBEffect const & effect) {
        if (effect.kind == RGBEffect::Kind::Off) {
            if (systemEffectActive_)
                rgbEffects_[2] = systemEffectBackup_;
            systemEffectActive_ = false;
        } else {
            if (!systemEffectActive_)
                systemEffectBackup_ = rgbEffects_[2];
            rgbEffects_[2] = effect;
            power5v(true);
            systemEffectActive_ = true;
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
            rgbs_[2] = platform::Color::Green().withBrightness(32);
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
        } else if (status & TWI_BUSERR_bm) {
            rgbs_[3] = platform::Color::Red().withBrightness(32);
            TWI0.SSTATUS |= TWI_BUSERR_bm;
        } else {
            // error - a state we do not know how to handle
            rgbs_[3] = platform::Color::White().withBrightness(32);

        }
    }

    static void processI2CCommand() {
        rgbs_[2] = platform::Color::Red().withBrightness(32);
        rgbs_.update();
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
                NO_ISR(ts_.state.setDeviceMode(DeviceMode::Debug));
                break;
            case cmd::DebugModeOff::ID:
                NO_ISR(ts_.state.setDeviceMode(DeviceMode::Normal));
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
                if (c.index == 2 && systemEffectActive_)
                    systemEffectBackup_ = c.effect;
                else 
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


#endif