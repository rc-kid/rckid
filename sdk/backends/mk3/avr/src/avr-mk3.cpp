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

// TODO  
// flag that can be used to enable breadboard, or device build. The breadboard is only useful while prototyping all breadboard guarded code should eventually disappear
#define BREADBOARD

#undef ASSERT
#define ASSERT(...) if (!(__VA_ARGS__)) { debugWrite() << "ASSERT " << __LINE__ << "\r\n"; }

#define LOG(...) debugWrite() << __VA_ARGS__ << "\r\n";

/** \name Debugging support. 
 
    To ease firmware development, the AVR chip supports serial port (only TX) to the programming header (together with UPDI and SWD) as well as using the RGB LEDs to indicate system state.
    */
//@{

inline Writer debugWrite() {
    return Writer(serial::write);
}
//@}

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

    NOTE: There is an errate for attiny1616 and smaller chips that states HW bug where turning off RTC turns off PIT as well, which means those chips will *not* work with RCKid as we use RTC for the system tick and PIT for the timekeeping.

    # Breadboard testing

    For breadboard testing, this is the layout of the ATTiny1616 chip in SOIC package, where the following pins are missing: 5V_ON, ACCEL_INT, BTN3, BTN2. 

               VDD -|============|- GND
            VDD_EN -|            |- PWM_RUMBLER
             BTN_4 -|            |- PWR_INT
               RGB -|            |- AVR_TX
          BTN_ABXY -| ATTiny1616 |- <UPDI>
          BTN_CTRL -|            |- AVR_INT
             BTN_1 -|            |- BTN_DPAD
           <TOSC1> -|            |- QSPI_SS
           <TOSC2> -|            |- PWM_BACKLIGHT
           I2C_SDA -|============|- I2C_SCL

    NOTE: Since the missing pins are effectively floating when in input mode, we cannot use the accel in breadboard mode testing. 

 */
#define AVR_PIN_AVR_TX          gpio::A1
#define AVR_PIN_PWR_INT         gpio::A2
#define AVR_PIN_PWM_RUMBLER     gpio::A3
#define AVR_PIN_VDD_EN          gpio::A4
#define AVR_PIN_BTN_4           gpio::A5
#define AVR_PIN_RGB             gpio::A6
#define AVR_PIN_BTN_ABXY        gpio::A7
#define AVR_PIN_I2C_SCL         gpio::B0
#define AVR_PIN_I2C_SDA         gpio::B1
#define AVR_PIN_BTN_1           gpio::B4
#define AVR_PIN_BTN_CTRL        gpio::B5
#define AVR_PIN_BTN_3           gpio::B6
#define AVR_PIN_BTN_2           gpio::B7
#define AVR_PIN_PWM_BACKLIGHT   gpio::C0
#define AVR_PIN_QSPI_SS         gpio::C1
#define AVR_PIN_BTN_DPAD        gpio::C2
#define AVR_PIN_AVR_INT         gpio::C3
#define AVR_PIN_5V_ON           gpio::C4
#define AVR_PIN_ACCEL_INT       gpio::C5

using namespace rckid;

/** The RCKid firmware
 
    The firmware is rather simple in operation, especially compared to mkII as the AVR does not have to deal with that many things - when powered on, it acts as a simple IO controller for buttons, baclight and rumbler. When powered off, the can act as a master of the I2C bus (its subsets that includes the always on devices - power management and sensors) and simply has to react to their interrupts. 

    Internally all functionality happens in the main loop and the interrupt handlers only pass status flags to it - with the exception of I2C communication which also sends or fills in the communications buffer and the state itself. For more details see the subsystems below:

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
    #ifdef BREADBOARD
        RTC.CLKSEL = RTC_CLKSEL_INT32K_gc; // run from the internal 32.768kHz oscillator
    #else
        RTC.CLKSEL = RTC_CLKSEL_TOSC32K_gc; // run from the external 32.768kHz oscillator
    #endif
        RTC.PITINTCTRL |= RTC_PI_bm; // enable the PIT interrupt
        while (RTC.PITSTATUS & RTC_CTRLBUSY_bm);
        RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc | RTC_PITEN_bm;

        // initializes the AVR TX pin for serial debugging
        serial::setAlternateLocation(true);
        serial::initializeTx(RCKID_SERIAL_SPEED);

        // TODO some initialization routine with checks, etc.
        LOG("SYSTEM RESET DETECTED: " << hex(RSTCTRL.RSTFR));
        initializeMasterMode();
        // TODO scanning I2C devices hangs (!)
        detectI2CDevices();
        // setup PMIC, everything else can be setup by the RP2350 when powered on
        // TODO  

        // and start the device in powerOff mode, i.e. set sleep mode and start checking ctrl group interrupts
        stopSystemTicks();
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
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
            // wait for any TX transmissions before going to sleep 
            serial::waitForTx();
            // After everything was processed, go to sleep - we will wake up with the ACCEL or PMIC interrupts, or if in power off mode also by the HOME button interrupt
            cpu::sei();
            sleep_enable();
            sleep_cpu();
        }
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
    }

    /** Initializes the device power on state. 
     */
    static void powerOn() {
        LOG("power on");
        NO_ISR(
            // set power mode, which also ensures system ticks are running
            setPowerMode(POWER_MODE_ON);
            powerVDD(true);
            // initialize the PWM subsystem for baclight & rumbler
            initializePWM();
            setBacklightPWM(state_.brightness);
            // start the I2C slave as we will be contacted by the RP2350 shortly
            i2c::initializeSlave(RCKID_AVR_I2C_ADDRESS);
            TWI0.SCTRLA |= TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm;
            ADC0.CTRLA |= ADC_RUNSTBY_bm;
        );
    }

    /** Initializes the device power off state. 
     */
    static void powerOff() {
        LOG("power off");
        NO_ISR(
            clearPowerMode(POWER_MODE_ON);
            powerVDD(false);
            // make the AVR_INT floating so that we do not leak any voltage to the now off RP2350
            gpio::outputFloat(AVR_PIN_AVR_INT);
        );
    }

    static void chargerConnected() { 
        setPowerMode(POWER_MODE_CHARGING | POWER_MODE_DC);
        setNotification(RGBEffect::Breathe(platform::Color::Blue().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), 1));
    }

    static void chargerDisconnected() {
        // no longer in DC mode, go back to system notification for debug mode, if active, or turn system notification off
        if (state_.status.debugMode())
            setNotification(RGBEffect::Solid(platform::Color::Red().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), 1));
        else
            setNotification(RGBEffect::Off());
        clearPowerMode(POWER_MODE_CHARGING | POWER_MODE_DC);
    }

    static void chargerDone() {
        clearPowerMode(POWER_MODE_CHARGING);
        setNotification(RGBEffect::Breathe(platform::Color::Green().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), 1));
    }

    static void chargerError() {
        // TODO what to do when charger is in error state? 
    }

    static void lowBatteryWarning() {
        setNotification(RGBEffect::Breathe(platform::Color::Red().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), 1));
    }

    static void enterDebugMode() {
        LOG("Debug mode on");
        state_.status.setDebugMode(true);
        if (! (powerMode_ & POWER_MODE_DC))
            setNotification(RGBEffect::Solid(platform::Color::White().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), 1));
        if (state_.brightness < 128)
            setBacklightPWM(128);
    }

    static void leaveDebugMode() {
        LOG("Debug mode off");
        state_.status.setDebugMode(false);
        if (! (powerMode_ & POWER_MODE_DC))
            setNotification(RGBEffect::Off());
    }

    static void powerVDD(bool enable) {
        // TODO enable / disbale power on for testing purposes
        //return;
        if (enable) {
            gpio::outputFloat(AVR_PIN_AVR_INT);
            gpio::outputHigh(AVR_PIN_VDD_EN);
        } else {
            gpio::outputFloat(AVR_PIN_AVR_INT);
            gpio::outputFloat(AVR_PIN_VDD_EN);
        }
    }

    /** Reboots the RP2350 chip. 
     */
    static void rebootRP() {
        LOG("RP reboot...");
        NO_ISR(
            powerVDD(false);
            // do RGB red countdown effect in a busy loop to give the voltages time to settle, the countdown lasts for approximatekly 1 second
            rgbOn(true);
            rgbClear();
            for (unsigned i = 0; i < NUM_RGB_LEDS; ++i) {
                cpu::wdtReset();
                rgb_[i] = platform::Color::Red().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS);
                rgb_.update();
                cpu::delayMs(200);
            }
            rgbClear();
            rgb_.update();
            powerVDD(true);
        );
    }

    /** Reboots the RP2350 chip into bootloader mode. This is done by pulling the QSPI_CS pin low from the AVR  
     */
    static void bootloaderRP() {
        LOG("RP bootloader...");
        NO_ISR(
            powerVDD(false);
            // pull QSPI_SS low to indicate bootloader
            gpio::outputLow(AVR_PIN_QSPI_SS);
            // do a one second countdown with enabling power to the VDD rail in the middle so that the QSPI_CS low can be picked up
            for (unsigned i = 0; i < NUM_RGB_LEDS; ++i) {
                cpu::wdtReset();
                rgb_[i] = platform::Color::Red().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS);
                rgb_.update();
                if (i == 3)
                    powerVDD(true);
                cpu::delayMs(200);
            }
            // reset the QSPI_SS back to float
            gpio::outputFloat(AVR_PIN_QSPI_SS);
            // since we are in the bootloader mode now, indicate by breathing all keys in green
            rgbEffect_[0] = RGBEffect::Breathe(platform::Color::Green().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), 1);
            rgbEffect_[1] = RGBEffect::Breathe(platform::Color::Green().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), 1);
            rgbEffect_[3] = RGBEffect::Breathe(platform::Color::Green().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), 1);
            rgbEffect_[4] = RGBEffect::Breathe(platform::Color::Green().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), 1);
            rgbEffect_[5] = RGBEffect::Breathe(platform::Color::Green().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), 1);
        );
    }

    //@}

    /** \name System ticks and clocks.
     
        System ticks are used only when the device is on and fire every 5ms. 
     */
    //@{

    static inline volatile bool systemTick_ = false;
    static inline volatile bool secondTick_ = false;
    static inline uint8_t tickCounter_ = 0;

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
#ifndef BREADBOARD
        // only use in real board as the pin is floating on attiny3216
        GPIO_PIN_PINCTRL(AVR_PIN_ACCEL_INT) |= PORT_ISC_BOTHEDGES_gc;
#endif
        GPIO_PIN_PINCTRL(AVR_PIN_PWR_INT) |= PORT_ISC_BOTHEDGES_gc;
        // do not use interrupt on home button (we handle it in the loop due to matrix row rotation)
        GPIO_PIN_PINCTRL(AVR_PIN_BTN_1) &= ~PORT_ISC_gm;
        // if debug mode is enabled, start system notification to white signifying the debug mode power up
        if (state_.status.debugMode())
            setNotification(RGBEffect::Solid(platform::Color::White().withBrightness(RCKID_RGB_LED_DEFAULT_BRIGHTNESS), 255));
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
#ifndef BREADBOARD
        // only use in real board as the pin is floating on attiny1616
        GPIO_PIN_PINCTRL(AVR_PIN_ACCEL_INT) |= PORT_ISC_BOTHEDGES_gc;
#endif
        GPIO_PIN_PINCTRL(AVR_PIN_PWR_INT) |= PORT_ISC_BOTHEDGES_gc;
        GPIO_PIN_PINCTRL(AVR_PIN_BTN_1) |= PORT_ISC_BOTHEDGES_gc;
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
        state_.time.secondTick();
        if (state_.alarm.check(state_.time)) {
            // power the device on, if not on yet
            powerOn();
            // and set the IRQ to notify the RP2350
            NO_ISR(
                state_.status.setAlarmInt();
                setIrq();
            );
        }
        if (powerMode_ & POWER_MODE_ON) {
            // tell RP to increase second
            NO_ISR(
                state_.status.setSecondInt();
                setIrq();
            );
            // start meassuring the temperature
            startADC(ADC_MUXPOS_TEMPSENSE_gc);
            LOG("uptime " << state_.uptime);
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
    static constexpr uint8_t PWR_INT_REQUEST = 2;
    static constexpr uint8_t HOME_BTN_INT_REQUEST = 4;

    static inline volatile uint8_t intRequests_ = 0;

    static inline uint8_t ramPages_[1024];

    static void initializeMasterMode() {
        i2c::initializeMaster();
    }

    static void detectI2CDevices() {
        LOG("Scanning I2C devices...");
        // 0x53 0x68 0x6a
        // 0x53 == LTR390UV
        // 0x68 == MPU6500
        // 0x6a == BQ25895
        for (uint8_t addr = 0; addr < 128; ++addr) {
            if (i2c::isPresent(addr))
                LOG(addr);
            cpu::wdtReset();
            cpu::delayMs(10);
        }
        // anod now see if we can read from BQ25895
        /*
        LOG("BQ25895 register dump:");
        for (uint8_t i = 0; i < 0x15; ++i) {
            uint8_t x;
            i2c::readRegister(0x6a, i, &x, 1);
            LOG(hex(i) << " = " << hex(x, false) << "(binary " << bin(x, false) << ")");
        }
            */
    }

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
            if (irqs & PWR_INT_REQUEST) {
                NO_ISR(
                    state_.status.setPwrInt();
                    setIrq();
                );
            }
            ASSERT(irqs & HOME_BTN_INT_REQUEST == 0); 
        } else {
            // TODO deal with accel and power interrupts - this probably requires some I2C communcation to determine what is going on
            // if the power button is pressed, wake up - enter the debug mode depending on whether the volume down button is pressed
            if (irqs & HOME_BTN_INT_REQUEST) {
                setPowerMode(POWER_MODE_WAKEUP);
                // enable debug mode if volume down is pressed
                if (! gpio::read(AVR_PIN_BTN_3))
                    enterDebugMode();
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
#ifndef BREADBOARD            
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
                gpio::outputFloat(AVR_PIN_AVR_INT);
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
             
                TODO how this will actually work depends on whether we have 1, or 2 dpad LEDs. If two DPAD leds, then DPAD effect sould be copied to both of them. 
             */
            case cmd::SetRGBEffects::ID: {
                auto & c = cmd::SetRGBEffects::fromBuffer(state_.buffer);
                rgbEffect_[0] = c.b;
                rgbEffect_[1] = c.a;
                rgbEffect_[3] = c.dpad;
                rgbEffect_[4] = c.sel;
                rgbEffect_[5] = c.start;
                rgbOn(true);
                break;
            }
            case cmd::Rumbler::ID: {
                auto & c = cmd::Rumbler::fromBuffer(state_.buffer);
                if (c.effect.cycles > 0 && c.effect.strength > 0) {
                    rumblerEffect_ = c.effect;
                    rumblerCurrent_ = rumblerEffect_;
                    rumblerCurrent_.timeOn = 0;
                    rumblerCurrent_.timeOff = 0;
                } else {
                    rumblerEffect_ = RumblerEffect::Off();
                    rumblerCurrent_ = RumblerEffect::Off();
                }
                break;
            }
            case cmd::SetNotification::ID: {
                auto & c = cmd::SetNotification::fromBuffer(state_.buffer);
                setNotification(c.effect);;
                break;
            }
            // when powered on, the AVR does not directly monitor the charging status, but allows riggering of the charging events by the RP2350 that communicates with the charger
            case cmd::ChargerConnected::ID:
                chargerConnected();
                break;
            case cmd::ChargerDisconnected::ID:
                chargerDisconnected();
                break;
            case cmd::ChargerDone::ID:
                chargerDone();
                break;
            case cmd::ChargerError::ID:
                chargerError();
                break;
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
        homeBtnLongPress_ = RCKID_HOME_BUTTON_LONG_PRESS_FPS;
        if (powerMode_ & POWER_MODE_ON) {
            powerOff();
        } else {
            ASSERT(powerMode_ & POWER_MODE_WAKEUP);
            powerOn();
            clearPowerMode(POWER_MODE_WAKEUP);
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
        // check if there has been change
        if (state_.status.btnHome()) {
            if (homeBtnLongPress_ > 0 && (--homeBtnLongPress_ == 0))
                homeBtnLongPress();
        }
        if (changed) {
            LOG("CTRL: " << state_.status.btnHome() << " " << state_.status.btnVolumeUp() << " " << state_.status.btnVolumeDown());
            if (! state_.status.btnHome()) {
                homeBtnLongPress_ = RCKID_HOME_BUTTON_LONG_PRESS_FPS;
                clearPowerMode(POWER_MODE_WAKEUP);
            }
            if (state_.status.debugMode()) {
#ifndef BREADBOARD
                if (powerMode_ && POWER_MODE_WAKEUP && ! state_.btnVolumeUp())
                    leaveDebugMode();
                // the extra button pins are not available on breadboard so the readings are useless
                if (state_.status.btnVolumeUp()) {
                    rebootRP();
                    return;
                } 
                if (state_.status.btnVolumeDown()) {
                    bootloaderRP();
                    return;
                }
#endif
            }
            NO_ISR(setIrq());
        }
    }

    static void readABXYGroup() {
        bool changed = state_.status.setABXYtButtons(
            ! gpio::read(AVR_PIN_BTN_2), // a
            ! gpio::read(AVR_PIN_BTN_1), // b
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
            ! gpio::read(AVR_PIN_BTN_2), // left
            ! gpio::read(AVR_PIN_BTN_4), // right
            ! gpio::read(AVR_PIN_BTN_1), // up
            ! gpio::read(AVR_PIN_BTN_3)  // down
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
     
        We use the ADC to measure the temperature, which makes the ADC handling rather simple. The ADC works in a single converstion mode, is triggered manually (e.g. every second for the temperture measurements) and we are notified about the result via interrupt (the ADC can run in standby mode).

        NOTE: For the AAA battery version the ADC will also be used to measure the battery voltage, which will make this slightly more complex, but assuing the APR_INT pin will be used for the battery voltage measurement, we can still use ADC0. 
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
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_INTREF_gc | ADC_SAMPCAP_bm;
                ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;
                break;
            default:
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
            default:
                // we do not support other ADC channels at this time
                ASSERT(false);
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

    static constexpr unsigned NUM_RGB_LEDS = 6;

    static inline bool rgbOn_ = false;
    static inline RGBEffect notification_{RGBEffect::Off()};
    static inline RGBEffect rgbEffect_[NUM_RGB_LEDS];
    static inline platform::ColorStrip<NUM_RGB_LEDS> rgbTarget_;
    static inline platform::NeopixelStrip<NUM_RGB_LEDS> rgb_{AVR_PIN_RGB};

    static constexpr uint8_t RGB_TICKS_PER_SECOND = 66;
    static inline uint8_t rgbTicks_ = RGB_TICKS_PER_SECOND; 

    static void rgbOn(bool value, bool delayAfterPowerOn = true) {
        if (rgbOn_ == value)
            return;
        if (value) {
            gpio::outputHigh(AVR_PIN_5V_ON);
            gpio::setAsOutput(AVR_PIN_RGB);
            if (delayAfterPowerOn) {
                cpu::wdtReset();
                cpu::delayMs(100);
            }
        } else {
            gpio::outputFloat(AVR_PIN_RGB);
            gpio::outputFloat(AVR_PIN_5V_ON);
            rgbClear();
        }
        rgbOn_ = value;
        rgbTicks_ = RGB_TICKS_PER_SECOND;
    }

    /** Sets RGB notification, or turns it off if the effect is Off.  
     */
    static void setNotification(RGBEffect effect) {
        notification_ = effect;
        if (notification_.active()) {
            rgbOn(true, false);
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
    }

    static void rgbTick() {
        if (!rgbOn_)
            return;
        if (notification_.active()) {
            // notifiations do not go away on a timeout, so no need to worry here
            // for notification, move all LEDs in sync
            bool done = true;
            for (int i = 0; i < NUM_RGB_LEDS; ++i)
                done = done && (! rgb_[i].moveTowards(rgbTarget_[i], notification_.speed));
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
                bool done = ! rgb_[i].moveTowards(rgbTarget_[i], rgbEffect_[i].speed);
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

/** Power interrupt ISR.
 */
ISR(PORTA_PORT_vect) {
    static_assert(AVR_PIN_PWR_INT == gpio::A2);
    VPORTA.INTFLAGS = (1 << GPIO_PIN_INDEX(AVR_PIN_PWR_INT));
    // only do stuff if we were transitioning from low to high
    if (gpio::read(AVR_PIN_PWR_INT))
        RCKid::intRequests_ |= RCKid::PWR_INT_REQUEST;
}

/** Home button interrupt ISR.
 */
ISR(PORTB_PORT_vect) {
    static_assert(AVR_PIN_BTN_1 == gpio::B4);
    VPORTB.INTFLAGS = (1 << GPIO_PIN_INDEX(AVR_PIN_BTN_1));
    // only do stuff if we were tranistioning from high to low (button press)
    if (gpio::read(AVR_PIN_BTN_1) == false)
        RCKid::intRequests_ |= RCKid::HOME_BTN_INT_REQUEST;
}

/** Accel pin ISR.
 */
ISR(PORTC_PORT_vect) {
    static_assert(AVR_PIN_ACCEL_INT == gpio::C5);
    VPORTC.INTFLAGS = (1 << GPIO_PIN_INDEX(AVR_PIN_ACCEL_INT));
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
    RCKid::initialize();
    RCKid::loop();
}
