#include "app.h"

#define MEASURE_TIME(whereTo, ...) { \
    uint32_t start__ = uptimeUs(); \
    __VA_ARGS__; \
    whereTo = uptimeUs() - start__; \
}

namespace rckid {

    void App::run() {
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
            MEASURE_TIME(updateUs_,     update());
            MEASURE_TIME(tickUs_,       tick());
            MEASURE_TIME(waitRenderUs_, displayWaitUpdateDone());
            MEASURE_TIME(drawUs_,       draw());
            MEASURE_TIME(waitVSyncUs_,  displayWaitVSync());
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