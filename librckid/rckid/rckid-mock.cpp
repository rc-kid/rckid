#include "rckid.h"

#if (defined ARCH_MOCK)

#include <iostream>

#include <raylib.h>

#include "rckid/audio/audio.h"
#include "graphics/ST7789.h"
#include "graphics/framebuffer.h"

#include "fs/sd.h"
#include "fs/filesystem.h"


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

    void fatalError(uint32_t code, uint32_t line, char const * file) {
        // directly do the IRQ code in the mock mode
        FrameBuffer<ColorRGB> fb{};
        fb.fill(ColorRGB::Blue());
        Writer w = fb.textMultiline(10,20);
        w << ":( Fatal error:\n\n" 
          << "   code: " << code << "\n\n";
        if (file != nullptr) {
            w << "   line: " << line << "\n"
            << "   file: " << file << "\n\n";
        }
        w << "   Long press home button to turn off,\n   then restart.";
        // reset the display and draw the framebuffer
        ST7789::reset();
        fb.enable();
        fb.render();
        // enter busy wait loop - we need to be restarted now
        while (true) {}
    }


    void powerOff() {

    }

    Writer writeToSerial() {
        return Writer{[](char x) {
            std::cout << x; 
            if (x == '\n')
                std::cout << std::flush;
        }};
    }

    // ============================================================================================
    // Audio
    // ============================================================================================

    bool audio::headphonesActive() {
        return DeviceWrapper:: state_.state.audioEnabled() && DeviceWrapper::state_.state.headphones();         
    }

    void audio::play(audio::OutStream * stream) {
//        for (int i = 0; i < 44; ++i) {
            //stream->fillBuffer(DeviceWrapper::audioBuffer0_, RP_AUDIO_BUFFER_SIZE);
//            for (int i = 0; i < RP_AUDIO_BUFFER_SIZE; i += 2)
//                std::cout << DeviceWrapper::audioBuffer0_[i] << std::endl;
//        }
//        while(true) {};
    }

    void audio::pause() {
    }

    void audio::stop(){
    }

    void audio::record(std::function<void (uint16_t const *, size_t)> f) {

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
        unsigned renderCallbackDepth_ = 0;
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
        std::cout << xStart_ << ", " << xEnd_ << " -- " << yStart_ << ", " << yEnd_ << std::endl;
        x_ = xEnd_ - 1;
        y_ = yStart_;
    }

    void ST7789::leaveContinuousUpdate() {
        updating_ = false;
    }

    void ST7789::initializePinsBitBang() {}

    
    void ST7789::sendMockPixels(uint16_t const * pixels, size_t numPixels) {
        // send the pixels
        while (numPixels-- != 0) {
            ColorRGB rgb = ColorRGB::Raw565(*pixels++);
            framebuffer_[x_ + y_ * 320] = rgb;
            if (++y_ == yEnd_) {
                if (x_-- == xStart_)
                    x_ = xEnd_ - 1;
                y_ = yStart_;
            }
        }
        // only the first pixel send deals with calling further callbacks
        if (++renderCallbackDepth_ == 1) {
            while (renderCallbackDepth_ > 0) {
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
                    x_ = xEnd_ - 1;
                    y_ = yStart_;
                }
                --renderCallbackDepth_;
            }
        }
    }

    // ============================================================================================
    // SD
    // ============================================================================================

    bool SD::initialize() {
        status_ = Status::Ready;
        return true;
    }

    void SD::enableUSBMsc(bool value) {
        status_ = value ? Status::USB : Status::Ready;
    }

} // namespace rckid

// ================================================================================================
// Filesystem
// ================================================================================================

namespace rckid::fs {

    std::string getLabel(Drive drive) {
        switch (drive) {
            case Drive::Device:
                return "Device";
            case Drive::Cartridge:
                return "Cartridge";
            default:
                UNREACHABLE;
        }
    }

    Format getFormat(Drive drive) {
        switch (drive) {
            case Drive::Device:
                return Format::EXFAT;
            default:
                UNIMPLEMENTED;
        }
    }

    uint64_t getTotalCapacity(Drive drive) {
        switch (drive) {
            case Drive::Device:
                return 64_u64 * 1024 * 1024 * 1024;
            default:
                UNIMPLEMENTED;
        }
    }

    uint64_t getFreeCapacity(Drive drive) {
        switch (drive) {
            case Drive::Device:
                return 62_u64 * 1024 * 1024 * 1024;
            default:
                UNIMPLEMENTED;
        }
    }

} // namespace rckid::fs

// ============================================================================================
// Assembly mocks
// ============================================================================================

extern "C" {
    void rckid_mem_fill_32x8(uint32_t * buffer, size_t num, uint32_t value) {
        while (num-- > 0)
            *(buffer++) = value;
    }

    uint8_t const * rckid_color256_to_rgb(uint8_t const * in, uint16_t * out, unsigned numPixels, uint16_t const * palette) {
        while (numPixels-- > 0)
            *(out++) = palette[*in++];
        return in;
    }
    
}

#endif // ARCH_MOCK