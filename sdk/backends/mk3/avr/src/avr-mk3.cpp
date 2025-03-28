/** RCKid MK III AVR firmware
 
    The AVR chip reads user inputs, controls the pwm backlight and rumbler peripherals, serves as a RTC and monitors the power. As the chip is always on, it is also used as simple device-bound data storage across cartridges and power cycles (modulo battery changes). 

    The AVR operates in two modes - when the device is powered on, the AVR acts as an I2C slave and only responds to RP2350 commands.
    
 */

#include <avr/sleep.h>
#include <avr/delay.h>
#include <avr/interrupt.h>

#undef ASSERT
#define ASSERT(...) if (!(__VA_ARGS__)) {}

#include <platform.h>
#include <platform/peripherals/neopixel.h>
#include <platform/tinydate.h>
#include <platform/ringavg.h>

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
 

    For breadboard testing, this is the layout of the ATTiny3216 chip in SOIC package, where the following pins are missing: 5V_ON, ACCEL_INT, BTN3, BTN2. 

               VDD -|============|- GND
            VDD_EN -|            |- PWM_RUMBLER
             BTN_4 -|            |- PWR_INT
               RGB -|            |- AVR_TX
          BTN_ABXY -| ATTiny3216 |- <UPDI>
          BTN_CTRL -|            |- AVR_INT
             BTN_1 -|            |- BTN_DPAD
           <TOSC1> -|            |- QSPI_SS
           <TOSC2> -|            |- PWM_BACKLIGHT
           I2C_SDA -|============|- I2C_SCL

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

class RCKid {
public:

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

        // TODO AVR TX for debugging


        powerOn();
    }

    static void loop() {
        while (true) {
            cpu::wdtReset();
            // do system tick, if system tick is active and tickCounter is 0, also do the effects tick, which is roughly at 66 frames per second
            if (systemTick() || tickCounter_ == 0) {
                rgbTick();
                rumblerTick();
            }
            // see if there were any I2C commands received and if so, execute
            processI2CCommand();
            // TODO sleep 

        }
    }

    /** Initializes the device power on state. 
     */
    static void powerOn() {
        cli();
        powerVDD(true);
        // initialize the system ticks which wake the AVR up for actions such as reading user inputs or managing rgb & rumbler effects
        startSystemTicks();

        // initialize the PWM subsystem for baclight & rumbler
        initializePWM();
        // TODO set backlight
        
        // disable I2C master and start I2C slave
        TWI0.MCTRLA = 0;
        i2c::initializeSlave(AVR_I2C_ADDRESS);
        TWI0.SCTRLA |= TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm;
        // enable interrupts for ACCEL_INT and PWR_INT
        GPIO_PIN_PINCTRL(AVR_PIN_ACCEL_INT) |= PORT_ISC_RISING_gc;
        GPIO_PIN_PINCTRL(AVR_PIN_PWR_INT) |= PORT_ISC_RISING_gc;

        // enable interrupts back
        sei();
    }

    /** Initializes the device power off state. 
     */
    static void powerOff() {
        cli();
        powerVDD(false);
        stopSystemTicks();
        initializeButtons();
        // TODO initialize I2C master (we can still talk to PMIC, accelerometer and light sensor)
        sei();
    }

    static void powerVDD(bool enable) {
        if (enable) {
            gpio::outputFloat(AVR_PIN_AVR_INT);
            gpio::outputHigh(AVR_PIN_VDD_EN);
        } else {
            gpio::outputFloat(AVR_PIN_AVR_INT);
            gpio::outputFloat(AVR_PIN_VDD_EN);
        }
    }

    static void power5V(bool enable) {
        // TODO
    }

    /** Reboots the RP2350 chip. 
     */
    static void rebootRP() {
        cli();
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
        sei();
    }


    /** Reboots the RP2350 chip into bootloader mode. This is done by pulling the QSPI_CS pin low from the AVR  
     */
    static void bootloaderRP() {
        cli();
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
        sei();
    }

    static void accelInt() {
        state_.status.setAccelInt();
        setIrq();
        // TODO what to do when in master mode?
    }

    static void pwrInt() {
        state_.status.setPwrInt();
        setIrq();
        // TODO what to do when in master mode?
    }

    /** \name System ticks and clocks.
     
        System ticks are used only when the device is on and fire every 5ms. 
     */
    //@{

    static inline uint8_t tickCounter_ = 0;

    /** Starts the system tick on TCA0 with 5ms interval. 
     */
    static void startSystemTicks() {
        if (TCA0.SINGLE.CTRLA & TCA_SINGLE_ENABLE_bm)
            return;
        TCA0.SINGLE.CTRLD = 0;
        TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
        TCA0.SINGLE.PER = 625;
        TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;
        tickCounter_ = 0;
    }

    static void stopSystemTicks() {
        // no harm disabling multiple times
        TCA0.SINGLE.CTRLA = 0;
    }

    static bool systemTick() {
        // do nothing if system tick interrupt is not requested, clear the flag otherwise
        if ((TCA0.SINGLE.INTFLAGS & TCA_SINGLE_OVF_bm) == 0)
            return false;
        TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
        // increment the system tick counter and perform the tick actions
        switch (++tickCounter_) {
            case 1:
                readControlGroup();
                break;
            case 2:
                readABXYGroup();
                break;
            case 3:
                tickCounter_ = 0;
                [[fallthrough]];
            case 0:
                readDPadGroup();
                break;
        }
        return true;
    }

    static void secondTick() {
        ++state_.uptime;
        state_.time.secondTick();
        if (state_.alarm.check(state_.time)) {
            state_.status.setAlarmInt();
            // TODO deal with IRQ, powering device on, etc.
        }

    }
    //@}


    /** \name I2C Communications
     * 
     */
    //@{

    static inline AVRState state_;

    static inline volatile uint8_t i2cTxIdx_ = 0;
    static inline volatile uint8_t i2cRxIdx_ = 0;
    static inline volatile bool i2cCommandReady_ = false;

    static void setIrq() {
        // TODO
    }

    static void clearIrq() {
        // TODO
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
            ++i2cTxIdx_;
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
        switch (state_.buffer[0]) {
            case cmd::Nop::ID:
                break;
            case cmd::PowerOff::ID:
                // TODO
                break;
            case cmd::Sleep::ID:
                // TODO
                break;
            case cmd::ResetRP::ID:
                // TODO
                break;
            case cmd::BootloaderRP::ID:
                // TODO
                break;
            case cmd::ResetAVR::ID:
                // TODO
                break;
            case cmd::BootloaderAVR::ID:
                // TODO
                break;
            case cmd::DebugModeOn::ID:
                // TODO
                break;
            case cmd::DebugModeOff::ID:
                // TODO
                break;
            case cmd::SetBrightness::ID: {
                uint8_t value = cmd::SetBrightness::fromBuffer(state_.buffer).value;
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
            case cmd::ClearAlarm::ID: {
                state_.status.clearAlarmInt();
                break;
            }
        }
        // and reset the command state so that we can read more commands 
        cli();
        i2cRxIdx_ = 0;
        i2cCommandReady_ = false;
        sei();
    }

    //@}

    /** Initializes the button matrix and gets ready to read the control group.
     
        The button matrix is 3 groups of 4 buttons. Namely the BTN_CTRL group selects the Home button and volume up & down side buttons, the BTN_ABXY group selects the A, B and Select & Start buttons and the BTN_DPAD group selects the dpad. 

        When idle, all button pins are in input mode, pulled up. Button groups all output logical high. When a button group is selected, only that group's pin goes to logical 0. We then read the four buttons and reading 0 means the button is pressed (i.e. connected to the button group). The diodes from buttons to group pins ensure that no ghosting happens (i.e. low from one button would through different group enter other button here.

        All button pins are digital and are read as part of system tick. 
     */
    //@{
    static void initializeButtons() {
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
        gpio::low(AVR_PIN_BTN_CTRL);
    }

    static void readControlGroup() {
        uint8_t b = state_.status.b_;
        b = b & ~ 0x7; // clear all buttons
        if (! gpio::read(AVR_PIN_BTN_1))
            b |= AVRState::Status::BTN_HOME;
        if (! gpio::read(AVR_PIN_BTN_2))
            b |= AVRState::Status::BTN_VOLUMEUP;
        if (! gpio::read(AVR_PIN_BTN_3))
            b |= AVRState::Status::BTN_VOLUMEDOWN;
        // check if there has been change
        if (b != state_.status.b_) {
            state_.status.b_ = b;
            // TODO raise IRQ, react to presses
        }
        gpio::high(AVR_PIN_BTN_CTRL);
        gpio::low(AVR_PIN_BTN_ABXY);
    }

    static void readABXYGroup() {
        uint8_t b = state_.status.b_;
        b = b & ~ 0x7; // clear all buttons


        // check if there has been change
        if (b != state_.status.b_) {
            state_.status.b_ = b;
            // TODO raise IRQ, react to presses
        }
        gpio::high(AVR_PIN_BTN_ABXY);
        gpio::low(AVR_PIN_BTN_DPAD);
    }

    static void readDPadGroup() {
        uint8_t b = state_.status.b_;
        b = b & ~ 0x7; // clear all buttons

        // check if there has been change
        if (b != state_.status.b_) {
            state_.status.b_ = b;
            // TODO raise IRQ, react to presses
        }
        gpio::high(AVR_PIN_BTN_DPAD);
        gpio::low(AVR_PIN_BTN_CTRL);
    }
    //@}





    /** \name PWM (Rumbler and Backlight)

        The PWM signals used for backlight and rumbler control are generated by the TCB0 and TCB1 respectively.

        Backlight is pulled low externally, setting the pin to 1 make the backlight work, hence the value is unchanged.  
     */
    //@{

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

    static void rumblerTick() {
        // TODO
    }
    //@}


    /** \name RGB Effects
     
        The LEDs are powered from a 5V step-up generator that is turned off when the LEDs are not used to conserve power (each neopixel takes a bit more than 1mA even if not on at all).
    
     
     */
    //@{

    static constexpr unsigned NUM_RGB_LEDS = 6;

    static inline bool rgbOn_ = false;
    static inline RGBEffect rgbEffect_[NUM_RGB_LEDS];
    static inline platform::ColorStrip<NUM_RGB_LEDS> rgbTarget_;
    static inline platform::NeopixelStrip<NUM_RGB_LEDS> rgb_{AVR_PIN_RGB};

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
        // TODO
    }




    //@}
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

/** Power interrupt ISR.
 */
ISR(PORTA_PORT_vect) {
    static_assert(AVR_PIN_PWR_INT == gpio::A2);
    VPORTA.INTFLAGS = (1 << GPIO_PIN_INDEX(AVR_PIN_PWR_INT));
    RCKid::pwrInt();
}

/** Home button interrupt ISR.
 */
ISR(PORTB_PORT_vect) {
    static_assert(AVR_PIN_BTN_1 == gpio::B4);
    VPORTB.INTFLAGS = (1 << GPIO_PIN_INDEX(AVR_PIN_BTN_1));

}

/** Accel pin ISR.
 */
ISR(PORTC_PORT_vect) {
    static_assert(AVR_PIN_ACCEL_INT == gpio::C5);
    VPORTC.INTFLAGS = (1 << GPIO_PIN_INDEX(AVR_PIN_ACCEL_INT));
    RCKid::accelInt();
}





int main() {
    RCKid::initialize();
    RCKid::loop();
}
