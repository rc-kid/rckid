/** 
    \section raylib_backend Raylib backend
    \addtogroup backends
 
    A fantasy console backend that uses RayLib for the graphics, audio and other aspects of the device. Should work anywhere raylib does. 
 */

#ifndef ARCH_FANTASY
#error "You are building fantasy (RayLib) backend without the indicator macro"
#endif

#include <cstdlib>
#include <iostream>
#include <chrono>
#include <thread>

#include <raylib.h>

#include "rckid/rckid.h"

extern "C" {

    // start in system malloc so that any pre-main initialization does not pollute rckid's heap
    static thread_local bool systemMalloc_ = true;

    // Replace the malloc and free with own versions that check if we are in the SDK/user code and therefore use rckid's malloc implementation, or if this is a 3rd party library and we should default to the system's malloc implementation. This is done by wrapping all raylib calls with set & clear of the systemMalloc_ flag 
    /// TODO: only works on linux for now
#if (defined __linux__)
    extern void *__libc_malloc(size_t);
    extern void __libc_free(void *);

    void * malloc(size_t numBytes) {
        if (systemMalloc_)
            return __libc_malloc(numBytes);
        else 
            return rckid::malloc(numBytes);
    }

    void free(void * ptr) {
        if (rckid::memoryIsOnHeap(ptr))
            rckid::free(ptr);
        else 
            __libc_free(ptr);
    }
#endif 
} // extern C - memory

namespace rckid {

    namespace {
        DisplayMode displayMode_ = DisplayMode::Off;
        DisplayUpdateCallback displayCallback_;
        uint8_t displayBrightness_ = 255;
        size_t displayUpdating_ = 0; 
        Rect displayRect_ = Rect::WH(320, 240);
        size_t displayMax_ = 320 * 240;
        int displayUpdateX_ = 319;
        int displayUpdateY_ = 0;
        Image displayImg_;
        Texture displayTexture_;
        std::chrono::steady_clock::time_point displayLastVSyncTime_;
    }



    void displayDraw();

    void initialize() {
        systemMalloc_ = true;
        InitWindow(640, 480, "RCKid");
        systemMalloc_ = false;
        displayImg_ = GenImageColor(320, 240, BLACK);
        displayTexture_ = LoadTextureFromImage(displayImg_);
        displayLastVSyncTime_ = std::chrono::steady_clock::now();
    }

    void tick() {
        systemMalloc_ = true;
        if (WindowShouldClose())
            std::exit(-1);
        systemMalloc_ = false;
        PollInputEvents();
    }

    void yield() {
        // TODO
    }

    uint32_t uptimeUs() {
        using namespace std::chrono;
        static auto first = steady_clock::now();
        return static_cast<uint32_t>(duration_cast<microseconds>(steady_clock::now() - first).count()); 
    }

    Writer debugWrite() {
        return Writer([](char c) {
            std::cout << c;
            if (c == '\n')
                std::cout.flush();
        });
    }

    // io

    bool btnDown([[maybe_unused]] Btn b) {
        return false;
    }

    bool btnPressed([[maybe_unused]] Btn b) {
        return false;
    }

    bool btnReleased([[maybe_unused]] Btn b) {
        return false;
    }


    void drawPixel(int x, int y, ColorRGB c) {
        Color rc;
        rc.a = 255;
        rc.r = c.r();
        rc.g = c.g();
        rc.b = c.b();
        DrawRectangle(x * 2, y * 2, 2, 2, rc);
    }

    void displayDraw() {
        systemMalloc_ = true;
        UpdateTexture(displayTexture_, displayImg_.data);
        BeginDrawing();
        DrawTextureEx(displayTexture_, {0, 0}, 0, 2.0, WHITE);
        EndDrawing();
        SwapScreenBuffer();
        systemMalloc_ = false;
    }

    DisplayMode displayMode() { return displayMode_; }

    void displaySetMode(DisplayMode mode) {
        displayMode_ = mode;
        // TODO if display is off, mark it as off 
    }

    uint8_t displayBrightness() { return displayBrightness_; }

    void displaySetBrightness(uint8_t value) { displayBrightness_ = value; }

    Rect displayUpdateRegion() { return displayRect_; }

    void displaySetUpdateRegion(Rect region) { 
        displayRect_ = region; 
        displayMax_ = region.w * region.h;
        ASSERT(region.w > 0 && region.h > 0 && region.x >= 0 && region.y >= 0);
        ASSERT(region.bottom() <= 240 && region.right() <= 320);
        displayUpdateX_ = displayRect_.right() - 1;
        displayUpdateY_ = displayRect_.top();
    }

    bool displayUpdateActive() { return displayUpdating_ > 0; }

    // there is no VSYNC on raylib, it's being handled by Begin & EndDrawing instead
    void displayWaitVSync() { 
        yield();
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - displayLastVSyncTime_).count();
        // hardwired for ~60 fps...  
        if (elapsed < 16666)
            std::this_thread::sleep_for(std::chrono::microseconds(16666 - elapsed));
        displayLastVSyncTime_ = std::chrono::steady_clock::now();
    }

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels) {
        ++displayUpdating_;
        // update the pixels
        while (numPixels != 0) {
            ImageDrawPixel(&displayImg_, displayUpdateX_, displayUpdateY_, { pixels->r(), pixels->g(), pixels->b(), 255});
            ++pixels;
            --numPixels;
            if (++displayUpdateY_ == displayRect_.bottom()) {
                displayUpdateY_ = displayRect_.top();
                if (--displayUpdateX_ < displayRect_.left())
                    displayUpdateX_ = displayRect_.right() - 1; 
            }
        }
        // check if this is the first update call, in which case call all the other updates (as long as the callback generates a new update) and when no more updates are scheduled, actually redraw the display. Note that if the update is not the first, no callbacks are called
        if (displayUpdating_ == 1) {
            while (true) {
                size_t updatingOld = displayUpdating_;
                if (displayCallback_)
                    displayCallback_();
                if (updatingOld == displayUpdating_)
                    break;
            }
            displayUpdating_ = 0;
            displayDraw();
        }
    }

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels, DisplayUpdateCallback callback) {
        displayCallback_ = callback;
        displayUpdate(pixels, numPixels);
    }





    // accelerated functions

    #include "rckid/accelerated.inc.h"
    MEM_FILL_8
    MEM_FILL_16
    MEM_FILL_32

}