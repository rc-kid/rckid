#include "rckid.h"

#if (defined ARCH_MOCK)

#include <iostream>

#include <raylib.h>

#include "rckid/audio/audio.h"
#include "graphics/ST7789.h"
#include "graphics/framebuffer.h"

#include "fs/sd.h"


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

    void ST7789::setUpdateRegion(Rect rect) {
        xStart_ = rect.left();
        yStart_ = rect.top();
        xEnd_ = rect.right();
        yEnd_ = rect.bottom();
    }

    void ST7789::beginUpdate() {
        // reset the update cursor to the beginning according to the mode
        switch (displayMode_) {
            case DisplayMode::Native_RGB565:
            case DisplayMode::Native_2X_RGB565:
                x_ = xEnd_ - 1;
                y_ = yStart_;
                break;
            case DisplayMode::Natural_RGB565:
            case DisplayMode::Natural_2X_RGB565:
                x_ = xStart_;
                y_ = yStart_;
                break;
            default:
                UNREACHABLE;
        }
    }

    void ST7789::endUpdate() {
        BeginDrawing();
        for (int x = 0; x < 320; ++x) {
            for (int y = 0; y < 240; ++y) {
                ColorRGB c = framebuffer_[x + y * 320];
                DrawRectangle(x * 2, y * 2, 2, 2, (Color) { c.r(), c.g(), c.b(), 255});
            }
        }
        EndDrawing();
    }

    void ST7789::update(ColorRGB const * pixels, uint32_t numPixels) {
        while (numPixels-- != 0) {
            framebuffer_[x_ + y_ * 320] = *pixels++;
            // move to next pixel
            switch (displayMode_) {
                case DisplayMode::Native_RGB565:
                    if (++y_ == yEnd_) {
                        if (x_-- == xStart_)
                            x_ = xEnd_ - 1;
                        y_ = yStart_;
                    }
                    break;
                case DisplayMode::Native_2X_RGB565:
                    UNIMPLEMENTED;
                    break;
                case DisplayMode::Natural_RGB565:
                    if (++x_ == xEnd_) {
                        if (++y_ == yEnd_) 
                            y_ = yStart_;
                        x_ = xStart_;
                    }
                    break;
                case DisplayMode::Natural_2X_RGB565:
                    UNIMPLEMENTED;
                    break;
                default:
                    UNREACHABLE;
            }
        }
    }

    void ST7789::beginDMAUpdate() {
        beginUpdate();
    }

    void ST7789::endDMAUpdate() {
        endUpdate();
    }

    void ST7789::initializePinsBitBang() {
        // NOP
    }

    
    void ST7789::sendMockPixels(ColorRGB const * pixels, size_t numPixels) {
        // send the pixels
        update(pixels, numPixels);
        //  do callback
        if (cb_ != nullptr)
            cb_();
        // check if we are done updating
        if (--updating_ == 0)
            endUpdate();
    }

    // ============================================================================================
    // SD
    // ============================================================================================

    // status of the SD card
    SD::Status sdStatus_ = SD::Status::NotPresent;
    // number of blocks, not very useful in mock mode
    uint32_t sdNumBlocks_ = 0;

    /** Getters for the state shared between the SD card class wrapper and the implementation here
     */
    SD::Status SD::status() { return sdStatus_; }
    bool SD::ready() { return sdStatus_ == Status::Ready; }
    uint32_t SD::numBlocks() { return sdNumBlocks_; }

    bool SD::initialize() {
        sdStatus_ = Status::Ready;
        return true;
    }

    void SD::enableUSBMsc(bool value) {
        sdStatus_ = value ? Status::USB : Status::Ready;
    }


    uint64_t SD::getCapacity() { 
        UNIMPLEMENTED; 
    }

    uint64_t SD::getFreeCapacity() { 
        UNIMPLEMENTED;
    }

    SD::Format SD::getFormatKind() {
        UNIMPLEMENTED;
    }

    std::string SD::getLabel() {
        UNIMPLEMENTED;
    }

    // ============================================================================================
    // SD File
    // ============================================================================================

    // ============================================================================================
    // SD Folder
    // ============================================================================================

} // namespace rckid

#endif // ARCH_MOCK