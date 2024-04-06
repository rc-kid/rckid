#if (defined LIBRCKID_MOCK)

#include <iostream>

#include "raylib.h"

#include "rckid.h"
#include "app.h"
#include "ST7789.h"
#include "audio.h"
#include "rckid/graphics/framebuffer.h"
#include "fonts/Iosevka_Mono6pt7b.h"

// fake section start & end to keep linker happy

uint8_t __vram_start__;
uint8_t __vram_end__;

namespace rckid {

    void irqDMADone_();

    bool dmaDoneDisplay_ = false;

    void start() {
        // prepare the VRAM 

        // TODO initialize the mock display & friends
        InitWindow(640, 480, "RCKid");
        resetVRAM();

        int errorCode = setjmp(rckid::Device::fatalError_);
        if (errorCode != 0) 
            rckid::Device::BSOD(errorCode);
        rckid_main();
    }

    void yield() {
        // TODO do we do anything here actually? 
    }

    Writer writeToUSBSerial() {
        return Writer([](char x) {
            std::cout << x;
            if (x == '\n')
                std::cout << std::flush;
        });
    }

    // power management ---------------------------------------------------------------------------

    void powerOff() {
        // TODO exit the app
        UNIMPLEMENTED;
    }

    // memory -------------------------------------------------------------------------------------

    size_t freeHeap() {
        //UNIMPLEMENTED;
        return 0;
    }

    uint8_t __vram__[RCKID_VRAM_SIZE];

    size_t freeVRAM() { return (__vram__ + RCKID_VRAM_SIZE) - Device::vramPtr_; }

    void resetVRAM() { Device::vramPtr_ = __vram__; }

    bool isVRAMPtr(void * ptr) { return (ptr >= static_cast<void*>(& __vram_start__)) && (ptr < static_cast<void*>(& __vram_end__)); }

    // 
  
    void Device::tick() {
        lastState_ = state_.state;
        state_.state.setBtnSel(IsKeyDown(KEY_SPACE));    
        state_.state.setBtnStart(IsKeyDown(KEY_ENTER));    
        state_.state.setBtnA(IsKeyDown(KEY_A));    
        state_.state.setBtnB(IsKeyDown(KEY_B));    
        state_.state.setBtnLeft(IsKeyDown(KEY_LEFT));    
        state_.state.setBtnRight(IsKeyDown(KEY_RIGHT));    
        state_.state.setBtnUp(IsKeyDown(KEY_UP));    
        state_.state.setBtnDown(IsKeyDown(KEY_DOWN));    
        if (WindowShouldClose())
            std::exit(-1);
        // TODO get state
        // UNIMPLEMENTED;
    }

    void Device::BSOD(int code) {
        // TODO show BSOD in window?
        std::cout << "BSOD:" << std::endl << std::endl;
        std::cout << "If you ran this on RCKid, you would have been treated to its blue screen of death." << std::endl;
        std::cout << "Error code: " << code << std::endl;
        std::cout << "File:       " << fatalErrorFile_ << ":" << fatalErrorLine_ << std::endl;
        resetVRAM();
        FrameBuffer<ColorRGB> fb{Bitmap<ColorRGB>{320, 240, MemArea::VRAM}};
        fb.setFg(ColorRGB::White());
        fb.setFont(Iosevka_Mono6pt7b);
        fb.setBg(ColorRGB::Blue());
        fb.fill();
        fb.textMultiline(0,0) << ":(\n\n"
            << "FATAL ERROR " << code << "\n\n"
            << "File: " << fatalErrorFile_ << "\n"
            << "Line: " << fatalErrorLine_; 
        ST7789::reset();
        ST7789::enterContinuousUpdate();
        fb.render();
        while(true) {
        }
        exit(EXIT_FAILURE);
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
    }

    void ST7789::leaveContinuousUpdate() {

    }

    void ST7789::initializePinsBitBang() {}

    void ST7789::sendMockPixels(uint16_t const * pixels, size_t numPixels) {
        BeginDrawing();
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
        EndDrawing();
        dmaDoneDisplay_ = true;
        irqDMADone_();
    }

    // ============================================================================================
    // Audio Driver
    // ============================================================================================

    void audio::initialize() {

    }

    void audio::startPlayback(SampleRate rate, uint16_t * buffer, size_t bufferSize, CallbackPlay cb) {

    }

    void audio::stopPlayback() {

    }

    void audio::startRecording(SampleRate rate) {

    }

    void audio::stopRecording() {

    }

    void audio::processEvents() {

    }

    void audio::configureDMA(int dma, int other, uint16_t const * buffer, size_t bufferSize) {

    }

    void audio::setSampleRate(uint16_t rate) {

    }

    // ============================================================================================
    // DMA handler
    // ============================================================================================

    void irqDMADone_() {
        if (dmaDoneDisplay_) {
            dmaDoneDisplay_ = false;
            if (ST7789::cb_()) {
                ST7789::updating_ = false;
                stats::displayUpdateUs_ = static_cast<unsigned>(uptimeUs() - stats::updateStart_);
            }
        }
        // TODO audio
    }

} // namespace rckid

int main() {
    rckid::start();
    UNREACHABLE;
}

#endif // RCKID_MOCK