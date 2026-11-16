#pragma once

#include <kiddo/kiddo.h>

namespace kiddo {

    /** A simple flashlight with PWM controlled brightness. 
     
        
     */
    class LED {
    public:
        static LED * instance();

        bool active() const { return active_; }

        uint8_t brightness() const { return brightness_; }

        void setActive(bool value);

        void setBrightness(uint8_t value);

    private:
        uint8_t brightness_ = 0;
        bool active_ = false;

    }; // kiddo::LED


} // namespace kiddo