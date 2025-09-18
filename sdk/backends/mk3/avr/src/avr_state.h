#pragma once

#include <platform.h>
#include <platform/tinydate.h>

class RCKid;

namespace rckid {

    /** AVR State information */

    PACKED(class AVRState {
    public:
        /** Status information, which is a 2 byte structure containing the most important state information, such as button states and interrupt flags. 
         
            The status is 3 bytes, first two bytes are mostly button states, with the second button also containing the bootloader & debug mode flags. Those are identical for the whole power on state, but we have space there. The last byte is the state of interrupts and it requires special handling. Its value is only or'd with new updates and the only way to clear it is to call the clearInterrupts() method. 

            Furthermore on the AVR, this byte can only be changed when interrupts are disabled, as it is automatically cleared when the state is transmitted. 
         */
        PACKED(class Status {
        public:

            bool btnLeft()   const { return a_ & BTN_LEFT; }
            bool btnRight()  const { return a_ & BTN_RIGHT; }
            bool btnUp()     const { return a_ & BTN_UP; }
            bool btnDown()   const { return a_ & BTN_DOWN; }
            bool btnA()      const { return a_ & BTN_A; }
            bool btnB()      const { return a_ & BTN_B; }
            bool btnSelect() const { return a_ & BTN_SELECT; }
            bool btnStart()  const { return a_ & BTN_START; }

            bool btnHome()       const { return b_ & BTN_HOME; }
            bool btnVolumeUp()   const { return b_ & BTN_VOLUMEUP; }
            bool btnVolumeDown() const { return b_ & BTN_VOLUMEDOWN; }

            bool pwrInt()        const { return c_ & PWR_INT; }
            bool accelInt()      const { return c_ & ACCEL_INT; }
            bool alarmInt()      const { return c_ & ALARM_INT; }
            bool secondInt()     const { return c_ & SECOND_INT; }

            bool lowBattery()     const { return b_ & LOW_BATT; }
            bool vusb()           const { return b_ & VUSB; }
            bool charging()       const { return b_ & CHARGING; }
            bool debugMode()      const { return b_ & DEBUG_MODE; }
            bool bootloaderMode() const { return b_ & BOOTLOADER_MODE; }

            uint16_t vcc() const {
                return (vcc_ == 0) ? 0 : (vcc_ + 245);
            }

            /** Updates the AVR status. This changes the values of the buttons immediately, but ors the interrupts so that no interrupts are ever lost. To clear the interrupts, use the clearInterrupts() method.
             
             */
            void updateWith(Status const & other) {
                a_ = other.a_;
                b_ = other.b_;
                vcc_ = other.vcc_;
                // or interrupts, but update the debug & bootloader states
                c_ = c_ | other.c_;
            }

            /** Clears the interrupts.
             */
            void clearInterrupts() { c_ = 0; }

        private:

            void setPwrInt() { c_ |= PWR_INT; }
            void setAccelInt() { c_ |= ACCEL_INT; }
            void setAlarmInt() { c_ |= ALARM_INT; }
            void setSecondInt() { c_ |= SECOND_INT; }

            bool setDPadButtons(bool left, bool right, bool up, bool down) {
                uint8_t aa = (a_ & 0xf0) | ((left ? BTN_LEFT : 0) | (right ? BTN_RIGHT : 0) | (up ? BTN_UP : 0) | (down ? BTN_DOWN : 0));
                if (aa == a_)
                    return false;
                a_ = aa;
                return true;
            }
            bool setABXYButtons(bool a, bool b, bool select, bool start) {
                uint8_t aa = (a_ & 0x0f) | ((a ? BTN_A : 0) | (b ? BTN_B : 0) | (select ? BTN_SELECT : 0) | (start ? BTN_START : 0));
                if (aa == a_)
                    return false;
                a_ = aa;
                return true;
            }

            bool setControlButtons(bool home, bool volumeUp, bool volumeDown) {
                uint8_t bb = (b_ & 0xf8) | ((home ? BTN_HOME : 0) | (volumeUp ? BTN_VOLUMEUP : 0) | (volumeDown ? BTN_VOLUMEDOWN : 0));
                if (bb == b_)
                    return false;
                b_ = bb;
                return true;
            }

            void setLowBattery(bool value) { value ? (b_ |= LOW_BATT) : (b_ &= ~LOW_BATT); }
            void setVUsb(bool value) { value ? (b_ |= VUSB) : (b_ &= ~VUSB); }
            void setCharging(bool value) { value ? (b_ |= CHARGING) : (b_ &= ~CHARGING); }
            void setDebugMode(bool value) { value ? (b_ |= DEBUG_MODE) : (b_ &= ~DEBUG_MODE); }
            void setBootloaderMode(bool value) { value ? (b_ |= BOOTLOADER_MODE) : (b_ &= ~BOOTLOADER_MODE); }

            void setVcc(uint16_t vx100) {
                if (vx100 < 250)
                    vcc_ = 0;
                else if (vx100 > 500)
                    vcc_ = 255;
                else
                    vcc_ = static_cast<uint8_t>(vx100 - 245);
            }

            friend class ::RCKid;

            static constexpr uint8_t BTN_LEFT        = 1;
            static constexpr uint8_t BTN_RIGHT       = 2;
            static constexpr uint8_t BTN_UP          = 4;
            static constexpr uint8_t BTN_DOWN        = 8;
            static constexpr uint8_t BTN_A           = 16;
            static constexpr uint8_t BTN_B           = 32;
            static constexpr uint8_t BTN_SELECT      = 64;
            static constexpr uint8_t BTN_START       = 128;
            uint8_t a_ = 0;

            static constexpr uint8_t BTN_HOME        = 1;
            static constexpr uint8_t BTN_VOLUMEUP    = 2;
            static constexpr uint8_t BTN_VOLUMEDOWN  = 4;
            static constexpr uint8_t LOW_BATT        = 8;
            static constexpr uint8_t VUSB            = 16;
            static constexpr uint8_t CHARGING        = 32;
            static constexpr uint8_t DEBUG_MODE      = 64;
            static constexpr uint8_t BOOTLOADER_MODE = 128;
            uint8_t b_ = 0;

            static constexpr uint8_t PWR_INT         = 1;
            static constexpr uint8_t ACCEL_INT       = 2;
            static constexpr uint8_t ALARM_INT       = 4;
            static constexpr uint8_t SECOND_INT      = 8;
            uint8_t c_ = 0;

            uint8_t vcc_ = 0;

        }); // AVRState::Status


        Status status;

        /** Temperature in x10 (mind the precision of the measurement at 0.5C).
         */
        int16_t temp;

        /** Budget in seconds for budgeted apps. This is kept by the AVR and reset at midnight each day. Certain apps count their run time towards the budget.
         */
        uint32_t budget;

        /** Daily budget, value to which the budget resets automatically every midnight. 
         */
        uint32_t dailyBudget = 3600; 

        TinyDateTime time;

        TinyAlarm alarm;

        uint8_t brightness = RCKID_DISPLAY_BRIGHTNESS;
        uint32_t uptime = 0;

    }); // rckid::AVRState

    /** The entire transferrable state, which consists of the AVR state itself as well as the communications buffer. On the AVR side, this is used to store the state & buffer close together so that it can be send as one consecutive memory block. 
     */
    PACKED(class TransferrableState : public AVRState {
    public:

        /** Communications buffer. This is where commands are stored and where extra commands store the data they wish to transfer to the RP. The size is large enough to 1 byte command, 2 byte page number and 128 byte data, which is necessary to program flash pages on the device.
         */
        uint8_t buffer[131];

    }); 

} // namespace rckid