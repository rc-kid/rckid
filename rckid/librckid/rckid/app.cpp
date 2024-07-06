#include "graphics/ST7789.h"
//#include "audio.h"
#include "app.h"
#include "device_wrapper.h"

uint32_t nextFpsTick_ = 0;
uint32_t fpsCounter_ = 0;

namespace rckid {

    void BaseApp::loop() {
        // handle the focus - blur previous app if any 
        BaseApp * last = currentApp_;
        if (last != nullptr) {
            LOG("leaving parent app - onblur");
            last->onBlur();
        }
        // switch to current app, set focus
        currentApp_ = this;
        TRACE("Entering app - onfocus");
        onFocus();
        nextFpsTick_ = uptimeUs() + 1000000;
        fpsCounter_ = 0;
        TRACE("Entering app loop");
        // the actual loop - loop as long as the currentApp is set (exit clears it)
        while (currentApp_ != nullptr) {
            if (nextFpsTick_ <= uptimeUs()) {
                nextFpsTick_ += 1000000;
                stats::fps_ = fpsCounter_;
                fpsCounter_ = 0;
            }
            // wait for the tick to be done so that I2C can be used while in update method
            MEASURE_TIME(stats::waitTickUs_, DeviceWrapper::waitTickDone());
            // call update
            MEASURE_TIME(stats::updateUs_, update());
            // start new tick (no I2C allowed after update)
            tick();
            // wait for the display update to be done so that draw has complete access
            MEASURE_TIME(stats::waitRenderUs_, ST7789::waitUpdateDone());
            MEASURE_TIME(stats::drawUs_, draw());
            stats::waitVSyncUs_ = 0;
            MEASURE_TIME(stats::renderUs_, render());
            stats::renderUs_ -= stats::waitVSyncUs_; 
            ++fpsCounter_;
        }
        TRACE("leaving app loop (on blur)");
        // patch current app to ourselves so that the state blur sees is consistent and blur the current app
        currentApp_ = this;
        onBlur();
        // go back to previous app and handle its focus, if any
        currentApp_ = last;
        if (currentApp_) {
            TRACE("entering parent app - onfocus");
            currentApp_->onFocus();
        }
    }

    void BaseApp::exit() {
        currentApp_ = nullptr;
    }

} // namespace rckid
