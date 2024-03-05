#if (defined LIBRCKID_MOCK)

#include <iostream>

#include "rckid.h"
#include "app.h"
#include "ST7789.h"
#include "audio.h"
#include "rckid/graphics/framebuffer.h"
#include "fonts/Iosevka_Mono6pt7b.h"


extern uint8_t vram_[];
extern size_t rckid_vram_size_;

extern void rckid_main();



namespace rckid {

    void start() {
        int errorCode = setjmp(rckid::Device::fatalError_);
        if (errorCode != 0) 
            rckid::Device::BSOD(errorCode);
        rckid_main();
    }

    void initialize() {
        // TODO initialize the mock display & friends
        InitWindow(640, 480, "RCKid");
        resetVRAM();
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

    size_t freeVRAM() {
        return (vram_ + rckid_vram_size_) - reinterpret_cast<uint8_t *>(Device::vramNext_);
    }

    void resetVRAM() {
        Device::vramNext_ = reinterpret_cast<uint32_t*>(&vram_);
    }

    uint32_t * allocateVRAM(size_t numBytes) {
        // ensure alignment
        if (numBytes % 4 != 0)
            numBytes += 4 - (numBytes % 4);
        if (numBytes > freeVRAM())
            FATAL_ERROR(VRAM_OUT_OF_MEMORY);
        uint32_t * result = Device::vramNext_;
        Device::vramNext_ += numBytes / 4;
        return result;
    }

    bool isVRAMPtr(void * ptr) {
        return (ptr >= vram_) && (ptr < vram_ + rckid_vram_size_);
    }

    // 
  
    void cpuOverclock(unsigned hz, bool overvolt) {
        // does nothing, overclocking is ignored in mock mode 
    }

    void Device::tick() {
        lastState_ = state_;
        state_.status.setBtnSelect(IsKeyDown(KEY_SPACE));    
        state_.status.setBtnStart(IsKeyDown(KEY_ENTER));    
        state_.status.setBtnA(IsKeyDown(KEY_A));    
        state_.status.setBtnB(IsKeyDown(KEY_B));    
        state_.status.setDpadLeft(IsKeyDown(KEY_LEFT));    
        state_.status.setDpadRight(IsKeyDown(KEY_RIGHT));    
        state_.status.setDpadUp(IsKeyDown(KEY_UP));    
        state_.status.setDpadDown(IsKeyDown(KEY_DOWN));    
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
        FrameBuffer<ColorRGB> fb{Bitmap<ColorRGB>{320,240, reinterpret_cast<uint32_t*>(&vram_)}};
        fb.setFg(ColorRGB::White());
        fb.setFont(Iosevka_Mono6pt7b);
        fb.setBg(ColorRGB::Blue());
        fb.fill();
        fb.textMultiline(0,0) << ":(\n\n"
            << "FATAL ERROR " << code << "\n\n"
            << "File: " << fatalErrorFile_ << "\n"
            << "Line: " << fatalErrorLine_; 
        ST7789::reset();
        ST7789::enterContinuousMode(ST7789::Mode::Single);
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
        int w_ = 320;
        int h_ = 240;
        ST7789::DisplayMode displayMode_ = ST7789::DisplayMode::Native;
        ST7789::Mode mode_ = ST7789::Mode::Single;
    }


    void ST7789::initialize() {

    }

    void ST7789::reset() {

    }

    void ST7789::fill(ColorRGB color) {

    }

    void ST7789::enterContinuousMode(Rect rect, ST7789::Mode mode) {
        // TODO set W H and stuff
        mode_ = mode;
    }

    void ST7789::leaveContinuousMode() {

    }

    void ST7789::initializePinsBitBang() {}

    void ST7789::irqDMADone() {
        if (cb_)
            cb_();
        else 
            updating_ = false;
    }

    void ST7789::sendMockPixels(uint16_t const * pixels, size_t numPixels) {
        BeginDrawing();
        while (numPixels-- != 0) {
            ColorRGB rgb = ColorRGB::Raw565(*pixels++);
            switch (mode_) {
                case Mode::Double:
                    DrawRectangle(x_ * 2, y_ * 2, 2, 2, (Color) { rgb.r(), rgb.g(), rgb.b(), 255});
                    if (++y_ == h_) {
                        if (x_-- == 0)
                            x_ = w_ - 1;
                        y_ = 0;
                    }
                case Mode::Single: 
                    DrawRectangle(x_ * 2, y_ * 2, 2, 2, (Color) { rgb.r(), rgb.g(), rgb.b(), 255});
                    if (++y_ == h_) {
                        if (x_-- == 0)
                            x_ = w_ - 1;
                        y_ = 0;
                    }
                    break;
            }
        }
        EndDrawing();
        irqDMADone();
    }

    // ============================================================================================
    // Audio Driver
    // ============================================================================================

    void Audio::initialize() {

    }

    void Audio::startPlayback(SampleRate rate, uint16_t * buffer, size_t bufferSize, CallbackPlay cb) {

    }

    void Audio::stopPlayback() {

    }

    void Audio::startRecording(SampleRate rate) {

    }

    void Audio::stopRecording() {

    }

    void Audio::processEvents() {

    }

    void Audio::configureDMA(int dma, int other, uint16_t const * buffer, size_t bufferSize) {

    }

    void Audio::setSampleRate(uint16_t rate) {

    }

    void Audio::irqDMADone() {

    }
} // namespace rckid

int main() {
    rckid::initialize();
    rckid::start();
    return EXIT_SUCCESS;
}

#endif // RCKID_MOCK