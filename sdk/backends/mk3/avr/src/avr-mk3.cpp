/** RCKid MK III AVR firmware
 
    The AVR chip reads user inputs, controls the pwm backlight and rumbler peripherals, serves as a RTC and monitors the power. As the chip is always on, it is also used as simple device-bound data storage across cartridges and power cycles (modulo battery changes). 

    The AVR operates in two modes - when the device is powered on, the AVR acts as an I2C slave and only responds to RP2350 commands.
    
 */

#include <avr/sleep.h>
#include <avr/delay.h>
#include <avr/interrupt.h>

#include <platform.h>
#include <platform/peripherals/neopixel.h>
#include <platform/tinydate.h>
#include <platform/ringavg.h>

#include "../../backend_config.h"
#include "../../backend_internals.h"

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

    }

    static void loop() {
        while (true) {
            cpu::wdtReset();

        }
    }


    /** Initializes the device power on state. 
     */
    static void powerOn() {
        cli();
        // ensure the AVR_IRQ pin is in input mode
        gpio::outputFloat(AVR_PIN_AVR_INT);
        // enable the VDD (3V3)
        gpio::outputHigh(AVR_PIN_VDD_EN);
        
        // disable I2C master and start I2C slave
        TWI0.MCTRLA = 0;
        i2c::initializeSlave(AVR_I2C_ADDRESS);
        TWI0.SCTRLA |= TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm;
        // enable interrupts back
        sei();
    }

    /** Initializes the device power off state. 
     */
    static void powerOff() {
        cli();
        gpio::outputFloat(AVR_PIN_AVR_INT);
        gpio::outputFloat(AVR_PIN_VDD_EN);
        initializeButtons();
        // TODO initialize I2C master (we can still talk to PMIC, accelerometer and light sensor)
        sei();
    }

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
    //@}

}; // class RCKid



int main() {
    RCKid::initialize();
    RCKid::loop();
}
