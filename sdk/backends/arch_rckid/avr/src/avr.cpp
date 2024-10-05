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

#include <platform/ringavg.h>

#include "../../backend_config.h"
#include "../../../../rckid/common.h"
//#include "state.h"
//#include "commands.h"

/** State of the AVR MCU. 
 
    This can be either sleep (the CPU is power off, periodically wakes up every second to increment RTC), in charging mode (when DC is present, the AVR is fully on and monitors charging, but RP power is off) and On, in which case the RP is powered on ans AVR is not sleeping. A separate mode - PowerOn - is entered when the home button is pressed from charging or sleep states (special mode is required because AVR is on and counts ticks as well as checks the VCC for terminating due to low battery level). 
 */
enum class AVRState {
    Sleep, 
    Charging, 
    PowerOn,
    On, 
}; // AVRState

class Status {
public:



private:
    static constexpr uint8_t BTN_HOME = 1 << 0;
    static constexpr uint8_t BTN_VOL_UP = 1 << 1;
    static constexpr uint8_t BTN_VOL_DOWN = 1 << 2;
    static constexpr uint8_t CHARGING = 1 << 3;
    static constexpr uint8_t DC_POWER = 1 << 4;
    static constexpr uint8_t AUDIO_EN = 1 << 5;
    static constexpr uint8_t AUDIO_HEADPHONES = 1 << 6;
    // 7 free
    uint8_t status_ = 0;

    uint8_t vbatt_ = 0;

    uint8_t temp_ = 0;


}; 

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


        // set sleep mode to powerdown (we only need RTC and GPIO Interrupts)
        set_sleep_mode(SLEEP_MODE_POWERDOWN);

        // delay so that voltages stabilize and so on
        cpu::delayMs(100); 
    }

    /** The main loop implementation (including the loop). 
     */
    static void loop() {
        while (true) {
            cpu::wdtReset();


            // sleep if we should, otherwise resume iteration
            if (avrState_ == AVRState::Sleep) {
                sleep_enable();
                sleep_cpu();
            }
        }
    }

    /** Turns the device on, i.e. enables the 3V3 rail and enters PowerOn state. 
     */
    static void powerOn() {

    }

    /** Turns the device off, i.e. disables the 3V3 rail, ensures that SDA and SCL are pulled low  */
    static void powerOff() {

    }




    /** I2C communication interrupt handler. 
     
        
     */





private:

    AVRState avrState_ = AVRState::On;

}; // class RCKid


/** Interrupt for a second tick from the RTC. We need the interrupt routine so that the CPU can wake up and increment the real time clock and uptime. 
 */
ISR(RTC_PIT_vect) {
    RTC.PITINTFLAGS = RTC_PI_bm; // clear the interrupt
    RCKid::systemTick();
}

int main() {
    RCKid::initialize();
    RCKid::loop();
} 
