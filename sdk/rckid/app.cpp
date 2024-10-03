#include "app.h"

#define MEASURE_TIME(whereTo, ...) { \
    uint32_t start__ = uptimeUs(); \
    __VA_ARGS__; \
    whereTo = uptimeUs() - start__; \
}

namespace rckid {

#if ARCH_RCKID_2
    void rckid_mkII_waitTickDone();
#endif

    void App::loop() {
        if (current_)
            current_->onBlur();
        App * lastApp = current_;
        current_ = this;
        onFocus();
        // reste the FPS counter & period
        uint32_t lastFrame = uptimeUs();
        uint32_t currentFrame = 0;
        uint32_t currentFps = 0;
        while (current_ == this) {
#if ARCH_RCKID_2
            // this is a rather dirty hack to get around the fact that on mkII the app needs talking to the AVR chip for some of its functionality via I2C, which is also used during tick to get peripheral information from AVR and other sensors. To make sure the I2C commands and the tick requests are not overlapping, on mkII the tick happens *after* the update, i.e. during draw and render when no commands should be issued, and we wait before calling the update method for the tick to be done. 
            rckid_mkII_waitTickDone();
            MEASURE_TIME(updateUs_,     update());
            MEASURE_TIME(tickUs_,       tick());
#else
            MEASURE_TIME(updateUs_,     update());
            MEASURE_TIME(tickUs_,       tick());
#endif
            MEASURE_TIME(waitRenderUs_, displayWaitUpdateDone());
            MEASURE_TIME(drawUs_,       draw());
            // don't wait for Vsync here as the rendering might want to preprocess the graphic data first
            MEASURE_TIME(renderUs_,     render());
            ++currentFps;
            uint32_t uus = uptimeUs();
            currentFrame += uus - lastFrame;
            lastFrame = uus;
            if (currentFrame >= 1000000) {
                fps_ = currentFps;
                currentFrame -= 1000000;
                currentFps = 0;
            }
        }
        onBlur();
        current_ = lastApp;
        if (current_)
            current_->onFocus();
    }

} // namespace rckid