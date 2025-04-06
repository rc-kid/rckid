#pragma once

#include <platform.h>
#include <platform/tinydate.h>

class RCKid;

namespace rckid {

    /** AVR State information */

    class AVRState {
    public:
        /** Status information, which is a 2 byte structure containing the most important state information, such as button states and interrupt flags. 
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

            bool pwrInt()        const { return b_ & PWR_INT; }
            bool accelInt()      const { return b_ & ACCEL_INT; }
            bool alarmInt()      const { return b_ & ALARM_INT; }

            bool debugMode() const { return b_ & DEBUG_MODE; }
            bool bootloaderMode() const { return b_ & BOOTLOADER_MODE; }

        private:

            void setAlarmInt() { b_ |= ALARM_INT; }
            void setAccelInt() { b_ |= ACCEL_INT; }
            void setPwrInt() { b_ |= PWR_INT; }
            void clearAlarmInt() { b_ &= ~ALARM_INT; }

            bool setDPadButtons(bool left, bool right, bool up, bool down) {
                uint8_t aa = (a_ & 0xf0) | ((left ? BTN_LEFT : 0) | (right ? BTN_RIGHT : 0) | (up ? BTN_UP : 0) | (down ? BTN_DOWN : 0));
                if (aa == a_)
                    return false;
                a_ = aa;
                return true;
            }
            bool setABXYtButtons(bool a, bool b, bool select, bool start) {
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

            void setDebugMode(bool value) { value ? (b_ |= DEBUG_MODE) : (b_ &= ~DEBUG_MODE); }
            void setBootloaderMode(bool value) { value ? (b_ |= BOOTLOADER_MODE) : (b_ &= ~BOOTLOADER_MODE); }

            friend class ::RCKid;

            static constexpr unsigned BTN_LEFT        = 1;
            static constexpr unsigned BTN_RIGHT       = 2;
            static constexpr unsigned BTN_UP          = 4;
            static constexpr unsigned BTN_DOWN        = 8;
            static constexpr unsigned BTN_A           = 16;
            static constexpr unsigned BTN_B           = 32;
            static constexpr unsigned BTN_SELECT      = 64;
            static constexpr unsigned BTN_START       = 128;
            uint8_t a_ = 0;

            static constexpr unsigned BTN_HOME        = 1;
            static constexpr unsigned BTN_VOLUMEUP    = 2;
            static constexpr unsigned BTN_VOLUMEDOWN  = 4;
            static constexpr unsigned PWR_INT         = 8;
            static constexpr unsigned ACCEL_INT       = 16;
            static constexpr unsigned ALARM_INT       = 32;
            static constexpr unsigned DEBUG_MODE      = 64;
            static constexpr unsigned BOOTLOADER_MODE = 128;
            uint8_t b_ = 0;

        }); // AVRState::Status


        Status status;

        /** Temperature in x10 (mind the precision of the measurement at 0.5C).
         */
        int16_t temp;

        TinyDate time;

        TinyAlarm alarm;

        uint8_t brightness = RCKID_DISPLAY_BRIGHTNESS;
        uint32_t uptime = 0;

        /** Communications buffer. This is where commands are stored and where extra commands store the data they wish to transfer to the RP. The size is large enough to 1 byte command, 2 byte page number and 128 byte data, which is necessary to program flash pages on the device.
         */
        uint8_t buffer[131];

    }; // rckid::AVRState

} // namespace rckid