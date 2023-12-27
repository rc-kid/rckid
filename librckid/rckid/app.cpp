#include "tusb.h"

#include "audio.h"
#include "sd.h"

#include "app.h"

namespace rckid {

    /*
    
     We want to call onEnter just before we start first update and onExit after the last update, do we? 
    */

    void BaseApp::run() {
        if (! apps_.empty())
            currentApp_->onBlur();
        // add the application the task of running apps
        apps_.push_back(this);
        onFocus();
        currentApp_ = this;
        if (apps_.size() == 1)
            loop_();
    }

    void BaseApp::exit() {
        if (currentApp_ != this)
            return;
        onBlur();
        apps_.pop_back();
        currentApp_ = nullptr;
    }

    void BaseApp::loop_() {
        nextFpsTick_ = uptime_us() + 1000000;
        fps_ = 0;
        fpsCounter_ = 0;
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
                ST7789::waitVSync();
                currentApp_->render();
                ++fpsCounter_;
            } else if (! apps_.empty()) {
                currentApp_ = apps_.back();
                currentApp_->onFocus();
                drawUs_ = 0;
            }
            systemUs_ = static_cast<unsigned>(afterSystem - frameStart);
            updateUs_ = static_cast<unsigned>(afterUpdate - afterSystem);
            frameUs_ = static_cast<unsigned>(uptime_us() - frameStart);
        }
    }
}