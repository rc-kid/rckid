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
        //ST7789::processEvents();
    }

    void tick() {
        MEASURE_TIME(stats::tickUs_, 
            ++stats::ticks_;
            DeviceWrapper::lastState_ = DeviceWrapper::state_.state;
            DeviceWrapper::state_.state.setBtnSel(IsKeyDown(KEY_SPACE));    
            DeviceWrapper::state_.state.setBtnStart(IsKeyDown(KEY_ENTER));    
            DeviceWrapper::state_.state.setBtnA(IsKeyDown(KEY_A));    
            DeviceWrapper::state_.state.setBtnB(IsKeyDown(KEY_B));    
            DeviceWrapper::state_.state.setBtnLeft(IsKeyDown(KEY_LEFT));    
            DeviceWrapper::state_.state.setBtnRight(IsKeyDown(KEY_RIGHT));    
            DeviceWrapper::state_.state.setBtnUp(IsKeyDown(KEY_UP));    
            DeviceWrapper::state_.state.setBtnDown(IsKeyDown(KEY_DOWN));    
            if (WindowShouldClose())
                std::exit(-1);

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
        ColorRGB framebuffer_[320 * 240];
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
    }

    void ST7789::initializePinsBitBang() {}

    void ST7789::sendMockPixels(uint16_t const * pixels, size_t numPixels) {
        while (numPixels-- != 0) {
            ColorRGB rgb = ColorRGB::Raw565(*pixels++);
            framebuffer_[x_ + y_ * (xEnd_ - xStart_)] = rgb;
            if (++y_ == yEnd_) {
                if (x_-- == xStart_)
                    x_ = xEnd_ - 1;
                y_ = yStart_;
            }
        }
        if (cb_()) {
            updating_ = false;
            BeginDrawing();
            for (int x = 0; x < 320; ++x) {
                for (int y = 0; y < 240; ++y) {
                    ColorRGB c = framebuffer_[x + y * 320];
                    DrawRectangle(x * 2, y * 2, 2, 2, (Color) { c.r(), c.g(), c.b(), 255});
                }
            }
            EndDrawing();

        }
            /*

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
            */
        //irqReady_ = true;
    }

} // namespace rckid

// ============================================================================================
// Assembly mocks
// ============================================================================================

extern "C" {
    void rckid_mem_fill_32x8(uint32_t * buffer, size_t num, uint32_t value) {
        while (num-- > 0)
            *(buffer++) = value;
    }
}


#endif // ARCH_MOCK