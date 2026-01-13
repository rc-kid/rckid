#pragma once

#include <platform.h>
#include <platform/tinydate.h>

class RCKid;

namespace rckid {

    /** AVR version.
     
        The AVR version is returned as part of the AVR extended state right after status itself so that it can be used by cartridges to refuse to run on devices with incompatible AVR firmware, which can lead to different commands being issued, etc. 

        Ensure to bump the version whenever there are backwards incompatible changes to the AVR firmware such as new commands, status layout changes, etc.

        Changelist

        01 - prohibited interval support added to budget, SetBudget command sets entire budget settings, PowerOffAck command added and commands renumbered 

     */
    static constexpr uint8_t AVR_VERSION = 0x01;

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

            bool vusb()           const { return b_ & VUSB; }
            bool charging()       const { return b_ & CHARGING; }
            bool debugMode()      const { return b_ & DEBUG_MODE; }
            bool bootloaderMode() const { return b_ & BOOTLOADER_MODE; }
            bool heartbeatMode()  const { return b_ & HEARTBEAT_MODE; }

            /** Power off interrupt. 
             
                When received, the RP2350 should finalize all its tasks, save state, etc. and then turn the device off via sending the poweroff command. Once the power off interrupt is received, it cannot be cleared. If the power off command is not received in time (see RCKID_POWEROFF_TIMEOUT_FPS), the AVR will forcibly power off the device anyways. 
             */
            bool powerOffInt()   const { return c_ & PWROFF_INT; }
            bool accelInt()      const { return c_ & ACCEL_INT; }
            bool alarmInt()      const { return c_ & ALARM_INT; }
            bool secondInt()     const { return c_ & SECOND_INT; }
            bool heartbeatInt()  const { return c_ & HEARTBEAT_INT; }

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

            void clearInterrupts() { c_ &= ~(PWROFF_INT | ACCEL_INT | SECOND_INT); }

            void clearAlarmInterrupt() { c_ &= ~ALARM_INT; }

            void clearHeartbeatInterrupt() { c_ &= ~HEARTBEAT_INT; }

            /** Clears the pressed state of all buttons. This method is useful for rapidfire where every interval, we reset the button state in last state giving us btnPressed again.
             */
            void clearButtons() {
                a_ = 0;
                b_ &= ~(BTN_HOME | BTN_VOLUMEUP | BTN_VOLUMEDOWN);
            }

            /** Returns true if any of the button state changed between this state and the other one. 
             */
            bool controlChange(Status const & other) const {
                if (a_ != other.a_)
                    return true;
                if ((b_ & (BTN_HOME | BTN_VOLUMEUP | BTN_VOLUMEDOWN)) != (other.b_ & (BTN_HOME | BTN_VOLUMEUP | BTN_VOLUMEDOWN)))
                    return true;
                return false;
            }

            uint32_t buttonChangeDown(Status const & other) const {
                uint32_t last = other.a_ | ((other.b_ & (BTN_HOME | BTN_VOLUMEUP | BTN_VOLUMEDOWN)) << 8);
                uint32_t curr = a_ | ((b_ & (BTN_HOME | BTN_VOLUMEUP | BTN_VOLUMEDOWN)) << 8);
                return curr & (~last);
            }

            uint32_t buttonChangeUp(Status const & other) const {
                uint32_t last = other.a_ | ((other.b_ & (BTN_HOME | BTN_VOLUMEUP | BTN_VOLUMEDOWN)) << 8);
                uint32_t curr = a_ | ((b_ & (BTN_HOME | BTN_VOLUMEUP | BTN_VOLUMEDOWN)) << 8);
                return (~curr) & last;
            }

        private:

        /** Clears the interrupts.
             */
            void clearAllInterrupts() { c_ = 0; }

            void setPowerOffInt() { c_ |= PWROFF_INT; }
            void setAccelInt() { c_ |= ACCEL_INT; }
            void setAlarmInt() { c_ |= ALARM_INT; }
            void setSecondInt() { c_ |= SECOND_INT; }
            void setHeartbeatInt() { c_ |= HEARTBEAT_INT; }

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

            void setVUsb(bool value) { value ? (b_ |= VUSB) : (b_ &= ~VUSB); }
            void setCharging(bool value) { value ? (b_ |= CHARGING) : (b_ &= ~CHARGING); }
            void setDebugMode(bool value) { value ? (b_ |= DEBUG_MODE) : (b_ &= ~DEBUG_MODE); }
            void setBootloaderMode(bool value) { value ? (b_ |= BOOTLOADER_MODE) : (b_ &= ~BOOTLOADER_MODE); }
            void setHeartbeatMode(bool value) { value ? (b_ |= HEARTBEAT_MODE) : (b_ &= ~HEARTBEAT_MODE); }

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
            static constexpr uint8_t HEARTBEAT_MODE  = 8;
            static constexpr uint8_t VUSB            = 16;
            static constexpr uint8_t CHARGING        = 32;
            static constexpr uint8_t DEBUG_MODE      = 64;
            static constexpr uint8_t BOOTLOADER_MODE = 128;
            uint8_t b_ = 0;

            static constexpr uint8_t PWROFF_INT      = 1;
            static constexpr uint8_t ACCEL_INT       = 2;
            static constexpr uint8_t ALARM_INT       = 4;
            static constexpr uint8_t SECOND_INT      = 8;
            static constexpr uint8_t HEARTBEAT_INT   = 16;
            uint8_t c_ = 0;

            uint8_t vcc_ = 0;

        }); // AVRState::Status

        PACKED(struct AudioSettings {
        public:

            uint8_t volumeHeadphones() const { return volume_ & 0x0f; }
            uint8_t volumeSpeaker()     const { return (volume_ >> 4) & 0x0f; }

            void setVolumeHeadphones(uint8_t v) { volume_ = (volume_ & 0xf0) | (v & 0x0f); }
            void setVolumeSpeaker(uint8_t v) { volume_ = (volume_ & 0x0f) | ((v & 0x0f) << 4); }

        private:
            
            uint8_t volume_ = 0x88;

        }); // AVRState::AudioSettings

        PACKED(struct BudgetSettings {
            /** Budget in seconds for budgeted apps. This is kept by the AVR and reset at midnight each day. Certain apps count their run time towards the budget.
             */
            uint32_t remainingSeconds;

            /** Daily budget, value to which the budget resets automatically every midnight. 
             */
            uint32_t dailySeconds = 3600; 

            /** Budget prohibited interval
             */
            DailyIntervalHM prohibitedInterval;

        }); // AVRState::BudgetSettings

        Status status;

        /** AVR version to ensure incompatible cartridge & device will not talk to each other.
         */
        uint8_t avrVersion = AVR_VERSION;

        /** Temperature in x10 (mind the precision of the measurement at 0.5C).
         */
        int16_t temp;

        /** Game budget settings.
         */
        BudgetSettings budget;

        TinyDateTime time;

        TinyAlarm alarm;

        uint32_t uptime = 0;

        uint8_t brightness = RCKID_DISPLAY_BRIGHTNESS;

        AudioSettings audio;

        /** BCD device pin (0000-9999), or 0xffff if the pin is disabled
         */
        uint16_t pin = 0xffff;

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