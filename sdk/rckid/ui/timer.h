#pragma once

#include "../rckid.h"

namespace rckid {

    /** A simple timer. 
     
        Support single, or continuous mode. Together with interpolation can be used to drive animations.  

        NOTE that the timer must be updated manually in order to progress, but since it uses the system clock the frequency of the updates does not matter for precission. It's a good bahit to update the timers before they are used in every frame, etc. 
     */
    class Timer {
    public:

        Timer(int32_t duration): duration_{duration} {}

        void start() { start(duration_); }

        void start(int32_t duration) {
            t_ = 0;
            duration_ = duration;
            lastCheck_ = uptimeUs();
            state_ = State::Single;
        }

        void startContinuous() { startContinuous(duration_); }

        void startContinuous(int32_t duration) {
            t_ = 0;
            duration_ = duration;
            lastCheck_ = uptimeUs();
            state_ = State::Continuous;
        }

        void stop() { state_ = State::Off; }

        bool running() const { return state_ != State::Off; }

        /** Updates the timer, returns true if the timer's duration has passed, i.e. when a single timer is done or, or when a continuous moves past the duration again
         */
        bool update() {
            if (state_ == State::Off)
                return true;
            uint32_t t = uptimeUs();      
            uint32_t d = (t - lastCheck_) / 1000;
            lastCheck_ = t;
            bool result = false;
            t_ += d;
            if (t_ > duration_) {
                result = true;
                // if we are in single mode, ensure we are at duration when done, otherwise start again
                if (state_ == State::Single) {
                    state_ = State::Off;
                    t_ = duration_;
                } else {
                    t_ -= duration_;
                }
            }
            return result;
        }

        /** Returns the duration of the animation in milliseconds. 
         */
        int32_t duration() const { return duration_; }

        void setDuration(int32_t value) {
            duration_ = value;
            if (t_ > duration_)
                t_ -= duration_;
        }

        int32_t t() const { return t_; }
        
    private:

        enum class State {
            Off,
            Single, 
            Continuous,
        }; 

        State state_{State::Off};

        int32_t t_;
        int32_t duration_; 
        uint32_t lastCheck_;

    }; // rckid::Timer

} // namespace rckid