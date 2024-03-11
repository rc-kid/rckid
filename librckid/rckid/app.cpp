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
        Stats::nextFpsTick_ = time_us_32() + 1000000;
        Stats::fps_ = 0;
        Stats::fpsCounter_ = 0;
        // do our loop 
        currentApp_ = this;
        while (true) {
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
            ++Stats::fpsCounter_;
            Stats::systemUs_ = tSys;
            Stats::updateUs_ = tUpdate;
            Stats::drawUs_ = tDraw; 
            Stats::frameUs_ = static_cast<unsigned>(time_us_32() - frameStart);
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
