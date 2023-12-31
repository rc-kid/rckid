#pragma once

#include "../rckid.h"

namespace rckid {

    class Animation {
    public:

        Animation(unsigned duration): duration_{duration} {}

        void start() { start(duration_); }

        void start(unsigned duration) {
            t_ = 0;
            duration_ = duration_;
            lastCheck_ = time_us_32();
            state_ = State::Single;
        }

        void startContinuous() { startContinuous(duration_); }

        void startContinuous(unsigned duration) {
            t_ = 0;
            duration_ = duration_;
            lastCheck_ = time_us_32();
            state_ = State::ContinuousUp;
        }

        void stop() { state_ = State::Off; }

        bool running() { return state_ != State::Off; }

        /** Updates the animation, returns true if the animation's duration has passed, i.e. when a single animation is done or, or when a continuous animation changes direction. 
         */
        bool update() {
            if (state_ == State::Off)
                return true;
            uint32_t t = time_us_32();      
            uint32_t d = (t - lastCheck_) / 1000;
            lastCheck_ = t;
            switch (state_) {
                case State::Single:
                    t_ += d;
                    lastCheck_ = t;
                    if (t_ > duration_) {
                        t_ = duration_;
                        state_ = State::Off;
                        return true;
                    } else {
                        return false;
                    }
                case State::ContinuousUp:
                case State::ContinuousDown:
                    /// TODO: Fix the continuous animation modes
                    break;
                default: 
                    break;
            }
            return false;
        }


        /** Returns the duration of the animation in milliseconds. 
         */
        unsigned duration() const { return duration_; }

        void setDuration(unsigned value) {
            duration_ = value;
            if (t_ > duration_)
                t_ = duration_;
        }

        template<typename T> 
        T interpolate(T start, T end, Interpolation i = Interpolation::Cos) {
            return rckid::interpolate(start, end, t_ * 1000 / duration_, i);
        }

        
    private:

        enum class State {
            Off,
            Single, 
            ContinuousUp,
            ContinuousDown,
        }; 

        State state_{State::Off};

        unsigned t_;
        unsigned duration_; 
        uint32_t lastCheck_;

    }; // rckid::Animation

} // namespace rckid