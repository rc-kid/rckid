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
#include <fstream>

#include <raylib.h>

#include <platform/buffer.h>

#include "rckid/rckid.h"
#include "rckid/graphics/color.h"
#include "rckid/internals.h"
#include "rckid/filesystem.h"

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
        using namespace rckid;
        if (Heap::contains(ptr)) {
            // if we are in system malloc phase, we should not be deallocating rckid memory, as this only happens during shutdown
            if (!systemMalloc_)
                Heap::free(ptr);
        } else if (Arena::contains(ptr)) {
            // don't delete if it comes from arena
        } else {
            // if the pointer is not on arena, it must come from the host system's heap and should be freed by libc's free mechanism
            __libc_free(ptr);
        }
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

    void joystickTick();
    void displayDraw();

    uint64_t uptimeUs64() {
        using namespace std::chrono;
        static auto first = steady_clock::now();
        return static_cast<uint64_t>(duration_cast<microseconds>(steady_clock::now() - first).count()); 
    }

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


        bool audioPlayback_;
        AudioStream audioStream_;
        DoubleBuffer * audioPlaybackBuffer_;
        uint32_t audioPlaybackBufferRemaining_;
        int16_t * audioPlaybackBufferRead_;
        uint8_t audioVolume_ = 10;

        std::fstream sdIso_;
        uint32_t sdNumBlocks_;

        std::fstream flashIso_;
        uint32_t flashSize_ = 0;

        uint64_t nextSecond_;
        TinyDate dateTime_;
        TinyAlarm alarm_;
    }

    void initialize() {
        systemMalloc_ = true;
        InitWindow(640, 480, "RCKid");
        // see if there is sd.iso file so that we can simulate SD card
        sdIso_.open("sd.iso", std::ios::in | std::ios::out | std::ios::binary);
        if (sdIso_.is_open()) {
            sdIso_.seekg(0, std::ios::end);
            size_t sizeBytes = sdIso_.tellg();
            LOG("sd.iso file found, mounting SD card - " << sizeBytes << " bytes");
            if (sizeBytes % 512 == 0 && sizeBytes != 0) {
                sdNumBlocks_ = sizeBytes / 512;
                LOG("    blocks: " << sdNumBlocks_);
            } else {
                LOG("    invalid file size (multiples of 512 bytes allowed)");
            }
        } else {
            LOG("sd.iso file not found, CD card not present");
        }
        // see if there is flash.iso file so that we can simulate flash storage
        flashIso_.open("cartridge.iso", std::ios::in | std::ios::out | std::ios::binary);
        if (flashIso_.is_open()) {
            flashIso_.seekg(0, std::ios::end);
            size_t sizeBytes = flashIso_.tellg();
            LOG("flash.iso file found, mounting cartridge store - " << sizeBytes << " bytes");
            if (sizeBytes % 4096 == 0 && sizeBytes != 0) {
                flashSize_ = sizeBytes;
                LOG("    size: " << flashSize_);
            } else {
                LOG("    invalid file size (multiples of 4096 bytes allowed)");
            }
        } else {
            LOG("flash.iso file not found, cartridge storage not present");
        }
        systemMalloc_ = false;
        displayImg_ = GenImageColor(320, 240, BLACK);
        displayTexture_ = LoadTextureFromImage(displayImg_);
        displayLastVSyncTime_ = std::chrono::steady_clock::now();
        filesystem::initialize();

        // enter base arena for the application
        // TODO is this needed? 
        Arena::enter();
        // initialize the next second and time & date from systems time and data
        nextSecond_ = uptimeUs64() + 1000000;
#if (defined _MSC_VER)        
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_time);
        dateTime_.set(
            now_tm->tm_mday, 
            now_tm->tm_mon, 
            now_tm->tm_year,
            now_tm->tm_hour, 
            now_tm->tm_min, 
            now_tm->tm_sec
        );
#endif
    }

    void tick() {
        joystickTick();
        systemMalloc_ = true;
        uint64_t now = uptimeUs64();
        while (now > nextSecond_) {
            nextSecond_ += 1000000;
            dateTime_.secondTick();
        }
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
        bsod(error, line, file, nullptr);
        systemMalloc_ = true;
        if (sdIso_.good())
            sdIso_.close();
        if (flashIso_.good())
            flashIso_.close();
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

    TinyDate dateTime() {
        return dateTime_;
    }

    void setDateTime(TinyDate value) {
        dateTime_ = value;
    }

    TinyAlarm alarm() {
        return alarm_;
    }

    void setAlarm(TinyAlarm value) {
        alarm_ = value;
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

    uint16_t lightAmbient() { return 1; }
    uint16_t lightUV() { return 2; }

    int16_t tempAvr() { return 250; }



    // power management

    void sleep() {

    }

    bool charging() { 
        return true; 
    }

    bool dcPower() {
        return true;
    }

    unsigned vBatt() {
        return 370;
    }

    unsigned batteryLevel() {
        return 67;
    }

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
            displayCallback_ = nullptr;
            displayDraw();
        }
    }

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels, DisplayUpdateCallback callback) {
        displayCallback_ = callback;
        displayUpdate(pixels, numPixels);
    }

    // audio

    void audioStreamRefill(void * buffer, unsigned int frames) {
        int16_t * stereo = reinterpret_cast<int16_t*>(buffer);
        while (frames-- != 0) {
            if (audioPlaybackBufferRemaining_ == 0) {
                audioPlaybackBuffer_->swap();
                audioPlaybackBufferRemaining_ = audioPlaybackBuffer_->size() / 4; // stereo uint16_t 
                audioPlaybackBufferRead_ = reinterpret_cast<int16_t*>(audioPlaybackBuffer_->getFrontBuffer());
            }
            if (audioVolume_ == 0) {
                *(stereo++) = 0;
                *(stereo++) = 0;
            } else {
                int16_t x = *(audioPlaybackBufferRead_++);
                x >>= (10 - audioVolume_);
                x = x & 0xfff0;
                *(stereo++) = x; //*(audioPlaybackBufferRead_++);

                x = *(audioPlaybackBufferRead_++);
                x >>= (10 - audioVolume_);
                x = x & 0xfff0;
                *(stereo++) = x; //*(audioPlaybackBufferRead_++);
            }

//            *(stereo++) = *(audioPlaybackBufferRead_++);
            --audioPlaybackBufferRemaining_;
        }
    }

    void audioOn() {
        InitAudioDevice();
    }

    void audioOff() {
        if (audioPlayback_)
            audioStop();
        CloseAudioDevice();
    }

    bool audioEnabled() {
        // TODO and recording
        return audioPlayback_; 
    }

    bool audioHeadphones() {
        // TODO detection ? 
        return true;
    }

    int32_t audioVolume() {
        return audioVolume_;
    }

    void audioSetVolume(int32_t value) {
        if (value > 10)
            value = 10;
        if (value < 0)
            value = 0;
        audioVolume_ = value; 
    }

    uint32_t audioSampleRate() {
        if (audioPlayback_)
            return audioStream_.sampleRate;
        else
            return 44100;
    }

    void audioPlay(DoubleBuffer & data, uint32_t sampleRate) {
        if (audioPlayback_)
            audioStop();
        audioStream_ = LoadAudioStream(sampleRate, 16, 2);
        SetAudioStreamCallback(audioStream_, audioStreamRefill);   
        audioPlaybackBuffer_ = & data;   
        audioPlaybackBufferRemaining_ = 0;  
        PlayAudioStream(audioStream_);
        audioPlayback_ = true;
    }

    void audioRecord(DoubleBuffer & data, uint32_t sampleRate) {
        UNIMPLEMENTED;
    }

    void audioPause() {
        if (audioPlayback_)
            PauseAudioStream(audioStream_);
    }

    void audioStop() {
        if (audioPlayback_) {
            StopAudioStream(audioStream_);
            UnloadAudioStream(audioStream_);
            audioPlayback_ = false;
        }
    }

    // LEDs

    void ledsOff() {
        LOG("LED: all off");
//        UNIMPLEMENTED;
    }

    void ledSetEffect([[maybe_unused]]Btn b, [[maybe_unused]] RGBEffect const & effect) {
//        UNIMPLEMENTED;
    }

    void ledSetEffects([[maybe_unused]] RGBEffect const & dpad, [[maybe_unused]] RGBEffect const & a, [[maybe_unused]] RGBEffect const & b, [[maybe_unused]] RGBEffect const & select, [[maybe_unused]] RGBEffect const & start){
//        UNIMPLEMENTED;
    }

    // Rumbler

    void rumble(RumblerEffect const & effect) {
        // there is no rumbler, just log the action
        LOG("Rumbler: intensity " << (unsigned) effect.strength << ", duration: " << effect.timeOn << ", repetitions: " << effect.cycles << ", off duration: " << effect.timeOff);
    }

    // SD card interface

    uint32_t sdCapacity() {
        return sdNumBlocks_;
    }

    bool sdReadBlocks(uint32_t start, uint8_t * buffer, uint32_t numBlocks) {
        ASSERT(sdNumBlocks_ != 0);
        try {
            systemMalloc_ = true;
            sdIso_.seekg(start * 512);
            sdIso_.read(reinterpret_cast<char*>(buffer), numBlocks * 512);
            systemMalloc_ = false;
            return true;
        } catch (std::exception const & e) {
            LOG("SD card read error: " << e.what());
            return false;
        }
    }

    bool sdWriteBlocks(uint32_t start, uint8_t const * buffer, uint32_t numBlocks) {
        ASSERT(sdNumBlocks_ != 0);
        try {
            systemMalloc_ = true;
            sdIso_.seekp(start * 512);
            sdIso_.write(reinterpret_cast<char const *>(buffer), numBlocks * 512);
            systemMalloc_ = false;
            return true;
        } catch (std::exception const & e) {
            LOG("SD card write error: " << e.what());
            return false;
        }
    }

    // flash

    uint32_t cartridgeCapacity() { return flashSize_; }

    uint32_t cartridgeWriteSize() { return 256; }

    uint32_t cartridgeEraseSize() { return 4096; }

    void cartridgeRead(uint32_t start, uint8_t * buffer, uint32_t numBytes) {
        ASSERT(start + numBytes <= flashSize_);
        try {
            systemMalloc_ = true;
            flashIso_.seekg(start);
            flashIso_.read(reinterpret_cast<char*>(buffer), numBytes);
            systemMalloc_ = false;
        } catch (std::exception const & e) {
            LOG("Cartridge flash read error: " << e.what());
            UNREACHABLE;
        }
    }

    void cartridgeWrite(uint32_t start, uint8_t const * buffer) {
        ASSERT(start % 256 == 0);
        ASSERT(start + 256 <= flashSize_);
        try {
            flashIso_.seekp(start);
            systemMalloc_ = true;
            flashIso_.write(reinterpret_cast<char const *>(buffer), 256);
            systemMalloc_ = false;
        } catch (std::exception const & e) {
            LOG("Cartridge flash write error: " << e.what());
            UNREACHABLE;
        }
    }

    void cartridgeErase(uint32_t start) {
        ASSERT(start % 4096 == 0);
        ASSERT(start + 4096 <= flashSize_);
        try {
            flashIso_.seekp(start);
            uint8_t buffer[4096];
            std::memset(buffer, 4096, 0xff);
            systemMalloc_ = true;
            flashIso_.write(reinterpret_cast<char const *>(buffer), 4096);
            systemMalloc_ = false;
        } catch (std::exception const & e) {
            LOG("Cartridge flash erase error: " << e.what());
            UNREACHABLE;
        }
    }

    // accelerated functions

    #include "rckid/accelerated.inc.h"
    MEM_FILL_8
    MEM_FILL_16
    MEM_FILL_32

}
