#pragma once

#include "../rckid.h"
#include "fixedint.h"

namespace rckid {

    /** A simple timer. 
     
        Support single, or continuous mode. Together with interpolation can be used to drive animations.  

        NOTE that the timer must be updated manually in order to progress, but since it uses the system clock the frequency of the updates does not matter for precission. It's a good habit to update the timers before they are used in every frame, etc. 
     */
    class Timer {
    public:

        Timer(uint32_t duration): duration_{duration} {}

        void start() { start(duration_); }

        void start(uint32_t duration) {
            t_ = 0;
            duration_ = duration;
            lastCheck_ = uptimeUs();
            state_ = State::Single;
        }

        void startContinuous() { startContinuous(duration_); }

        void startContinuous(uint32_t duration) {
            t_ = 0;
            duration_ = duration;
            lastCheck_ = uptimeUs();
            state_ = State::Continuous;
        }

        void stop() { state_ = State::Off; t_ = duration_; }

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
        uint32_t duration() const { return duration_; }

        void setDuration(uint32_t value) {
            duration_ = value;
            if (t_ > duration_)
                t_ -= duration_;
        }

        uint32_t t() const { return t_; }
        
    private:

        enum class State {
            Off,
            Single, 
            Continuous,
        }; 

        State state_{State::Off};

        uint32_t t_;
        uint32_t duration_; 
        uint32_t lastCheck_;

    }; // rckid::Timer

    class Animation1D {
    public:
        using Interpolation = std::function<FixedInt(Timer const &, int, int)>;

        Animation1D() = default;

        Animation1D(Coord start, Coord end, Interpolation interp):
            start_{start}, end_{end}, value_{start}, interp_{interp} {
        }

        void reverse() {
            Coord t = start_;
            start_ = end_;
            end_ = t;
            value_ = start_;
        }

        Coord value() const { return value_; }
        
        Coord update(Timer const & t) {
            value_ = interp_(t, start_, end_).round();
            return value_;
        }

    private:
        Coord start_;
        Coord end_;
        Coord value_;
        Interpolation interp_;
    }; // rckid::Animation1D

    class Animation2D {
    public:
        using Interpolation = std::function<FixedInt(Timer const &, int, int)>;

        Animation2D() = default;

        Animation2D(Point start, Point end, Interpolation interp):
            start_{start}, end_{end}, value_{start}, interp_{interp} {
        }

        void reverse() {
            Point t = start_;
            start_ = end_;
            end_ = t;
            value_ = start_;
        }

        Point value() const { return value_; }

        Point update(Timer const & t) {
            value_.x = interp_(t, start_.x, end_.x).round();
            value_.y = interp_(t, start_.y, end_.y).round();
            return value_;
        }
    private:
        Point start_;
        Point end_;
        Point value_;
        Interpolation interp_;
    }; // rckid::Animation2D

} // namespace rckid