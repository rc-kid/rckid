#include "app.h"

namespace rckid {

#if ARCH_RCKID_2
    void rckid_mkII_waitTickDone();
#endif

    void App::loop() {
        //if (current_)
        //    current_->onBlur();
        App * lastApp = current_;
        current_ = this;
        onFocus();
        // add extra tick to ensure that button presses are cleared. We really need only the first tick, the second tick is there to ensure that any async processes of the first tick will actually finish before the update method call in the loop (tick waits for completion of previous one)
        tick();
        tick();
        // reste the FPS counter & period
        uint32_t lastFrame = uptimeUs();
        uint32_t currentFrame = 0;
        uint32_t currentFps = 0;
        while (current_ == this) {
            MEASURE_TIME(updateUs_,     update());
            MEASURE_TIME(tickUs_,       tick());
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
        // make sure the last frame of the app has been rendered properly (otherwise the rendering might stop mid frame when the app swaps and may create weird artefacts on the display)
        displayWaitUpdateDone();
        // current can only be nullptr at this point, if it is anything else than nullptr, we have a problem as the exit mechanic when running modal apps has been broken somehow. 
        ASSERT(current_ == nullptr);
        onBlur();
        current_ = lastApp;
        //if (current_)
        //    current_->onFocus();
    }
} // namespace rckid