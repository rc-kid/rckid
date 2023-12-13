#include "audio.h"

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
        nextFpsTick_ = to_us_since_boot(get_absolute_time()) + 1000000;
        fps_ = 0;
        fpsCounter_ = 0;
        size_t initialSize = apps_.size();
        while (apps_.size() >= initialSize) {
            uint64_t frameStart = to_us_since_boot(get_absolute_time());
            if (frameStart >= nextFpsTick_) {
                fps_ = fpsCounter_;
                fpsCounter_ = 0;
                nextFpsTick_ += 1000000;
            }
            // process system events
            Audio::processEvents();
            
            uint64_t afterSystem = to_us_since_boot(get_absolute_time());
            systemUs_ = static_cast<unsigned>(afterSystem - frameStart);
            // update the current app
            currentApp_->update();
            uint64_t afterUpdate = to_us_since_boot(get_absolute_time());
            updateUs_ = static_cast<unsigned>(afterUpdate - afterSystem);
            // if the update did not change the current application, draw, otherwise try to focus new app
            if (currentApp_ != nullptr) {
                currentApp_->draw();
                ++fpsCounter_;
            } else if (! apps_.empty()) {
                currentApp_ = apps_.back();
                currentApp_->onFocus();
            }
            uint64_t afterDraw = to_us_since_boot(get_absolute_time());
            drawUs_ = static_cast<unsigned>(afterDraw - afterUpdate); 
        }
    }
}