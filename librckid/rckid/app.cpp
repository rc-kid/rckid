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

    void BaseApp::loop_() {
        nextFpsTick_ = uptime_us() + 1000000;
        fps_ = 0;
        fpsCounter_ = 0;
        currentApp_ = apps_.back();
        size_t initialSize = apps_.size();
        while (apps_.size() >= initialSize) {
            uint64_t frameStart = uptime_us();
            if (frameStart >= nextFpsTick_) {
                fps_ = fpsCounter_;
                fpsCounter_ = 0;
                nextFpsTick_ += 1000000;
            }
            // new tick (check with AVR and peripherals, etc.)
            Device::tick();
            // process system events
            tud_task(); // USB Mass Storage
            Audio::processEvents(); // Audio buffers




            uint64_t afterSystem = uptime_us();
            // update the current app
            currentApp_->update();
            uint64_t afterUpdate = uptime_us();
            // if the update did not change the current application, draw, otherwise try to focus new app
            if (currentApp_ != nullptr) {
                ST7789::waitUpdateDone();
                uint64_t beforeDraw = uptime_us();
                currentApp_->draw();
                uint64_t afterDraw = uptime_us();
                drawUs_ = static_cast<unsigned>(afterDraw - beforeDraw); 
                ++fpsCounter_;
            } else if (! apps_.empty()) {
                currentApp_ = apps_.back();
                drawUs_ = 0;
            }
            systemUs_ = static_cast<unsigned>(afterSystem - frameStart);
            updateUs_ = static_cast<unsigned>(afterUpdate - afterSystem);
            frameUs_ = static_cast<unsigned>(uptime_us() - frameStart);
        }
    }
}