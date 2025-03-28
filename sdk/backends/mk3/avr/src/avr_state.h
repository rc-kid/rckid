#pragma once

#include <platform.h>

class RCKid;

namespace rckid {

    /** AVR State information */

    class AVRState {
    public:
        /** Status information, which is a 2 byte structure containing the most important state information, such as button states and interrupt flags. 
         */
        PACKED(class Status {
        public:


        private:

            void setAlarmInt() { b_ |= ALARM_INT; }
            void setAccelInt() { b_ |= ACCEL_INT; }
            void setPwrInt() { b_ |= PWR_INT; }
            void clearAlarmInt() { b_ &= ~ALARM_INT; }

            friend class ::RCKid;

            static constexpr unsigned BTN_LEFT   = 1;
            static constexpr unsigned BTN_RIGHT  = 2;
            static constexpr unsigned BTN_UP     = 4;
            static constexpr unsigned BTN_DOWN   = 8;
            static constexpr unsigned BTN_A      = 16;
            static constexpr unsigned BTN_B      = 32;
            static constexpr unsigned BTN_SELECT = 64;
            static constexpr unsigned BTN_START  = 128;
            uint8_t a_;

            static constexpr unsigned BTN_HOME       = 1;
            static constexpr unsigned BTN_VOLUMEUP   = 2;
            static constexpr unsigned BTN_VOLUMEDOWN = 4;
            static constexpr unsigned PWR_INT        = 8;
            static constexpr unsigned ACCEL_INT      = 16;
            static constexpr unsigned ALARM_INT      = 32;
            uint8_t b_;

        }); // AVRState::Status

        Status status;

        // TODO add temperature 

        TinyDate time;

        TinyAlarm alarm;

        uint8_t brightness;
        uint32_t uptime;

        /** Communications buffer. This is where commands are stored and where extra commands store the data they wish to transfer to the RP. The size is enough for a single byte command and 32 byte payload. 
         */
        uint8_t buffer[33];

    }; // rckid::AVRState

} // namespace rckid