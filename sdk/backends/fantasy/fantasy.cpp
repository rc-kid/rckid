#ifndef RCKID_BACKEND_FANTASY
#error "You are building fantasy (RayLib) backend without the indicator macro"
#endif

#include <raylib.h>
#include <string>
#include <fstream>
#include <chrono>

#include <platform/string_utils.h>
#include <platform/args.h>

#include "rckid/rckid.h"

namespace rckid {
    extern thread_local char ** stackStart_;
}

#ifndef _WIN32
extern "C" {
    extern void *__libc_malloc(size_t);
    extern void __libc_free(void *);

    //depending on whether we are in system malloc, or not use libc malloc, or RCKid's heap
    void * malloc(size_t numBytes) {
        if (rckid::SystemMallocGuard::isDefault())
            return __libc_malloc(numBytes);
        else if (rckid::Heap::isDefaultTarget())
            return rckid::Heap::allocBytes(numBytes);
        else 
            return rckid::Arena::allocBytes(numBytes);
    }

    // if the pointer to be freed belongs to RCKId's heap, we should use own heap free, otherwise use normal free (and assert it does not belong to fantasy heap in general as that would be weird)
    void free(void * ptr) {
        if (rckid::Heap::contains(ptr)) {
            rckid::Heap::free(ptr);
        } else if (rckid::Arena::contains(ptr)) {
            // don't do anything if the pointer comes from arena
            return;
        // otherwise this is a libc pointer and should be deleted accordingly
        } else {
            __libc_free(ptr);
        }
   }
} // extern C
#endif

namespace rckid {

    void memoryReset();

    bool initialized_ = false;

    // forward declaration of the bsod function
    NORETURN(void bsod(uint32_t error, uint32_t arg, uint32_t line = 0, char const * file = nullptr));

    namespace sd {
        std::fstream iso_;
        uint32_t numBlocks_ = 0;
    }

    namespace flash {
        std::fstream iso_;
        uint32_t size_ = 0;
    }

    namespace io {
        uint32_t buttons_;
        uint32_t lastButtons_;
        int16_t accelX_;
        int16_t accelY_;
        int16_t accelZ_;
        int16_t gyroX_;
        int16_t gyroY_;
        int16_t gyroZ_;
    }

    namespace display {
        DisplayRefreshDirection direction;
        Rect rect = Rect::WH(RCKID_DISPLAY_WIDTH, RCKID_DISPLAY_HEIGHT);
        Image img;
        Texture texture;
        uint8_t brightness;
        DisplayUpdateCallback callback;

        int updateX = 0;
        int updateY = 0;
        size_t updating = 0;
        std::chrono::steady_clock::time_point lastVSyncTime;   
        size_t pixelsSent = 0;
        size_t frameSize =  320 * 240;     

        std::chrono::steady_clock::time_point fpsStart;
        unsigned fps;
        unsigned currentFps;
    }

    namespace audio {
        enum class Mode {
            Off,
            Play,
            Record
        };
        Mode mode;
        AudioCallback cb;
        DoubleBuffer<int16_t> * buffer = nullptr;
        size_t bufferSize = 0;
        size_t bufferI = 0;
        bool playback = false;
        uint8_t volume = 10;
        AudioStream stream;
    }

    // forward declarations of internal functions
    namespace filesystem {
        void initialize();
        void initialize(std::string const & sdRoot, std::string const & flashRoot);
    }

    void fatalError(uint32_t error, uint32_t arg, uint32_t line, char const * file) {
        // disable stack protection (in case we bailed out because of the stack)
        stackStart_ = nullptr;
        // close the SD and flash files
        {
            SystemMallocGuard g_;
            if (sd::iso_.good())
                sd::iso_.close();
            if (flash::iso_.good())
                flash::iso_.close();
        }
        // reset the memory so that we have enough space
        memoryReset();
        // finally, go for BSOD
        bsod(error, arg, line, file);
    }

    Writer debugWrite() {
        return Writer([](char c) {
            SystemMallocGuard g;
            std::cout << c << std::flush;
        });
    }

    uint8_t debugRead(bool echo) {
        while (true) {
            SystemMallocGuard::enable();
            PollInputEvents();
            int x = GetCharPressed();
            SystemMallocGuard::disable();
            if (x != 0) {
                if (echo)
                    std::cout << static_cast<char>(x) << std::flush;
                return static_cast<uint8_t>(x);
            }
        }
    }

    void initialize(int argc, char const * argv[]) {
        SystemMallocGuard::enable();
        memoryInstrumentStackProtection();
        InitWindow(640, 480, "RCKid");
        display::img = GenImageColor(RCKID_DISPLAY_WIDTH, RCKID_DISPLAY_HEIGHT, BLACK);
        display::texture = LoadTextureFromImage(display::img);
        display::lastVSyncTime = std::chrono::steady_clock::now();
        display::fpsStart = std::chrono::steady_clock::now();
#ifdef RCKID_ENABLE_HOST_FILESYSTEM
        Args::Arg<std::string> sdIso{"sd", "sd"};
        Args::Arg<std::string> flashIso{"flash", "flash"};
#else
        Args::Arg<std::string> sdIso{"sd", "sd.iso"};
        Args::Arg<std::string> flashIso{"flash", "flash.iso"};
#endif
        Args::parse(argc, argv, { sdIso, flashIso });

#ifdef RCKID_ENABLE_HOST_FILESYSTEM
        filesystem::initialize(sdIso.value(), flashIso.value());
#else
        // see if there is sd.iso file so that we can simulate SD card
        sd::iso_.open(sdIso.value(), std::ios::in | std::ios::out | std::ios::binary);
        if (sd::iso_.is_open()) {
            sd::iso_.seekg(0, std::ios::end);
            size_t sizeBytes = sd::iso_.tellg();
            LOG(LL_INFO, sdIso.value() << " file found, mounting SD card - " << sizeBytes << " bytes");
            if (sizeBytes % 512 == 0 && sizeBytes != 0) {
                sd::numBlocks_ = static_cast<uint32_t>(sizeBytes / 512);
                LOG(LL_INFO, "    blocks: " << sd::numBlocks_);
            } else {
                LOG(LL_INFO, "    invalid file size (multiples of 512 bytes allowed)");
            }
        } else {
            LOG(LL_ERROR, sdIso.value() << " file not found, CD card not present");
        }
        // see if there is flash.iso file so that we can simulate flash storage
        flash::iso_.open(flashIso.value(), std::ios::in | std::ios::out | std::ios::binary);
        if (flash::iso_.is_open()) {
            flash::iso_.seekg(0, std::ios::end);
            size_t sizeBytes = flash::iso_.tellg();
            LOG(LL_INFO, flashIso.value() << " file found, mounting cartridge store - " << sizeBytes << " bytes");
            if (sizeBytes % 4096 == 0 && sizeBytes != 0) {
                flash::size_ = static_cast<uint32_t>(sizeBytes);
                LOG(LL_INFO, "    size: " << flash::size_);
            } else {
                LOG(LL_INFO, "    invalid file size (multiples of 4096 bytes allowed)");
            }
        } else {
            LOG(LL_ERROR, flashIso.value() << " file not found, cartridge storage not present");
        }
#endif

        // initialize the audio device
        InitAudioDevice();

        SystemMallocGuard::disable();

        filesystem::initialize();
        // mark that we are initialized and the graphics & sound should be used
        initialized_ = true;
    }

    void initializeNoWindow([[maybe_unused]] int argc, [[maybe_unused]] char * argv[]) {
        SystemMallocGuard::disable();
    }

    void tick() {
        SystemMallocGuard g_;
        if (WindowShouldClose())
            std::exit(-1);

        io::lastButtons_ = io::buttons_;
        io::buttons_ = 0;
        PollInputEvents();
        if (IsKeyDown(KEY_UP))
            io::buttons_ |= static_cast<uint32_t>(Btn::Up);
        if (IsKeyDown(KEY_DOWN))
            io::buttons_ |= static_cast<uint32_t>(Btn::Down);
        if (IsKeyDown(KEY_LEFT))
            io::buttons_ |= static_cast<uint32_t>(Btn::Left);
        if (IsKeyDown(KEY_RIGHT))
            io::buttons_ |= static_cast<uint32_t>(Btn::Right);
        if (IsKeyDown(KEY_A))
            io::buttons_ |= static_cast<uint32_t>(Btn::A);
        if (IsKeyDown(KEY_B))
            io::buttons_ |= static_cast<uint32_t>(Btn::B);
        if (IsKeyDown(KEY_SPACE))
            io::buttons_ |= static_cast<uint32_t>(Btn::Select);
        if (IsKeyDown(KEY_ENTER))
            io::buttons_ |= static_cast<uint32_t>(Btn::Start);
        if (IsKeyDown(KEY_PAGE_UP))
            io::buttons_ |= static_cast<uint32_t>(Btn::VolumeUp);
        if (IsKeyDown(KEY_PAGE_DOWN))
            io::buttons_ |= static_cast<uint32_t>(Btn::VolumeDown);
        if (IsKeyDown(KEY_H))
            io::buttons_ |= static_cast<uint32_t>(Btn::Home);
    }

    void yield() {
        memoryCheckStackProtection();   
        // nothing yet to be be done here in fantasy mode
    }

    uint32_t uptimeUs() {
        memoryCheckStackProtection();
        using namespace std::chrono;
        static auto first = steady_clock::now();
        return static_cast<uint32_t>(duration_cast<microseconds>(steady_clock::now() - first).count()); 
    }

    // io

    bool btnDown(Btn b) {
        memoryCheckStackProtection();
        return io::buttons_ & static_cast<uint32_t>(b);
    }    

    bool btnPressed(Btn b) {
        memoryCheckStackProtection();
        return btnDown(b) && ! (io::lastButtons_ & static_cast<uint32_t>(b));
    }    

    bool btnReleased(Btn b) {
        memoryCheckStackProtection();
        return btnDown(b) && ! (io::lastButtons_ & static_cast<uint32_t>(b));
    }
    
    void btnClear(Btn b) {
        memoryCheckStackProtection();
        // simply ensure last state is identical to current state
        io::lastButtons_ &= ~static_cast<uint32_t>(b);
        io::lastButtons_ |= (io::buttons_ & static_cast<uint32_t>(b));
    }

    int16_t accelX() { 
        memoryCheckStackProtection();
        return io::accelX_; 
    }

    int16_t accelY() { 
        memoryCheckStackProtection();
        return io::accelY_; 
    }

    int16_t accelZ() { 
        memoryCheckStackProtection();
        return io::accelZ_; 
    }

    int16_t gyroX() { 
        memoryCheckStackProtection();
        return io::gyroX_; 
    }

    int16_t gyroY() { 
        memoryCheckStackProtection();
        return io::gyroY_; 
    }

    int16_t gyroZ() { 
        memoryCheckStackProtection();
        return io::gyroZ_; 
    }

    // display 

    void displayDraw() {
        if (! initialized_)
            return;
        SystemMallocGuard g;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - display::fpsStart).count();
        if (elapsed > 1000000) {
            display::fps = display::currentFps;
            display::currentFps = 0;
            display::fpsStart = now;
            LOG(LL_DISPLAY_FPS, "FPS: " << display::fps);
        }
        UpdateTexture(display::texture, display::img.data);
        BeginDrawing();
        DrawTextureEx(display::texture, {0, 0}, 0, 2.0f, WHITE);
    
        DrawText(TextFormat("FPS: %d", display::fps), 540, 240, 20, RED);
        DrawText(TextFormat("MEM: %d", memoryFree() / 1024), 540, 260, 20, RED);

        EndDrawing();
        SwapScreenBuffer();
        ++display::currentFps;
    }

    void displayResetDrawRectangle() {
        switch (display::direction) {
            case DisplayRefreshDirection::ColumnFirst:
                display::updateX = display::rect.right() - 1;
                display::updateY = display::rect.top();
                break;
            case DisplayRefreshDirection::RowFirst:
                display::updateX = display::rect.left();
                display::updateY = display::rect.top();
                break;
            default:
                UNREACHABLE;
        }
        display::pixelsSent = 0;
        display::frameSize = display::rect.width() * display::rect.height();
    }

    void displayOn() {
        memoryCheckStackProtection();
        // no need to do anything in fantasy mode
    }

    void displayOff() {
        memoryCheckStackProtection();
        // no need to do anything in fantasy mode
    }

    DisplayRefreshDirection displayRefreshDirection() {
        memoryCheckStackProtection();
        return display::direction;
    }

    void displaySetRefreshDirection(DisplayRefreshDirection value) {
        memoryCheckStackProtection();
        display::direction = value;
        displayResetDrawRectangle();
    }

    uint8_t displayBrightness() { 
        memoryCheckStackProtection();
        return display::brightness;
    }

    void displaySetBrightness(uint8_t value) {
        memoryCheckStackProtection();
        display::brightness = value;
    }

    Rect displayUpdateRegion() {
        memoryCheckStackProtection();
        return display::rect;
    }

    void displaySetUpdateRegion(Rect value) {
        memoryCheckStackProtection();
        display::rect = value;
        displayResetDrawRectangle();
    }

    void displaySetUpdateRegion(Coord width, Coord height) {
        memoryCheckStackProtection();
        displaySetUpdateRegion(Rect::XYWH((RCKID_DISPLAY_WIDTH - width) / 2, (RCKID_DISPLAY_HEIGHT - height) / 2, width, height));
    }

    bool displayUpdateActive() {
        memoryCheckStackProtection();
        return display::updating;
    }

    void displayWaitUpdateDone() {
        memoryCheckStackProtection();
        while (displayUpdateActive())
            yield();
    }

    void displayWaitVSync() {
        yield();
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - display::lastVSyncTime).count();
        // hardwired for ~60 fps...  
        if (elapsed < 16666)
            std::this_thread::sleep_for(std::chrono::microseconds(16666 - elapsed));
        display::lastVSyncTime = std::chrono::steady_clock::now();

    }

    void displayUpdate(uint16_t const * pixels, uint32_t numPixels, DisplayUpdateCallback callback) {
        display::callback = callback;
        displayUpdate(pixels, numPixels);
    }

    void displayUpdate(uint16_t const * pixels, uint32_t numPixels) {
        memoryCheckStackProtection();
        ++display::updating;
        display::pixelsSent += numPixels;
        // update the pixels in the internal framebuffer
        while (numPixels != 0) {
            ColorRGB c = ColorRGB::fromRaw16(*pixels++);
            ImageDrawPixel(&display::img, display::updateX, display::updateY, { c.r(), c.g(), c.b(), 255});
            --numPixels;
            switch (display::direction) {
                case DisplayRefreshDirection::ColumnFirst:
                    if (++display::updateY >= display::rect.bottom()) {
                        display::updateY = display::rect.top();
                        if (--display::updateX < display::rect.left())
                            display::updateX = display::rect.right() - 1; 
                    }
                    break;
                case DisplayRefreshDirection::RowFirst:
                    if (++display::updateX >= display::rect.right()) {
                        display::updateX = display::rect.left();
                        if (++display::updateY >= display::rect.bottom())
                            display::updateY = display::rect.top(); 
                    }
                    break;
                default:
                    UNREACHABLE;
            }
        }
        // check if this is the first update call, in which case call all the other updates (as long as the callback generates a new update) and when no more updates are scheduled, actually redraw the display. Note that if the update is not the first, no callbacks are called
        if (display::updating == 1) {
            while (true) {
                size_t updatingOld = display::updating;
                if (display::callback)
                    display::callback();
                if (updatingOld == display::updating)
                    break;
            }
            display::updating = 0;
            display::callback = nullptr;
            if (display::pixelsSent >= display::frameSize) {
                display::pixelsSent -= display::frameSize;
                displayDraw();
            }
        }
    }

    // audio

    /** Refills the raylib's audio stream. 
     */
    void audioStreamRefill(void * buffer, unsigned int samples) {
        memoryCheckStackProtection();
        // get the stereo view of the input buffer
        int16_t * stereo = reinterpret_cast<int16_t*>(buffer);
        while (samples-- != 0) {
            if (audio::bufferI >= audio::bufferSize) {
                audio::bufferSize = audio::cb(audio::buffer->front(), static_cast<uint32_t>(audio::buffer->size()) / 2) * 2;
                audio::bufferI = 0;
                audio::buffer->swap();
            }
            int16_t l = audio::buffer->back()[audio::bufferI++];
            int16_t r = audio::buffer->back()[audio::bufferI++];
            if (audio::volume == 0) {
                l = 0;
                r = 0;
            } else {
                l >>= (10 - audio::volume);
                l = l & 0xfff0;
                r >>= (10 - audio::volume);
                r = r & 0xfff0;
            }
            *(stereo++) = l;
            *(stereo++) = r;
        }
    }

    bool audioHeadphones() {
        memoryCheckStackProtection();
        // TODO make this conditional on something
        return false;
    }

    bool audioPaused() {
        memoryCheckStackProtection();
        switch (audio::mode) {
            case audio::Mode::Play:
                return !IsAudioStreamPlaying(audio::stream);
                break;
            case audio::Mode::Record:
                UNIMPLEMENTED;
            case audio::Mode::Off:
                break;
        }
        return false;
    }

    bool audioPlayback() {
        memoryCheckStackProtection();
        return audio::mode == audio::Mode::Play;
    }

    bool audioRecording() {
        memoryCheckStackProtection();
        return audio::mode == audio::Mode::Record;
    }

    uint8_t audioVolume() {
        memoryCheckStackProtection();
        return audio::volume;
    }

    void audioSetVolume(uint8_t value) {
        memoryCheckStackProtection();
        audio::volume = value;
    }

    void audioPlay(DoubleBuffer<int16_t> & buffer, uint32_t sampleRate, AudioCallback cb) {
        memoryCheckStackProtection();
        if (audio::mode != audio::Mode::Off)
            audioStop();
        {
            SystemMallocGuard g;
            audio::cb = cb;
            audio::buffer = & buffer;
            audio::stream = LoadAudioStream(sampleRate, 16, 2);
            SetAudioStreamCallback(audio::stream, audioStreamRefill);   
            audio::mode = audio::Mode::Play;
            PlayAudioStream(audio::stream);
        }
    }   

    void audioPause() {
        memoryCheckStackProtection();
        if (!audioPaused()) {
            switch (audio::mode) {
                case audio::Mode::Play: {
                    SystemMallocGuard g;
                    PauseAudioStream(audio::stream);
                    break;
                }
                case audio::Mode::Record:
                    UNIMPLEMENTED;
                case audio::Mode::Off:
                    break;
            }
        }
    }

    void audioResume() {
        memoryCheckStackProtection();
        if (audioPaused()) {
            switch (audio::mode) {
                case audio::Mode::Play: {
                    SystemMallocGuard g;
                    ResumeAudioStream(audio::stream);
                    break;
                }
                case audio::Mode::Record:
                    UNIMPLEMENTED;
                case audio::Mode::Off:
                    break;
            }
        }
    }

    void audioStop() {
        memoryCheckStackProtection();
        switch (audio::mode) {
            case audio::Mode::Play: {
                SystemMallocGuard g;
                StopAudioStream(audio::stream);
                UnloadAudioStream(audio::stream);
                audio::buffer = nullptr;
                //audio::bufferSize_ = 0;
                //audio::bufferI_ = 0;
                break;
            }
            case audio::Mode::Record:
                UNIMPLEMENTED;
            case audio::Mode::Off:
                break;
        }
        audio::mode = audio::Mode::Off;
    }

    // SD Card access

    uint32_t sdCapacity() {
        memoryCheckStackProtection();
        return sd::numBlocks_;
    }

    bool sdReadBlocks(uint32_t start, uint8_t * buffer, uint32_t numBlocks) {
        memoryCheckStackProtection();
        ASSERT(sd::numBlocks_ != 0);
        try {
            SystemMallocGuard g;
            sd::iso_.seekg(start * 512);
            sd::iso_.read(reinterpret_cast<char*>(buffer), numBlocks * 512);
            return true;
        } catch (std::exception const & e) {
            LOG(LL_ERROR, "SD card read error: " << e.what());
            return false;
        }
    }

    bool sdWriteBlocks(uint32_t start, uint8_t const * buffer, uint32_t numBlocks) {
        memoryCheckStackProtection();
        ASSERT(sd::numBlocks_ != 0);
        try {
            SystemMallocGuard g;
            sd::iso_.seekp(start * 512);
            sd::iso_.write(reinterpret_cast<char const *>(buffer), numBlocks * 512);
            return true;
        } catch (std::exception const & e) {
            LOG(LL_ERROR, "SD card write error: " << e.what());
            return false;
        }
    }

    // Cartridge filesystem access

    uint32_t cartridgeCapacity() { 
        memoryCheckStackProtection();
        return flash::size_;
    }

    uint32_t cartridgeWriteSize() { 
        memoryCheckStackProtection();
        return 256; 
    }

    uint32_t cartridgeEraseSize() { 
        memoryCheckStackProtection();
        return 4096; 
    }

    void cartridgeRead(uint32_t start, uint8_t * buffer, uint32_t numBytes) {
        memoryCheckStackProtection();
        ASSERT(start + numBytes <= flash::size_);
        try {
            SystemMallocGuard g;
            flash::iso_.seekg(start);
            flash::iso_.read(reinterpret_cast<char*>(buffer), numBytes);
        } catch (std::exception const & e) {
            LOG(LL_ERROR, "Cartridge flash read error: " << e.what());
            UNREACHABLE;
        }
    }

    void cartridgeWrite(uint32_t start, uint8_t const * buffer) {
        memoryCheckStackProtection();
        ASSERT(start % 256 == 0);
        ASSERT(start + 256 <= flash::size_);
        try {
            SystemMallocGuard g;
            flash::iso_.seekp(start);
            flash::iso_.write(reinterpret_cast<char const *>(buffer), 256);
        } catch (std::exception const & e) {
            LOG(LL_ERROR, "Cartridge flash write error: " << e.what());
            UNREACHABLE;
        }
    }

    void cartridgeErase(uint32_t start) {
        memoryCheckStackProtection();
        ASSERT(start % 4096 == 0);
        ASSERT(start + 4096 <= flash::size_);
        try {
            SystemMallocGuard g;
            flash::iso_.seekp(start);
            uint8_t buffer[4096];
            std::memset(buffer, 4096, 0xff);
            flash::iso_.write(reinterpret_cast<char const *>(buffer), 4096);
        } catch (std::exception const & e) {
            LOG(LL_ERROR, "Cartridge flash erase error: " << e.what());
            UNREACHABLE;
        }
    }

    // rumbler
    void rumblerEffect(RumblerEffect const & effect) {
        memoryCheckStackProtection();
        if (effect.strength == 0)
            LOG(LL_INFO, "rumbler: off");
        else  
            LOG(LL_INFO, "rumbler: " << effect);
    }

    // rgb

    void rgbEffect(uint8_t rgb, RGBEffect const & effect) {
        memoryCheckStackProtection();
        LOG(LL_INFO, "rgb: " << rgb << ", effect: " << effect);
    }
    
    void rgbEffects(RGBEffect const & a, RGBEffect const & b, RGBEffect const & dpad, RGBEffect const & sel, RGBEffect const & start) {
        memoryCheckStackProtection();
        LOG(LL_INFO, "rgb: a,     effect: " << a);
        LOG(LL_INFO, "rgb: b,     effect: " << b);
        LOG(LL_INFO, "rgb: dpad,  effect: " << dpad);
        LOG(LL_INFO, "rgb: sel,   effect: " << sel);
        LOG(LL_INFO, "rgb: start, effect: " << start);
    }
    
    void rgbOff() {
        memoryCheckStackProtection();
        LOG(LL_INFO, "rgb: off");
    }


    bool memoryIsImmutable([[maybe_unused]] void const * ptr) {
        memoryCheckStackProtection();
        return false;
    }

}

extern "C" {

    void memset8(uint8_t * buffer, uint8_t value, uint32_t size) {
        while (size-- != 0)
            *(buffer++) = value;
    }

    void memset16(uint16_t * buffer, uint16_t value, uint32_t size) {
        while (size-- != 0)
            *(buffer++) = value;
    }

    void memset32(uint32_t * buffer, uint32_t value, uint32_t size) {
        while (size-- != 0)
            *(buffer++) = value;
    }

}