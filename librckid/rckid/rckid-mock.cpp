#include "rckid.h"

#if (defined ARCH_MOCK)

#include <iostream>
#include <raylib.h>

#include "graphics/ST7789.h"

namespace rckid {

    void initialize() {
        // TODO initialize the mock display & friends
        InitWindow(640, 480, "RCKid");
        SetTargetFPS(60);
    }

    void yield() {
        // TODO do we do anything here actually? 
        ST7789::processEvents();
    }

    void tick() {
        MEASURE_TIME(stats::tickUs_, 
            ++stats::ticks_;
        );
    }

    void powerOff() {

    }

    // ============================================================================================
    // DeviceWrapper
    // ============================================================================================

    void DeviceWrapper::waitTickDone() {
        // do nothing, tick is always done in mock 
    }

    // ============================================================================================
    // ST7789
    // ============================================================================================

    // state so that we can render properly
    namespace {
        int xStart_ = 0;
        int yStart_ = 0;
        int x_ = 319; 
        int y_ = 0;
        int xEnd_ = 320;
        int yEnd_ = 240;
        bool drawing_ = false;
    }

    void ST7789::initialize() {

    }

    void ST7789::reset() {

    }

    void ST7789::configure(DisplayMode mode) {
        displayMode_ = mode;
    }

    void ST7789::fill(ColorRGB color) {

    }

    void ST7789::enterContinuousUpdate(Rect rect) {
        // TODO set W H and stuff
        xStart_ = rect.left();
        yStart_ = rect.top();
        xEnd_ = rect.right();
        yEnd_ = rect.bottom();
        x_ = xEnd_ - 1;
        y_ = 0;
    }

    void ST7789::leaveContinuousUpdate() {
        updating_ = false;
        irqReady_ = false;
    }

    void ST7789::initializePinsBitBang() {}

    void ST7789::sendMockPixels(uint16_t const * pixels, size_t numPixels) {
        if (!drawing_) {
            BeginDrawing();
            drawing_ = true;
        }
        while (numPixels-- != 0) {
            ColorRGB rgb = ColorRGB::Raw565(*pixels++);
            switch (displayMode_) {
                case DisplayMode::Native_2X_RGB565:
                    DrawRectangle(x_ * 2, y_ * 2, 2, 2, (Color) { rgb.r(), rgb.g(), rgb.b(), 255});
                    if (++y_ == yEnd_) {
                        if (x_-- == xStart_)
                            x_ = xEnd_ - 1;
                        y_ = yStart_;
                    }
                case DisplayMode::Native_RGB565:
                    DrawRectangle(x_ * 2, y_ * 2, 2, 2, (Color) { rgb.r(), rgb.g(), rgb.b(), 255});
                    if (++y_ == yEnd_) {
                        if (x_-- == xStart_)
                            x_ = xEnd_ - 1;
                        y_ = yStart_;
                    }
                    break;
                default:
                    UNREACHABLE;
            }
        }
        irqReady_ = true;
    }

    void ST7789::processEvents() {
        if (irqReady_) {
            irqReady_ = false;
            if (cb_()) {
                updating_ = false;
                drawing_ = false;
                EndDrawing();
                stats::displayUpdateUs_ = static_cast<unsigned>(uptimeUs() - stats::updateStart_);
            }
        }
    }

} // namespace rckid

#endif // ARCH_MOCK