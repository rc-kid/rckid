#pragma once

#include <rckid/rckid.h>

namespace rckid {

    class Timer {
    public:

        uint64_t us() const {
            return us_;
        }

        TinyTime time() const {
            return TinyTime{static_cast<uint32_t>(us_ / 1000000)};
        }

        void tick() {
            if (!paused_) {
                uint64_t now = time::uptimeUs();
                us_ += now - last_;
                last_ = now;
            }
        }

        void start() {
            us_ = 0;
            last_ = time::uptimeUs();
            paused_ = false;
        }

        bool paused() const { return paused_; }

        void pause() {
            if (!paused_) {
                tick();
                paused_ = true;
            }
        }

        void resume() {
            if (paused_) {
                last_ = time::uptimeUs();
                paused_ = false;
            }
        }
    
    private:
        uint64_t us_ = 0;
        uint64_t last_ = 0;
        bool paused_ = true;

    }; //rckid::Timer


} // namespacr rckid