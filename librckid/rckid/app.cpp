#include "ST7789.h"
#include "audio.h"
#include "app.h"

namespace rckid {

    void App::run() {
        App * last = currentApp_;
        if (last != nullptr)
            last->onBlur();
        resetVRAM();
        onFocus();
        // reset stats
        stats::nextFpsTick_ = time_us_32() + 1000000;
        stats::fps_ = 0;
        stats::fpsCounter_ = 0;
        // do our loop 
        currentApp_ = this;
        while (true) {
            uint32_t frameStart = time_us_32();
            if (frameStart >= stats::nextFpsTick_) {
                stats::fps_ = stats::fpsCounter_;
                stats::fpsCounter_ = 0;
                stats::nextFpsTick_ += 1000000;
            }
            uint32_t tSys = CALCULATE_TIME(
                Device::tick();
            );
            uint32_t tUpdate = CALCULATE_TIME(
                update();
            );
            // exit the loop if we have exitted (currentApp_ is null)
            if (currentApp_ == nullptr)
                break;
            
            ST7789::waitUpdateDone();

            uint32_t tDraw = CALCULATE_TIME(
                draw();
            );
            render();
            ++stats::fpsCounter_;
            stats::systemUs_ = tSys;
            stats::updateUs_ = tUpdate;
            stats::drawUs_ = tDraw; 
            stats::frameUs_ = static_cast<unsigned>(time_us_32() - frameStart);
        }
        onBlur();
        resetVRAM();
        currentApp_ = last;
        if (currentApp_)
            currentApp_->onFocus();
    }

    void App::exit() {
        currentApp_ = nullptr;
    }

} // namespace rckid
