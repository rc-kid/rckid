#include <Arduino.h>

#include "platform/platform.h"
#include "platform/peripherals/neopixel.h"

#include "common/comms.h"
#include "common/config.h"


using namespace platform;
using namespace comms;

#define ENTER_IRQ //gpio::high(RCKid::RGB)
#define LEAVE_IRQ //gpio::low(RCKid::RGB)

/** Static class containing all modules maintained by the AVR chip in RCKid.

    The AVR is connected diretly to the battery/usb-c power line and manages the power and many peripherals. To the RPI, AVR presents itself as an I2C slave and 2 dedicated pins are used to signal interrupts from AVR to RPI and to signal safe shutdown from RPI to AVR. The AVR is intended to run an I2C bootloader that can be used to update the AVR's firmware when necessary.

                   -- VDD             GND --
             VBATT -- (00) PA4   PA3 (16) -- VIB_EN
           MIC_OUT -- (01) PA5   PA2 (15) -- SCL (I2C)
            BTNS_1 -- (02) PA6   PA1 (14) -- SDA (I2C)
            BTNS_2 -- (03) PA7   PA0 (17) -- (reserved for UPDI)
             JOY_V -- (04) PB5   PC3 (13) -- BTN_HOME
             JOY_H -- (05) PB4   PC2 (12) -- RGB
            RGB_EN -- (06) PB3   PC1 (11) -- RPI_POWEROFF
            RPI_EN -- (07) PB2   PC0 (10) -- AVR_IRQ
            CHARGE -- (08) PB1   PB0 (09) -- BACKLIGHT

    # Design Blocks

    `RTC` is used to generate a 1 second interrupt for real time clock timekeeping, active even in sleep. 

    `ADC0` is used to capture the analog controls (JOY_H, JOY_V, BTNS_1, BTNS_2), power information (VBATT, CHARGE) and internally the VCC and temperature on AVR. ADC also generates a rough estimate of a tick when all measurements are cycled though. 

    `ADC1` is reserved for the microphone input at 8kHz. The ADC is left in a free running mode to accumulate as many results as possible, and `TCB0` is used to generate a precise 8kHz signal to average and capture the signal. 

    `TCA0` is used in split mode to generate PWM signals for the rumbler and screen backlight. 

    `TWI` (I2C) is used to talk to the RPi. When AVR wants attention, the AVR_IRQ pin is pulled low (otherwise it is left floating as RPI pulls it high). A fourth wire, RPI_POWEROFF is used to notify the AVR that RPi's power can be safely shut down. As per the gpio-poweroff overlay, once RPI is ready to be shutdown, it will drive the pin high for 100ms, then low and then high again. Shutdown is possible immediately after the pin goes up. 

    `RPI_EN` must be pulled high to cut the power to RPI, Radio, screen, light sensor, thumbstick and rumbler off. The power is on by default (pulled low externally) so that RPi can be used to re-program the AVR via I2C and to ensure that RPi survives any possible AVR crashes. 

    `RGB_EN` controls a RGB led's power (it takes about 1mA when on even if dark). Its power source is independednt so that it can be used to signal error conditions such as low battery, etc. 

 */
class RCKid {
public:
    static constexpr gpio::Pin VBATT = 0; // ADC0, channel 4
    static constexpr gpio::Pin MIC_OUT = 1; // ADC1, channel 1
    static constexpr gpio::Pin BTNS_1 = 2; // ADC0, channel 6
    static constexpr gpio::Pin BTNS_2 = 3; // ADC0, channel 7
    static constexpr gpio::Pin JOY_V = 4; // ADC0, channel 8
    static constexpr gpio::Pin JOY_H = 5; // ADC0, channel 9
    static constexpr gpio::Pin RGB_EN = 6; // digital, floating
    static constexpr gpio::Pin RPI_EN = 7; // digital, floating
    static constexpr gpio::Pin CHARGE = 8; // ADC0, channel 10

    static constexpr gpio::Pin VIB_EN = 16; // TCA W3
    static constexpr gpio::Pin SCL = 15; // I2C alternate
    static constexpr gpio::Pin SDA = 14; // I2C alternate
    static constexpr gpio::Pin BTN_HOME = 13; // digital
    static constexpr gpio::Pin RGB = 12; // digital
    static constexpr gpio::Pin RPI_POWEROFF = 11; // analog input, ADC1 channel 7
    static constexpr gpio::Pin AVR_IRQ = AVR_PIN_AVR_IRQ; // 10, digital 
    static constexpr gpio::Pin BACKLIGHT = AVR_PIN_BACKLIGHT; // 9,  TCA W0

    /** \name State
     */
    //@{

    // Device state
    static inline ExtendedState state_;

    // Persistent state
    static inline PersistentState pState_;

    // Extra flags that are not meant to be shared with RPi as much
    static inline struct {
        /// true if the end next tick should raise IRQ
        bool irq: 1;
        /// set when I2C command has been received
        bool i2cReady : 1;
        /// RTC one second tick ready
        bool secondTick : 1;
        /// BTN_HOME pin change detected
        bool btnHome : 1;
        /// Critical battery has been reached - reset by charging above certain threshold 
        bool batteryCritical : 1;
    } flags_;

    //@}

    /** \name Initialization

        This is the power-on initialization routine. 
        
     */
    //@{

    static void initialize() __attribute__((always_inline)) {
        // enable 2 second watchdog so that the second tick resets it always with enough time to spare
        while (WDT.STATUS & WDT_SYNCBUSY_bm); // required busy wait
            _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_2KCLK_gc);                
        // set CLK_PER prescaler to 2, i.e. 10Mhz, which is the maximum the chip supports at voltages as low as 3.3V
        CCP = CCP_IOREG_gc;
        CLKCTRL.MCLKCTRLB = CLKCTRL_PEN_bm; 
        gpio::initialize();
        // ensure pins are floating if for whatever reason they would not be
        gpio::input(RPI_EN);
        gpio::input(AVR_IRQ);
        gpio::input(RGB_EN);
        gpio::input(BTN_HOME);
        gpio::input(RGB);
        gpio::input(BACKLIGHT); // do not leak voltage
        gpio::input(VIB_EN); // do not leak voltage
        // initialize the ADC connected pins for better performance (turn of pullups, digital I/O, etc.)
        static_assert(VBATT == 0); // PA4
        PORTA.PIN4CTRL &= ~PORT_ISC_gm;
        PORTA.PIN4CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTA.PIN4CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(MIC_OUT == 1); // PA5
        PORTA.PIN5CTRL &= ~PORT_ISC_gm;
        PORTA.PIN5CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTA.PIN5CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(BTNS_1 == 2); // PA6
        PORTA.PIN6CTRL &= ~PORT_ISC_gm;
        PORTA.PIN6CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTA.PIN6CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(BTNS_2 == 3); // PA7
        PORTA.PIN7CTRL &= ~PORT_ISC_gm;
        PORTA.PIN7CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTA.PIN7CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(JOY_V == 4); // PB5
        PORTB.PIN5CTRL &= ~PORT_ISC_gm;
        PORTB.PIN5CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTB.PIN5CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(JOY_H == 5); // PB4
        PORTB.PIN4CTRL &= ~PORT_ISC_gm;
        PORTB.PIN4CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTB.PIN4CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(CHARGE == 8); // PB1
        PORTB.PIN1CTRL &= ~PORT_ISC_gm;
        PORTB.PIN1CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTB.PIN1CTRL &= ~PORT_PULLUPEN_bm;
        static_assert(RPI_POWEROFF == 11); // PC1
        PORTC.PIN1CTRL &= ~PORT_ISC_gm;
        PORTC.PIN1CTRL |= PORT_ISC_INPUT_DISABLE_gc;
        PORTC.PIN1CTRL &= ~PORT_PULLUPEN_bm;
        // initialize the RTC that fires every second for a semi-accurate real time clock keeping on the AVR, also start the timer
        RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;
        RTC.PITINTCTRL |= RTC_PI_bm; // enable the interrupt
        while (RTC.PITSTATUS & RTC_CTRLBUSY_bm);
        RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc | RTC_PITEN_bm;
        // configure the TCB0 timer to 8kHz and enable it SYNCCH0 
        EVSYS.SYNCCH0 = EVSYS_SYNCCH0_TCB0_gc;
        TCB0.CTRLB = TCB_CNTMODE_INT_gc;
        TCB0.CCMP = 1250; // for 8kHz
        TCB0.CTRLA =  TCB_CLKSEL_CLKDIV1_gc;
        // set the ADC1 voltage to 2.5V
        VREF.CTRLC &= ~VREF_ADC1REFSEL_gm;
        VREF.CTRLC |= VREF_ADC1REFSEL_2V5_gc;
        // initialize TCB1 for the effects, which run at 100Hz. 
        TCB1.CTRLB = TCB_CNTMODE_INT_gc;
        TCB1.CCMP = 50000; // for 100Hz, 10ms interval
        TCB1.INTCTRL = 0;
        TCB1.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
        // initialize TCA for the backlight and rumbler PWM without turning it on, remeber that for the waveform generator to work on the respective pins, they must be configured as output pins (!)
        TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm; // enable split mode
        TCA0.SPLIT.CTRLB = 0;    
        //TCA0.SPLIT.CTRLB = TCA_SPLIT_LCMP0EN_bm | TCA_SPLIT_HCMP0EN_bm; // enable W0 and W3 outputs on pins
        //TCA0.SPLIT.LCMP0 = 64; // backlight at 1/4
        //TCA0.SPLIT.HCMP0 = 128; // rumbler at 1/2
        TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV64_gc | TCA_SPLIT_ENABLE_bm; 
        // initialize the I2C in alternate position
        PORTMUX.CTRLB |= PORTMUX_TWI0_bm;
        // initialize flags as empty
        static_assert(sizeof(flags_) == 1);
        *((uint8_t*)(&flags_)) = 0;
        // initalize the i2c slave with our address
        i2c::initializeSlave(AVR_I2C_ADDRESS);
        // enable BTN_HOME interrupt and internal pull-up, invert the pin's value so that we read the button nicely
        static_assert(BTN_HOME == 13); // PC3
        PORTC.PIN3CTRL |= PORT_ISC_BOTHEDGES_gc | PORT_PULLUPEN_bm | PORT_INVEN_bm;
        // check if we have a WDT reset and set the debug info accordingly
        if (RSTCTRL.RSTFR | RSTCTRL_PORF_bm)
            state_.dinfo.setErrorCode(ErrorCode::InitialPowerOn);
        else if (RSTCTRL.RSTFR | RSTCTRL_WDRF_bm)
            state_.dinfo.setErrorCode(ErrorCode::WatchdogTimeout);
        RSTCTRL.RSTFR = 0xff;
        // verify that wakeup conditions have been met, i.e. that we have enough voltage, etc. and go to sleep immediately if that is not the case. Repeat until we can wakeup. NOTE going to sleep here cuts the power to RPi immediately which can be harmful, but if we are powering on with low battery, there is not much else we can do and the idea is that this ends long time before the RPi gets far enough in the booting process to be able to actually cause an SD card damage  
        if (!canWakeUp())
            sleep();
        // wakeup checks have passed, switch to powerUp phase immediately (no waiting for the home button long press in the case of power on)
        setMode(Mode::WakeUp);
        setMode(Mode::PowerUp);
        // reset the uptime clock
        state_.uptime = 0;
    }


    //@}

    /** \name Main Loop and Power Modes

        ## Wakeup

        The first mode in the power sequence after sleep. Entered immediately after AVR power on, or home button press.

        TODO arguably turning off power here is ok because RPi did not yet start writing to the SD card, so there should be no corruption (we are still deep in kernel startup after 2 seconds). Is this true? 

        ## PowerOn

        When the Home button has been pressed for long enough, the wake up mode transitions to power on mode during which 

        ## On

        This is the default mode during which the Pi is left to its own. AVR is true slave and will only do what RPi tells it. 

        ## PowerDown

        RPI shoukd turn itself off. When done (detected via the RPI_POWEROFF pin) the AVR transitions to the sleep state. If the RPI fails to turn itself off in time, the last error is set accordingly.  
    
     */
    //@{

    static inline uint16_t timeout_ = 0;

    /** The main loop function. Checks the flags, adc results, etc. 
     */
    static void loop() __attribute__((always_inline)) {
        // go to sleep if we should
        if (state_.status.mode() == Mode::Sleep)
            sleep();
        // if there is a command, process it 
        if (flags_.i2cReady) {
            processCommand();
            // be ready for next command to be received
            i2cBufferIdx_ = 0;
            flags_.i2cReady = false;
        }
        // check the ADC used for controls and peripherals, which is also used to measure the internal ticks
        if (adcRead()) {
            if (timeout_ > 0 && --timeout_ == 0)
                timeoutError();
            if (flags_.irq && !state_.status.recording())
                setIrq();
            flags_.irq = false;
        }
        // check the effects tick used for the RGB & rumbler
        effectTick();
        // second tick timekeeping and wdt
        if (flags_.secondTick) {
            secondTick();
            flags_.secondTick = false;
        }
        // check the home button state and presses
        if (flags_.btnHome) {
            btnHomeChange();
            flags_.btnHome = false;            
        }
        // check the RPI poweroff pin to determine if we can go to the sleep
        if (state_.status.mode() == Mode::PowerDown) {
            if (ADC1.INTFLAGS & ADC_RESRDY_bm) {
                if ((ADC1.RES / 64) >= 150) // > 2.9V at 5V VCC, or > 1.9V at 3V3, should be enough including some margin
                    state_.status.setMode(Mode::Sleep);
                else
                    ADC1.COMMAND = ADC_STCONV_bm;
            }
        }
    }

    /** A one-second tick from the RTC, used for timekeeping. 
     */
    static void secondTick() {
        state_.time.secondTick();
        ++state_.uptime;
        wdt::reset();
    }

    /** Called when there is a change in BTN_HOME. Releasing it in the WakeUp mode cancels the wakeup and goes to power down immediately, while during the On mode, we set the timeout to home button press at which point we transition to the power down state. 
     */
    static void btnHomeChange() {
        switch (state_.status.mode()) {
            case Mode::WakeUp:
                // a very crude form of deboucing
                if (gpio::read(BTN_HOME) == false && timeout_ < BTN_HOME_POWERON_PRESS / 2)
                    setMode(Mode::PowerDown);
                break;
            case Mode::On:
                if (gpio::read(BTN_HOME)) {
                    state_.controls.setButtonHome(true);
                    setTimeout(BTN_HOME_POWERON_PRESS);
                } else {
                    state_.controls.setButtonHome(false);
                    setTimeout(0);
                }
                flags_.irq = true;
                break;
            default:
                // don't do anything
                break;
        }
    }

    /** Sleep implementation. Turns everything off and goes to sleep. When sleeping only the BTN_HOME press and RTC second tick will wake the device up and both are acted on accordingly.  
     */
    static void sleep() {
        // clear irq & flags
        gpio::input(AVR_IRQ);
        // cut power to RPI
        gpio::output(RPI_EN);
        gpio::high(RPI_EN);
        // cut power to the RGB
        gpio::input(RGB_EN);
        gpio::input(RGB);
        // turn of ADCs
        ADC1.CTRLA = 0;
        ADC0.CTRLA = 0;
        // turn of backlight
        setBrightness(0);
        // turn off rumbler
        setRumbler(0);
        // sleep, we'll periodically wake up for second ticks and for btnHome press 
        while (true) {
            cpu::sleep();
            if (flags_.btnHome && gpio::read(BTN_HOME)) {
                flags_.btnHome = false;
                if (canWakeUp())
                    break;
            }
            if (flags_.secondTick) {
                rgbOn(); // turn the RGB on, rading charger state will give us some slack time for voltages to average
                secondTick();
                flags_.secondTick = false;
                showColor(Color::White());                
                // when sleeping, we look at the voltage & charging indicator every second to determine if we should show something on the rgb (blue = charging, green = charging done)
                //rgbOn(); // turn the RGB on, rading charger state will give us some slack time for voltages to average
                // voltage reference to 1.1V (internal for the temperature sensor)
                VREF.CTRLA &= ~ VREF_ADC0REFSEL_gm;
                VREF.CTRLA |= VREF_ADC0REFSEL_1V1_gc;
                ADC0.CTRLB = ADC_SAMPNUM_ACC64_gc;
                ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;
                // remove sampcap as per the info
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc; // use VDD as reference for VCC sensing, 1.25MHz
                ADC0.CTRLD = ADC_INITDLY_DLY256_gc;
                ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc;
                // start new conversion
                ADC0.COMMAND = ADC_STCONV_bm;
                while (! (ADC0.INTFLAGS & ADC_RESRDY_bm));
                uint16_t vcc = ADC0.RES / 64;
                vcc = 110 * 512 / vcc;
                vcc = vcc * 2;
                // if we have VUSB, check the charger's status
                if (vcc >= VCC_THRESHOLD_VUSB) {
                    rgbOn(); // turn the RGB on, rading charger state will give us some slack time for voltages to average
                    ADC0.MUXPOS = ADC_MUXPOS_AIN10_gc;
                    ADC0.COMMAND = ADC_STCONV_bm;
                    while (! (ADC0.INTFLAGS & ADC_RESRDY_bm));
                    // 64 accumulate, 2 from 10 to 8bits resolution
                    bool charging = (((ADC0.RES / 64) >> 2) & 0xff) < 64;
                    rgb_[0] = charging ? Color::RGB(0, 0, 32) : Color::RGB(0, 32, 0);
                    rgb_.update();
                } else {
                    // ensure rgb is off 
                    rgbOff();
                } 
                // turn the ADC off and go to sleep again
                ADC0.CTRLA = 0;

            }
        }
        setMode(Mode::WakeUp);
    }

    /** Checks if we can wakeup, i.e. if there is enough voltage. 
     */
    static bool canWakeUp() {
        // ensure interrupts are enabled
        sei();
        // enable the ADC but instead of real ticks, just measure the VCC 10 times
        adcStart();
        uint16_t avgVcc = 0;
        for (uint8_t i = 0; i < 10; ++i) {
            while (! (ADC0.INTFLAGS & ADC_RESRDY_bm));
            uint16_t vcc = ADC0.RES / 64;
            vcc = 110 * 512 / vcc;
            vcc = vcc * 2;
            avgVcc += vcc;
            ADC0.COMMAND = ADC_STCONV_bm;
        }
        avgVcc = avgVcc / 10;
        if (avgVcc >= BATTERY_THRESHOLD_CHARGED)
            flags_.batteryCritical = false;
        if (avgVcc <= BATTERY_THRESHOLD_CRITICAL) // power-on from empty battery
            flags_.batteryCritical = true;
        // if the battery flag is too low, flash the critical battery lights and go back to sleep
        if (flags_.batteryCritical) {
            criticalBatteryWarning();
            return false;
        }
        // otherwise we can power on 
        return true;
    }

    /** Enters the given mode. 
     */
    static void setMode(Mode mode) {
        switch (mode) {
            case Mode::On:
                setTimeout(0); // disable the timeout
                setBrightness(pState_.brightness);
                // and initialize the ADC1 we use for sound recording
                ADC1.CTRLA = ADC_RESSEL_8BIT_gc;
                ADC1.CTRLB = ADC_SAMPNUM_ACC32_gc; 
                ADC1.CTRLC = ADC_PRESC_DIV2_gc | ADC_REFSEL_INTREF_gc | ADC_SAMPCAP_bm;
                ADC1.CTRLD = 0; // no sample delay, no init delay
                ADC1.SAMPCTRL = 0;
                ADC1.EVCTRL = ADC_STARTEI_bm; // ADC will be triggered by event
                ADC1.MUXPOS = ADC_MUXPOS_AIN1_gc;
                ADC1.INTCTRL = ADC_RESRDY_bm;
                ADC1.CTRLA |= ADC_ENABLE_bm;
                break;
            // for sleep, don't do anything, all will be handled by the main loop that will enter sleep immediately
            case Mode::Sleep:
                break;
            case Mode::WakeUp:
                // turn off RGB just to be on the safe side
                rgbOff();
                // clear flags so that there are no leftovers from previous run
                flags_.irq = false;
                flags_.i2cReady = false;
                // turn rpi on
                gpio::input(RPI_EN);
                setTimeout(BTN_HOME_POWERON_PRESS);
                // reset the comms state for the power up
                setDefaultTxAddress();
                break;
            // transitioning from wakeup to powerup when the home key was pressed long enough
            case Mode::PowerUp:
                // if select is pressed as well, enter power on mode immediately to bypass the timer and enforce display brightness 
                if (state_.controls.select()) {
                    pState_.brightness = 128;
                    setMode(Mode::On);
                    // indicate forced power on by 2 medium rumbles
                    rumblerEffect(DEFAULT_RUMBLER_STRENGTH, 30, 23, 2);
                    return;
                }
                // rumble to indicate true power on and set the timeout for RPI poweron 
                rumblerOk();
                // disable the timeout if Select button is pressed
                setTimeout(RPI_POWERUP_TIMEOUT);
                break;
            case Mode::PowerDown:
                stopRecording();
                setBrightness(0);
                setTimeout(RPI_POWERDOWN_TIMEOUT);
                // start ADC1 in normal mode, read RPI_POWEROFF pin, start the conversion
                ADC1.CTRLB = ADC_SAMPNUM_ACC64_gc; 
                ADC1.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm;
                ADC1.CTRLD = ADC_INITDLY_DLY32_gc;
                ADC1.SAMPCTRL = 0;
                ADC1.INTCTRL = 0; // no interrupts
                ADC1.MUXPOS = ADC_MUXPOS_AIN7_gc;
                ADC1.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_8BIT_gc;
                ADC1.COMMAND = ADC_STCONV_bm;
                break;
            default:
                break;
        }
        state_.status.setMode(mode);
    }

    /** Sets the timeout, measured in ticks. Settin the timeout to 0 disables the function. 
     */
    static void setTimeout(uint16_t value) {
        timeout_ = value;
    }

    /** Called when timeout (Rpi power on or power off occurs). Sets the latest error, its color, rumbles and then goes to sleep. 
     
        The timeout is also used for long BTN_HOME press during normal mode, we do rumbler fail, then go to power down mode immediately. The IRQ is set to inform the RPi of the mode transition so that it can react and turn itself off. 
     */
    static void timeoutError() {
        switch (state_.status.mode()) {
            // in wakeup, transition to power up (home button has been pressed for long enough)
            case Mode::WakeUp:
                setMode(Mode::PowerUp);
                return;
            // in powerup mode, this indicates RPi failed to power up in time, we'll go to sleep
            case Mode::PowerUp:
                state_.dinfo.setErrorCode(ErrorCode::RPiBootTimeout);
                break;
            // in powerdown, this means that RPi failed to indicate safe power off in time, go to sleep
            case Mode::PowerDown:
                state_.dinfo.setErrorCode(ErrorCode::RPiPowerDownTimeout);
                break;
            // timeout in mode::on is HOME button long press - give RPI chance to turn itself off nicely first
            case Mode::On:
                // double check that the button is indeed still pressed so that we do not accidentally poweroff
                if (gpio::read(BTN_HOME) == false)
                    return;
                rumblerOk();
                setMode(Mode::PowerDown);
                setIrq();
                return;
        }
        // do error signal, then sleep
        rgbOn();
        showColor(Color::Red().withBrightness(32));
        // we need blocking failure here since we go to sleep immediately 
        rumblerFailBlocking();
        setMode(Mode::Sleep);
    }

    /** Sets the LCD display brightness. Set 0 to turn the display off. 
     */
    static void setBrightness(uint8_t value) {
        if (value == 0) { // turn off
            TCA0.SPLIT.CTRLB &= ~TCA_SPLIT_LCMP0EN_bm;
            gpio::input(BACKLIGHT);
        } else if (value == 255) {
            TCA0.SPLIT.CTRLB &= ~TCA_SPLIT_LCMP0EN_bm;
            gpio::output(BACKLIGHT);
            gpio::high(BACKLIGHT);
        } else {
            gpio::output(BACKLIGHT);
            TCA0.SPLIT.CTRLB |= TCA_SPLIT_LCMP0EN_bm;
            TCA0.SPLIT.LCMP0 = value;
        }
    }

    //@}

    /** \name Communication with RPI
     
        Communication with RPI is done via I2C bus with AVR being the slave. Additionally, the AVR_IRQ pin is pulled high to 3V3 by the RPI and can be set low by AVR to indicate to RPI a state change.

        Master write operations simply write bytes in the i2c buffer. When master write is done, the `i2cReady` flag is set signalling to the main loop that data from master are available. The main loop then reads the i2c buffer and performs the received command, finally resetting the `i2cready` flag. Only one command can be processed at once and the AVR will return nack if a master write is to be performed while `i2cready` flag is high. 

        Master read always reads the first status byte first giving information about the most basic device state. This first byte is then followed by the bytes read from the `i2cTxAddress`. By default, this will continue reading the rest of the state, but depending on the commands and state can be redirected to other addresses such as the command buffer itself, or the audio buffer. When not in recording mode, after each master read the `i2cTxAddress` is reset to the default value sending more state.  

        When recording, the AVR will be sending the recording buffer data preceded by the state byte. However, in recording mode the mode bits of the status are used to 
     */
    //@{

    // The I2C buffer used to store incoming data
    static inline uint8_t i2cBuffer_[I2C_BUFFER_SIZE];
    static inline uint8_t i2cBufferIdx_ = 0;

    static constexpr uint8_t TX_START = 0xff;

    static inline uint8_t * i2cTxAddress_ = nullptr;
    static inline uint8_t i2cNumTxBytes_ = TX_START;

    /** Pulls the AVR_IRQ pin low indicating to the RPI to talk to the AVR. Cleared automatically by the I2C interrupt vector. 
     */
    static void setIrq() {
        gpio::output(AVR_IRQ);
        gpio::low(AVR_IRQ);
    }

    /** Sets the default tx address when the initial status byte is followed by the rest of the state. To do so, the tx address is set to the state's address + 1 to account for the already sent status byte.
     */
    static void setDefaultTxAddress() {
        i2cTxAddress_ = ((uint8_t *) (& state_)) + 1;
    }

    /** Sets the tx address to given value meaning that next buffer read from the device will start here. 
     */
    static void setTxAddress(uint8_t * address) {
        i2cTxAddress_ = address;
    }

    /** Handles the processing of a previously received I2C comand. 
     */
    static void processCommand() {
        using namespace msg;
        switch (i2cBuffer_[0]) {
            // nothing, as expected
            case Nop::ID: {
                break;
            }
            // resets the chip
            case AvrReset::ID: {
                _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
                // unreachable here
            }
            // sends the information about the chip in the same way the bootloader does. The command first stores the information in the i2cCommand buffer and then sets the command buffer as the tx address
            case msg::Info::ID:
                i2cBuffer_[0] = SIGROW.DEVICEID0;
                i2cBuffer_[1] = SIGROW.DEVICEID1;
                i2cBuffer_[2] = SIGROW.DEVICEID2;
                i2cBuffer_[3] = 1; // app
                for (uint8_t i = 0; i < 10; ++i)
                    i2cBuffer_[4 + i] = ((uint8_t*)(&FUSE))[i];
                i2cBuffer_[15] = CLKCTRL.MCLKCTRLA;
                i2cBuffer_[16] = CLKCTRL.MCLKCTRLB;
                i2cBuffer_[17] = CLKCTRL.MCLKLOCK;
                i2cBuffer_[18] = CLKCTRL.MCLKSTATUS;
                i2cBuffer_[19] = MAPPED_PROGMEM_PAGE_SIZE >> 8;
                i2cBuffer_[20] = MAPPED_PROGMEM_PAGE_SIZE & 0xff;
                i2cTxAddress_ = (uint8_t *)(& i2cBuffer_);
                // let the RPi know we have processed the command
                setIrq();
                break;
            // starts the audio recording 
            case msg::StartAudioRecording::ID: {
                if (state_.status.mode() == Mode::On && state_.status.recording() == false)
                    startRecording();
                break;
            }
            // stops the recording
            case msg::StopAudioRecording::ID: {
                if (state_.status.recording())
                    stopRecording();
                break;
            }
            // sets the display brightness
            case SetBrightness::ID: {
                auto & m = SetBrightness::fromBuffer(i2cBuffer_);
                pState_.brightness = m.value;
                setBrightness(m.value);
                break;
            }
            // sets the time
            case SetTime::ID: {
                auto & m = SetTime::fromBuffer(i2cBuffer_);
                cli(); // avoid corruption by the RTC interrupt
                state_.time = m.value;
                sei();
                break;
            }
            case GetPersistentState::ID: {
                i2cTxAddress_ = (uint8_t *)(& pState_);
                // let the RPi know we have processed the command
                setIrq();
                break;
            }
            case SetPersistentState::ID: {
                auto & m = SetPersistentState::fromBuffer(i2cBuffer_);
                pState_ = m.pState;
                break;
            }
            case RumblerOk::ID: {
                rumblerOk();
                break;
            }
            case RumblerFail::ID: {
                rumblerFail();
                break;
            }
            case Rumbler::ID: {
                auto & m = Rumbler::fromBuffer(i2cBuffer_);
                rumblerEffect(m.intensity, m.duration);
                break;
            }
            case RGBOn::ID: {
                rgbOn();
                break;
            }
            case RGBOff::ID: {
                rgbOff();
                break;
            }
            case RGBColor::ID: {
                auto & m = RGBColor::fromBuffer(i2cBuffer_);
                showColor(m.color);
                break;
            }
            case PowerOn::ID: {
                // to be on the safe side, the power on command works in both PowerUp and WakeUp modes as there is a theoretical path to safe on mode from either of those modes
                if (state_.status.mode() == Mode::PowerUp || state_.status.mode() == Mode::WakeUp)
                    setMode(Mode::On);
                break;
            }
            case PowerDown::ID: {
                if (state_.status.mode() == Mode::On || state_.status.mode() == Mode::PowerUp)
                    setMode(Mode::PowerDown);
                break;
            }
            case DInfoClear::ID: {
                if (state_.dinfo.clear());
                    rgbOff();
                break;
            }
        }
    }

    /** The I2C interrupt handler. 
     */
    static inline void TWI0_TWIS_vect() __attribute__((always_inline)) {
        #define I2C_DATA_MASK (TWI_DIF_bm | TWI_DIR_bm) 
        #define I2C_DATA_TX (TWI_DIF_bm | TWI_DIR_bm)
        #define I2C_DATA_RX (TWI_DIF_bm)
        #define I2C_START_MASK (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm)
        #define I2C_START_TX (TWI_APIF_bm | TWI_AP_bm | TWI_DIR_bm)
        #define I2C_START_RX (TWI_APIF_bm | TWI_AP_bm)
        #define I2C_STOP_MASK (TWI_APIF_bm | TWI_DIR_bm)
        #define I2C_STOP_TX (TWI_APIF_bm | TWI_DIR_bm)
        #define I2C_STOP_RX (TWI_APIF_bm)
        ENTER_IRQ;
        uint8_t status = TWI0.SSTATUS;
        // sending data to accepting master is on our fastpath and is checked first. For the first byte we always send the status, followed by the data located at the txAddress. It is the responsibility of the master to ensure that only valid data sizes are reqested. 
        if ((status & I2C_DATA_MASK) == I2C_DATA_TX) {
            if (i2cNumTxBytes_ == TX_START)
                TWI0.SDATA = * (uint8_t*) (& state_.status);
            else
                TWI0.SDATA = i2cTxAddress_[i2cNumTxBytes_];
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
            ++i2cNumTxBytes_;
        // a byte has been received from master. Store it and send either ACK if we can store more, or NACK if we can't store more
        } else if ((status & I2C_DATA_MASK) == I2C_DATA_RX) {
            i2cBuffer_[i2cBufferIdx_++] = TWI0.SDATA;
            TWI0.SCTRLB = (i2cBufferIdx_ == sizeof(i2cBuffer_)) ? TWI_SCMD_COMPTRANS_gc : TWI_SCMD_RESPONSE_gc;
        // master requests slave to write data, reset the sent bytes counter, initialize the actual read address from the read start and reset the IRQ
        } else if ((status & I2C_START_MASK) == I2C_START_TX) {
            gpio::input(AVR_IRQ);
            TWI0.SCTRLB = TWI_ACKACT_ACK_gc + TWI_SCMD_RESPONSE_gc;
            if (state_.status.recording())
                state_.status.setBatchIncomplete((wrIndex_ >> 5) == state_.status.batchIndex());
            // reset the num tx bytes
            i2cNumTxBytes_ = TX_START;
        // master requests to write data itself. ACK if there is no pending I2C message, NACK otherwise. The buffer is reset to 
        } else if ((status & I2C_START_MASK) == I2C_START_RX) {
            TWI0.SCTRLB = flags_.i2cReady ? TWI_ACKACT_NACK_gc : TWI_SCMD_RESPONSE_gc;
        // sending finished, reset the tx address and when in recording mode determine if more data is available
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_TX) {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            if (! state_.status.recording()) {
                setDefaultTxAddress();
            } else if (i2cNumTxBytes_ >= 32 && !state_.status.batchIncomplete()) {
                uint8_t nextBatch = (state_.status.batchIndex() + 1) & 7;
                state_.status.setBatchIndex(nextBatch);
                setTxAddress(recBuffer_ + (nextBatch << 5));
                // if the next batch is already available, set the IRQ, otherwise it will be sent when it becomes available by the ADC reader
                if (nextBatch != (wrIndex_ >> 5)) 
                    setIrq();
            }
        // receiving finished, inform main loop we have message waiting if we have received at laast one byte (0 bytes received is just I2C ping)
        } else if ((status & I2C_STOP_MASK) == I2C_STOP_RX) {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            if (i2cBufferIdx_ > 0)
                flags_.i2cReady = true;
        } else {
            // error - a state we do not know how to handle
        }
        LEAVE_IRQ;
    }

    //@}

    /** \name Controls & Battery & Charging monitoring & Ticks

        `ADC0` handles all the analog inputs and their periodic reading. Since we need to change the MUXPOS after each reading, the reading is done in a polling mode. When the ADC cycles through all of its inputs, a tick is initiated (roughly at 200Hz), during which the real values are computed from the raw readings and appropriate action is taken (or the RPI is notified). 

        The ADC cycles through the following measurements: 

        - VCC for critical battery warning
        - TEMP
        - VBATT
        - CHARGE
        - BTNS_2
        - BTNS_1
        - JOY_H
        - JOY_V  

        `BTNS_1` and `BTNS_2` are connected to a custom voltage divider that allows us to sample multiple presses of 3 buttons using a single pin. The ladder assumes a 8k2 resistor fro VCC to common junction that is beaing read and is connected via the three buttons and three different resistors (8k2, 15k, 27k) to ground.  
     */
    //@{

    static inline uint8_t batteryDebounceTimer_ = 0;

    /** Starts the measurements on ADC0. 
    */
    static void adcStart() {
        // voltage reference to 1.1V (internal for the temperature sensor)
        VREF.CTRLA &= ~ VREF_ADC0REFSEL_gm;
        VREF.CTRLA |= VREF_ADC0REFSEL_1V1_gc;
        // delay 32us and sampctrl of 32 us for the temperature sensor, do averaging over 64 values, full precission
        ADC0.CTRLB = ADC_SAMPNUM_ACC64_gc;
        ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;
        ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc; // | ADC_SAMPCAP_bm; // use VDD as reference for VCC sensing, 1.25MHz
        ADC0.CTRLD = ADC_INITDLY_DLY32_gc;
        ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_10BIT_gc;
         // start new conversion
        ADC0.COMMAND = ADC_STCONV_bm;
    }

    static bool adcRead() {
        // if ADC is not ready, return immediately without a tick
        if (! (ADC0.INTFLAGS & ADC_RESRDY_bm))
            return false;
        uint16_t value = ADC0.RES / 64;
        uint8_t muxpos = ADC0.MUXPOS;
        // do stuff depending on what the ADC was doing, first move to the next ADC read and start the conversion, then process the current one
        switch (muxpos) {
            // VCC
            case ADC_MUXPOS_INTREF_gc:
                ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;
                ADC0.SAMPCTRL = 31;
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_INTREF_gc | ADC_SAMPCAP_bm; // internal reference
                break;
            // Temp
            case ADC_MUXPOS_TEMPSENSE_gc:
                ADC0.MUXPOS = ADC_MUXPOS_AIN4_gc;
                ADC0.SAMPCTRL = 0;
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; 
                break;
            // VBATT
            case ADC_MUXPOS_AIN4_gc:
                ADC0.MUXPOS = ADC_MUXPOS_AIN10_gc;
                break;
            // CHARGE 
            case ADC_MUXPOS_AIN10_gc:
                ADC0_MUXPOS = ADC_MUXPOS_AIN6_gc;
                break;
            // BTNS_2 
            case ADC_MUXPOS_AIN6_gc:
                ADC0_MUXPOS = ADC_MUXPOS_AIN7_gc;
                break;
            // BTNS_1 
            case ADC_MUXPOS_AIN7_gc:
                ADC0_MUXPOS = ADC_MUXPOS_AIN8_gc;
                break;
            // JOY_V 
            case ADC_MUXPOS_AIN8_gc:
                ADC0_MUXPOS = ADC_MUXPOS_AIN9_gc;
                break;
            // JOY_H 
            case ADC_MUXPOS_AIN9_gc: // fallthrough to default case
            /** Reset the ADC to take VCC measurements */
            default:
                ADC0.MUXPOS = ADC_MUXPOS_INTREF_gc;
                ADC0.CTRLC = ADC_PRESC_DIV8_gc | ADC_REFSEL_VDDREF_gc | ADC_SAMPCAP_bm; // use VDD as reference for VCC sensing
                ADC0.SAMPCTRL = 0;
                break;
        }
        // start the next conversion
        ADC0.COMMAND = ADC_STCONV_bm;
        // process the last measurement while reading the next measurement
        switch (muxpos) {
            // convert the reading to voltage and update the state 
            case ADC_MUXPOS_INTREF_gc: {
                value = 110 * 512 / value;
                value = value * 2;
                state_.einfo.setVcc(value);
                // if we have critical battery threshold go to powerdown mode immediately, set the battery critical flag
                if (value <= BATTERY_THRESHOLD_CRITICAL && ! flags_.batteryCritical) {
                    if (batteryDebounceTimer_-- == 0) {
                        flags_.irq = true;
                        flags_.batteryCritical = true;
                        criticalBatteryWarning();
                        setMode(Mode::PowerDown);
                    }
                } else {
                    batteryDebounceTimer_ = 10;
                }
                break;
            }
            // convert temperature reading to temperature, the code is taken from the ATTiny datasheet example
            case ADC_MUXPOS_TEMPSENSE_gc: {
                int8_t sigrow_offset = SIGROW.TEMPSENSE1; 
                uint8_t sigrow_gain = SIGROW.TEMPSENSE0;
                int32_t t = value - sigrow_offset; // Result might overflow 16 bit variable (10bit+8bit)
                t *= sigrow_gain;
                // temp is now in kelvin range, to convert to celsius, remove -273.15 (x256)
                t -= 69926;
                // and now loose precision to 0.5C (x10, i.e. -15 = -1.5C)
                value = (t >>= 7) * 5;
                state_.einfo.setTemp(value);
                break;
            }
            // VBATT
            case ADC_MUXPOS_AIN4_gc: {
                // convert the battery reading to voltage. The battery reading is relative to vcc, which we already have
                // TODO will this work? when we run on batteries, there might be a voltage drop on the switch? 
                // VBATT = VCC * VBATT / 255 
                // but 500 * 255 is too high to fit in uint16, so we divide VCC by 2 and then divide by 128 instead
                value >>= 2; // go for 8bit precision, which should be enough
                value = (state_.einfo.vcc() / 2 * value)  / 128; 
                state_.einfo.setVBatt(value);
                break;
            }
            // CHARGE - sent by the charger chip via a pull-up and pull-down resistors. Only works if VUSB is present. Close to 0 means charging, close to 1 means charging complete and around 0.5 means no battery present (which we for all purposes ignore)
            case ADC_MUXPOS_AIN10_gc: {
                value = (value >> 2) & 0xff;
                // now we have the VCC and VBATT voltages as well as charging info so we can set the power status
                PowerStatus pstatus{PowerStatus::Battery};
                if (state_.einfo.vcc() >= VCC_THRESHOLD_VUSB) {
                    pstatus = (value < 64) ? PowerStatus::Charging : PowerStatus::USB; 
                } else {
                    if (state_.einfo.vcc() < BATTERY_THRESHOLD_LOW)
                        pstatus = PowerStatus::LowBattery;
                }
                flags_.irq = state_.status.setPowerStatus(pstatus) | flags_.irq;
                break;
            }
            // BTNS_1 
            case ADC_MUXPOS_AIN6_gc:
                flags_.irq = state_.controls.setButtons1(decodeAnalogButtons((value >> 2) & 0xff)) | flags_.irq;
                break;
            // BTNS_2 
            case ADC_MUXPOS_AIN7_gc:
                flags_.irq = state_.controls.setButtons2(decodeAnalogButtons((value >> 2) & 0xff)) | flags_.irq;
                flags_.irq = state_.controls.setButtonHome(gpio::read(BTN_HOME)) | flags_.irq;
                break;
            // JOY_V 
            case ADC_MUXPOS_AIN8_gc:
                value = adjustJoystickValue(value, pState_.joyVMin, pState_.joyVMax);
                flags_.irq = state_.controls.setJoyV(value) | flags_.irq;
                break;
            // JOY_H 
            case ADC_MUXPOS_AIN9_gc:
                value = adjustJoystickValue(value, pState_.joyHMin, pState_.joyHMax);
                flags_.irq = state_.controls.setJoyH(value) | flags_.irq;
                return true;
        }
        return false;
    }

    /** Decodes the raw analog value read into the states of two buttons returned as LSB bits. The analog value is a result of a custom voltage divider ladder so that simultaneous button presses can be detected. The following are the expected values:
     
    00 : 255
    02 : 183
    10 : 153
    12 : 124
        
     */
    static uint8_t decodeAnalogButtons(uint8_t raw) {
        if (raw <= 138)
            return 0b11;
        if (raw <= 168)
            return 0b10;
        if (raw <= 198)
            return 0b01;
        return 0;
    }

    /** Adjusts the joystick axis value - since the joystick is powered by 3V3, but measured using the VCC, its value needs to be scaled appropriately. 
     */
    static uint8_t adjustJoystickValue(uint32_t value, uint8_t min, uint8_t max) {
        value = value * state_.einfo.vcc() / 132;
        value = (value > min * 10) ? value - min * 10 : 0;
        uint16_t scale = (max - min) * 10;
        value = (value > scale) ? scale : value;
        value = value * 255 / scale; 
        return value > 255 ? 255 : static_cast<uint8_t>(value);
    }

    //@}

    /** \name Audio Recording
      
        When audio recording is enabled, the ADC1 runs at 5MHz speed in 8bit mode and 32x sample accumulation. The ADC is triggered by TCB0 running at 8kHz and when its measurement is done, it is appended in a circular buffer. The buffer is divided into 8 32byte long batches. Each time there is 32 sampled bytes available, the AVR_IRQ is set informing the RPi to read the batch.  

        While in recording mode, when RPi starts reading, a status byte is sent first, followed by the recording buffer contents. The status byte contains a flag that the AVR is in recording mode and a batch index that will be returned. Each time a recording data is being downloaded, the status byte's incompleteBatch is set if the batch to be sent has not been finished (i.e. we are writing to it while transmitting). Such batches are to be ignored. 
        
        When 32 bytes after the status byte are read and the batch currently being read has been valid at the beginning of the read sequence (i.e. the whole batch was sent), the batch index is incremented. If the next batch is also available in full, the AVR_IRQ is set, otherwise it will be set when the recorder crosses the batch boundary.  

        The 8 batches and AVR_IRQ combined should allow enough time for the RPi to be able to read and buffer the data as needed without skipping any batches - but if a skip occurs the batch index in the status byte should be enough to detect it. 
    */
    //@{

    /// Circular buffer for the mic recorder, conveniently wrapping at 256, giving us 8 32 bytes long batches
    static inline uint8_t recBuffer_[256];
    /// Write index to the circular buffer
    static inline volatile uint8_t wrIndex_ = 0;

    static void startRecording() {
        cli();
        wrIndex_ = 0;
        state_.status.setRecording(true);
        state_.status.setBatchIndex(0);
        setTxAddress(recBuffer_);
        sei();
        // connect the eventsystem 
        EVSYS.ASYNCUSER12 = EVSYS_ASYNCUSER12_SYNCCH0_gc;
        // start the 8kHz timer on TCB0. The timer overflow will trigger ADC start
        TCB0.CTRLA |= TCB_ENABLE_bm;
    }

    static void stopRecording() {
        // restore mode and then disable the recording mode
        TCB0.CTRLA &= ~TCB_ENABLE_bm;
        EVSYS.ASYNCUSER12 = 0;
        cli();
        state_.status.setMode(Mode::On);
        state_.status.setRecording(false);
        state_.status.setBatchIncomplete(false);
        setDefaultTxAddress();
        sei();
    }

    /** Critical code, just accumulate the sampled value.
     */
    static inline void ADC1_RESRDY_vect(void) __attribute__((always_inline)) {
        ENTER_IRQ;
        // we are using 32x oversampling
        recBuffer_[wrIndex_++] = (ADC1.RES / 32) & 0xff;
        if (wrIndex_ % 32 == 0)
            setIrq();
        LEAVE_IRQ;
    }

    //@}

    /** \name Rumbler 
     */
    //@{

    static inline uint8_t rumblerStrength_ = 0;
    static inline uint8_t rumblerEffectRepeat_ = 0;
    static inline uint16_t rumblerEffectLength_ = 0;
    static inline uint16_t rumblerEffectPause_ = 0;
    static inline uint16_t rumblerEffectCountdown_ = 0;

    static void rumblerEffect(uint8_t strength, uint16_t length) {
        setRumbler(strength);
        rumblerEffectLength_ = length;
        rumblerEffectPause_ = 0;
        rumblerEffectRepeat_ = 1;
        rumblerEffectCountdown_ = length;
    }

    static void rumblerEffect(uint8_t strength, uint16_t length, uint16_t pause, uint8_t repeat) {
        setRumbler(strength);
        rumblerEffectLength_ = length;
        rumblerEffectPause_ = pause;
        rumblerEffectRepeat_ = repeat;
        rumblerEffectCountdown_ = length;
    }

    static void setRumbler(uint8_t value) {
        rumblerStrength_ = value;
        if (value == 0) { // turn off
            TCA0.SPLIT.CTRLB &= ~TCA_SPLIT_HCMP0EN_bm;
            gpio::input(VIB_EN);
        } else {
            gpio::output(VIB_EN);
            TCA0.SPLIT.CTRLB |= TCA_SPLIT_HCMP0EN_bm;
            TCA0.SPLIT.HCMP0 = 255 - value;
        }
    }

    static void rumblerOk() {
        rumblerEffect(DEFAULT_RUMBLER_STRENGTH, 75);
    }

    static void rumblerFail() {
        rumblerEffect(DEFAULT_RUMBLER_STRENGTH, 10, 23, 3);
    }

    static void rumblerOkBlocking() {
        setRumbler(DEFAULT_RUMBLER_STRENGTH);
        cpu::delayMs(750);
        setRumbler(0);
    }

    static void rumblerFailBlocking() {
        for (int i = 0; i < 3; ++i) {
            setRumbler(DEFAULT_RUMBLER_STRENGTH); 
            cpu::delayMs(100);
            setRumbler(0);
            cpu::delayMs(230);
        }
    }

    //@}

    /** \name RGB Led

        The RGB led is used for various status information by default, but can also be directly controlled by the RPi for feedback / flashlight. Uses the TCB1 timer for a frame information running at 80Hz, which seems to be enough for most LED effects.  

     */
    //@{

    static inline NeopixelStrip<1> rgb_{RGB};
    static inline ColorStrip<1> rgbTarget_;

    static void rgbOn() {
        gpio::output(RGB_EN);
        gpio::low(RGB_EN);
        gpio::output(RGB);
        gpio::low(RGB);
    }

    /** Turns the RGB led off. 
     */
    static void rgbOff() {
        gpio::input(RGB_EN);
        gpio::input(RGB);
        // clear the effect
        rgbTarget_[0] = Color::Black();
        rgb_[0] = Color::Black();
    }

    static void showColor(Color c) {
        rgbTarget_[0] = c;
    }

    /** Flashes the critical battery warning, 5 short red flashes. 
     */
    static void criticalBatteryWarning() {
        rgbOn();
        for (int i = 0; i < 5; ++i) {
            rgb_.fill(Color::Red());
            rgb_.update();
            cpu::delayMs(100);
            rgb_.fill(Color::Black());
            rgb_.update();
            cpu::delayMs(100);
        }
        rgbOff();
    }

    //@}

    /** \name Feedback effects (Rumbler & RGB)
     */
    //@{


    static void effectTick() {
        if (! TCB1.INTFLAGS & TCB_CAPT_bm)
            return;
        TCB1.INTFLAGS = TCB_CAPT_bm;
        // turn the rumbler off if the countdown has been done
        if (rumblerEffectCountdown_ > 0 && --rumblerEffectCountdown_ == 0) {
            if (rumblerStrength_ != 0) {
                setRumbler(0);
                rumblerEffectCountdown_ = rumblerEffectPause_;
            } else if (rumblerEffectRepeat_ > 1) {
                setRumbler(rumblerStrength_);
                rumblerEffectCountdown_ = rumblerEffectLength_;
                --rumblerEffectRepeat_;
            }
        }
        // update the RGB based on effect
        if (rgb_.moveTowards(rgbTarget_, 1))
            rgb_.update();
    }
    //@}


}; // RCKid

/** Interrupt handler for BTN_HOME (PC3), rising and falling.
 */
ISR(PORTC_PORT_vect) { 
    ENTER_IRQ;
    VPORTC.INTFLAGS = (1 << 3); // PC3
    RCKid::flags_.btnHome = true;
    LEAVE_IRQ;
}

/** The RTC one second interval tick ISR. 
 */
ISR(RTC_PIT_vect) {
    ENTER_IRQ;
    RTC.PITINTFLAGS = RTC_PI_bm; // clear the interrupt
    RCKid::flags_.secondTick = true;
    LEAVE_IRQ;
}

ISR(TWI0_TWIS_vect) { 
    ENTER_IRQ;
    RCKid::TWI0_TWIS_vect(); 
    LEAVE_IRQ;
}

ISR(ADC1_RESRDY_vect) {
    ENTER_IRQ; 
    RCKid::ADC1_RESRDY_vect();
    LEAVE_IRQ;
}

/*
ISR(TCB0_INT_vect) { 
    ENTER_IRQ;
    RCKid::TCB0_INT_vect();
    LEAVE_IRQ;
}
*/

void setup() { RCKid::initialize(); }
void loop() { RCKid::loop(); }



