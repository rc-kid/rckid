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
#include "rckid/graphics/color.h"
#include "rckid/internals.h"

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

namespace {

    class State {
    public:
        bool btnUp = false;
        bool btnDown = false;
        bool btnLeft = false;
        bool btnRight = false;
        bool btnA = false;
        bool btnB = false;
        bool btnSelect = false;
        bool btnStart = false;
        bool btnVolumeUp = false;
        bool btnVolumeDown = false;
        bool btnHome = false;

        bool buttonState(rckid::Btn b) {
            using namespace rckid;
            switch (b) {
                case Btn::Up: return btnUp;
                case Btn::Down: return btnDown;
                case Btn::Left: return btnLeft;
                case Btn::Right: return btnRight;
                case Btn::A: return btnA;
                case Btn::B: return btnB;
                case Btn::Select: return btnSelect;
                case Btn::Start: return btnStart;
                case Btn::VolumeUp: return btnVolumeUp;
                case Btn::VolumeDown: return btnVolumeDown;
                case Btn::Home: return btnHome;
                default:
                    UNREACHABLE;
            }
        }
    }; // State

} // anonymous namespace

namespace rckid {

    void displayDraw();

    namespace {
        State state_;
        State lastState_;
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
        systemMalloc_ = true;
        PollInputEvents();
        systemMalloc_ = false;
        lastState_ = state_;
        state_.btnUp = IsKeyDown(KEY_UP);
        state_.btnDown = IsKeyDown(KEY_DOWN);
        state_.btnLeft = IsKeyDown(KEY_LEFT);
        state_.btnRight = IsKeyDown(KEY_RIGHT);
        state_.btnA = IsKeyDown(KEY_A);
        state_.btnB = IsKeyDown(KEY_B);
        state_.btnSelect = IsKeyDown(KEY_SPACE);
        state_.btnStart = IsKeyDown(KEY_ENTER);
        state_.btnVolumeUp = IsKeyDown(KEY_PAGE_UP);
        state_.btnVolumeDown = IsKeyDown(KEY_PAGE_DOWN);
        state_.btnHome = IsKeyDown(KEY_H);
    }

    void yield() {
        // TODO
    }

    void fatalError(uint32_t error, uint32_t line, char const * file) {
        // clear all memory arenas to clean up space, this is guarenteed to succeed as the SDK creates memory arena when it finishes initialization    
        while (memoryInsideArena())
            memoryLeaveArena();
        bsod(error, line, file, nullptr);
        systemMalloc_ = true;
        while (! WindowShouldClose())
            PollInputEvents();
        systemMalloc_ = false;
        std::exit(EXIT_FAILURE);
    }

    void fatalError(Error error, uint32_t line, char const * file) {
        fatalError(static_cast<uint32_t>(error), line, file);
    }

    uint32_t uptimeUs() {
        using namespace std::chrono;
        static auto first = steady_clock::now();
        return static_cast<uint32_t>(duration_cast<microseconds>(steady_clock::now() - first).count()); 
    }

    uint32_t random() {
        return std::rand();
    }

    Writer debugWrite() {
        return Writer([](char c) {
            std::cout << c;
            if (c == '\n')
                std::cout.flush();
        });
    }

    // io

    bool btnDown(Btn b) {
        return state_.buttonState(b);
    }

    bool btnPressed(Btn b) {
        return state_.buttonState(b) && ! lastState_.buttonState(b);
    }

    bool btnReleased(Btn b) {
        return !state_.buttonState(b) && lastState_.buttonState(b);
    }

    int16_t accelX() { return 0; }
    int16_t accelY() { return 0; }
    int16_t accelZ() { return 0; }

    /** Returns the gyroscope readings. 
     */
    int16_t gyroX() { return 0; }
    int16_t gyroY() { return 0; }
    int16_t gyroZ() { return 0; }

    // display 

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

    // LEDs

    void ledsOff() {
        UNIMPLEMENTED;
    }

    void ledSetEffect(Btn b, LEDEffect const & effect) {
        UNIMPLEMENTED;
    }

    void ledSetEffects(LEDEffect const & dpad, LEDEffect const & a, LEDEffect const & b, LEDEffect const & select, LEDEffect const & start){
        UNIMPLEMENTED;
    }

    // Rumbler

    void rumble(uint8_t intensity, uint16_t duration, unsigned repetitions, uint16_t offDuration) {
        UNIMPLEMENTED;
    }

    // accelerated functions

    #include "rckid/accelerated.inc.h"
    MEM_FILL_8
    MEM_FILL_16
    MEM_FILL_32

}