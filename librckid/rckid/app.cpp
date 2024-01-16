#include "tusb.h"

#include "audio.h"
#include "sd.h"

#include "app.h"

namespace rckid {

     void BaseApp::run() {
        BaseApp * last = currentApp_;
        // reset current app so that main loop picks up
        currentApp_ = nullptr;
        apps_.push_back(this);
        onFocus(last);
        // if there is already main loop running, blur the last app and return immediately so that it will pick up the newly installed app and run it, otherwise we have to start the main loop ourselves
        if (last != nullptr)
            last->onBlur(this);
        else
            loop_();
    }

    void BaseApp::exit() {
        currentApp_ = nullptr;
        BaseApp * last = apps_.back();
        apps_.pop_back();
        BaseApp * next = apps_.empty() ? nullptr : apps_.back();
        last->onBlur(next);
        if (next != nullptr)
            next->onFocus(last);
    }

#define CALCULATE_TIME(...) [&](){ uint32_t start__ = time_us_32(); __VA_ARGS__; return static_cast<unsigned>(time_us_32() - start__); }()

    void BaseApp::loop_() {
        Stats::nextFpsTick_ = time_us_32() + 1000000;
        Stats::fps_ = 0;
        Stats::fpsCounter_ = 0;
        currentApp_ = apps_.back();
        size_t initialSize = apps_.size();
        while (apps_.size() >= initialSize) {
            uint32_t frameStart = time_us_32();
            if (frameStart >= Stats::nextFpsTick_) {
                Stats::fps_ = Stats::fpsCounter_;
                Stats::fpsCounter_ = 0;
                Stats::nextFpsTick_ += 1000000;
            }
            uint32_t tSys = CALCULATE_TIME(
                Device::tick();
            );
            uint32_t tUpdate = CALCULATE_TIME(
                currentApp_->update();
            );
            if (currentApp_ == nullptr) {
                currentApp_ = apps_.back();
                Stats::drawUs_ = 0;
                continue;
            }
            ST7789::waitUpdateDone();
            uint32_t tDraw = CALCULATE_TIME(
                currentApp_->draw();
            );
            currentApp_->render();
            ++Stats::fpsCounter_;
            Stats::systemUs_ = tSys;
            Stats::updateUs_ = tUpdate;
            Stats::drawUs_ = tDraw; 
            Stats::frameUs_ = static_cast<unsigned>(time_us_32() - frameStart);
       }
    }

} // namespace rckid