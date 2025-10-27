#ifndef RCKID_BACKEND_FANTASY
#error "You are building fantasy (RayLib) backend without the indicator macro"
#endif

#include <raylib.h>
#include <string>
#include <fstream>
#include <chrono>

#include <platform/string_utils.h>
#include <platform/args.h>

#include <rckid/rckid.h>
#include <rckid/app.h>
#include <rckid/filesystem.h>
#include <rckid/ui/header.h>
#include <rckid/radio.h>
#include <rckid/ui/style.h>

#ifndef _WIN32
extern "C" {
    extern void *__libc_malloc(size_t);
    extern void __libc_free(void *);

    //depending on whether we are in system malloc, or not use libc malloc, or RCKid's heap
    void * malloc(size_t numBytes) {
        if (rckid::SystemMallocGuard::isDefault())
            return __libc_malloc(numBytes);
        else 
            return rckid::RAMHeap::alloc(numBytes);
    }

    // if the pointer to be freed belongs to RCKId's heap, we should use own heap free, otherwise use normal free (and assert it does not belong to fantasy heap in general as that would be weird)
    void free(void * ptr) {
        if (rckid::RAMHeap::contains(ptr)) {
            rckid::RAMHeap::free(ptr);
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

    namespace time {
        TinyDateTime time_;
        TinyAlarm alarm_;
        std::chrono::steady_clock::time_point uptimeStart_;
        uint64_t nextSecond_ = 0;
    }; 

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
        int16_t * bufferCurrent = nullptr;
        uint32_t bufferSize = 0;
        uint32_t bufferI = 0;

        int16_t * bufferNext = nullptr;
        uint32_t bufferNextSize = 0;

        bool playback = false;
        uint8_t volume = 10;
        AudioStream stream;
    }

    // forward declarations of internal functions
    namespace fs {
        void initialize();
        void initialize(std::string const & sdRoot, std::string const & flashRoot);
    }

    namespace budgetInfo {
        uint32_t budget_ = 120;
        uint32_t budgetDaily_ = 3600;
    }

    void Error::setFatal(Error err) {
        // disable stack protection (in case we bailed out because of the stack)
        // stackStart_ = nullptr;
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
        set(err);
        // finally, go for BSOD
        bsod();
    }

    Writer debugWrite() {
        return Writer([](char c, void *) {
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
        StackProtection::currentSize();
        SystemMallocGuard::enable();
        InitWindow(640, 480, "RCKid");
        display::img = GenImageColor(RCKID_DISPLAY_WIDTH, RCKID_DISPLAY_HEIGHT, BLACK);
        display::texture = LoadTextureFromImage(display::img);
        display::lastVSyncTime = std::chrono::steady_clock::now();
        display::fpsStart = std::chrono::steady_clock::now();
#if RCKID_ENABLE_HOST_FILESYSTEM
        Args::Arg<std::string> sdIso{"sd", "sd"};
        Args::Arg<std::string> flashIso{"flash", "flash"};
#else
        Args::Arg<std::string> sdIso{"sd", "sd.iso"};
        Args::Arg<std::string> flashIso{"flash", "flash.iso"};
#endif
        Args::parse(argc, argv, { sdIso, flashIso });

#if RCKID_ENABLE_HOST_FILESYSTEM
        fs::initialize(sdIso.value(), flashIso.value());
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
        fs::initialize();
#endif

        // initialize the audio device
        InitAudioDevice();

        SystemMallocGuard::disable();

        Radio::initialize();

        // initialize the device time - on real device this is obtained from the always on avr rtc, here we get the system time
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_time);
        time::time_.time.set(
            now_tm->tm_hour, 
            now_tm->tm_min, 
            now_tm->tm_sec
        );
        time::time_.date.set(
            now_tm->tm_mday, 
            now_tm->tm_mon + 1, 
            now_tm->tm_year + 1900
        );
        // initialize the next second and uptime counters
        time::uptimeStart_ = std::chrono::steady_clock::now();
        time::nextSecond_ = uptimeUs64() + 1000000;

        // load the UI style
        ui::Style::load();
        // mark that we are initialized and the graphics & sound should be used
        initialized_ = true;
        LOG(LL_INFO, "Initialization done.");
        LOG(LL_INFO, "Heap used  " << RAMHeap::usedBytes());
        LOG(LL_INFO, "Stack size " << StackProtection::currentSize());
    }

    void initializeNoWindow([[maybe_unused]] int argc, [[maybe_unused]] char * argv[]) {
        StackProtection::currentSize();
        SystemMallocGuard::enable();
        std::cout << "RCKid initialization w/o window" << std::endl;
        SystemMallocGuard::disable();
    }

    void tick() {
        SystemMallocGuard g_;
        if (WindowShouldClose())
            std::exit(-1);
        // update timekeeping
        uint64_t now = uptimeUs64();
        while (now > time::nextSecond_) {
            time::nextSecond_ += 1000000;
            time::time_.inc();
            App::onSecondTick();
        }
        // and get button states
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

    void sleep() {
        // nothing to do in fantasy mode
    }

    void powerOff() {
        std::exit(0);
    }

    uint16_t powerVcc() { return 390; }

    bool powerUsbConnected() { return false; }

    bool powerCharging() { return false; }

    void yield() {
        StackProtection::check();   
        // nothing yet to be be done here in fantasy mode
    }

    void keepAlive() {
        StackProtection::check();
        // nothing to do here in fantasy mode, the device is always on
    }

    uint32_t uptimeUs() {
        StackProtection::check();
        using namespace std::chrono;
        return static_cast<uint32_t>(duration_cast<microseconds>(steady_clock::now() - time::uptimeStart_).count()); 
    }

    uint64_t uptimeUs64() {
        using namespace std::chrono;
        return static_cast<uint64_t>(duration_cast<microseconds>(steady_clock::now() - time::uptimeStart_).count()); 
    }

    TinyDateTime timeNow() { return time::time_; }

    void setTimeNow(TinyDateTime const & t) { time::time_ = t; }

    TinyAlarm timeAlarm() { return time::alarm_; }

    void setTimeAlarm(TinyAlarm alarm) { time::alarm_ = alarm; }

    // io

    bool btnDown(Btn b) {
        StackProtection::check();
        return io::buttons_ & static_cast<uint32_t>(b);
    }    

    bool btnPressed(Btn b) {
        StackProtection::check();
        return btnDown(b) && ! (io::lastButtons_ & static_cast<uint32_t>(b));
    }    

    bool btnReleased(Btn b) {
        StackProtection::check();
        return btnDown(b) && ! (io::lastButtons_ & static_cast<uint32_t>(b));
    }
    
    void btnClear(Btn b) {
        StackProtection::check();
        // simply ensure last state is identical to current state
        io::lastButtons_ &= ~static_cast<uint32_t>(b);
        io::lastButtons_ |= (io::buttons_ & static_cast<uint32_t>(b));
    }

    void btnClear() {
        StackProtection::check();
        io::lastButtons_ = 0xffffffff;
    }

    int16_t accelX() { 
        StackProtection::check();
        return io::accelX_; 
    }

    int16_t accelY() { 
        StackProtection::check();
        return io::accelY_; 
    }

    int16_t accelZ() { 
        StackProtection::check();
        return io::accelZ_; 
    }

    int16_t gyroX() { 
        StackProtection::check();
        return io::gyroX_; 
    }

    int16_t gyroY() { 
        StackProtection::check();
        return io::gyroY_; 
    }

    int16_t gyroZ() { 
        StackProtection::check();
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
        StackProtection::check();
        // no need to do anything in fantasy mode
    }

    void displayOff() {
        StackProtection::check();
        // no need to do anything in fantasy mode
    }

    void displayClear(ColorRGB color) {
        StackProtection::check();
        // fill the internal framebuffer with the given color
        ImageClearBackground(&display::img, { color.r(), color.g(), color.b(), 255 });
        // reset the draw rectangle
        displayResetDrawRectangle();
    }

    DisplayRefreshDirection displayRefreshDirection() {
        StackProtection::check();
        return display::direction;
    }

    void displaySetRefreshDirection(DisplayRefreshDirection value) {
        StackProtection::check();
        display::direction = value;
        displayResetDrawRectangle();
    }

    uint8_t displayBrightness() { 
        StackProtection::check();
        return display::brightness;
    }

    void displaySetBrightness(uint8_t value) {
        StackProtection::check();
        display::brightness = value;
    }

    Rect displayUpdateRegion() {
        StackProtection::check();
        return display::rect;
    }

    void displaySetUpdateRegion(Rect value) {
        StackProtection::check();
        display::rect = value;
        displayResetDrawRectangle();
    }

    void displaySetUpdateRegion(Coord width, Coord height) {
        StackProtection::check();
        displaySetUpdateRegion(Rect::XYWH((RCKID_DISPLAY_WIDTH - width) / 2, (RCKID_DISPLAY_HEIGHT - height) / 2, width, height));
    }

    bool displayUpdateActive() {
        StackProtection::check();
        return display::updating;
    }

    void displayWaitUpdateDone() {
        StackProtection::check();
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
        StackProtection::check();
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

    void audioStreamRefill(void * buffer, unsigned int samples) {
        StackProtection::check();
        // get the stereo view of the input buffer
        int16_t * stereo = reinterpret_cast<int16_t*>(buffer);
        while (samples-- != 0) {
            if (audio::bufferI >= audio::bufferSize * 2) {
                // go for the next buffer which should already be preloaded
                std::swap(audio::bufferCurrent, audio::bufferNext);
                std::swap(audio::bufferSize, audio::bufferNextSize);
                audio::bufferI = 0;
                // inform the callback that we have retired the current buffer and ask for replacement, which will be our next buffer
                audio::cb(audio::bufferNext, audio::bufferNextSize);
            }
            int16_t l = audio::bufferCurrent[audio::bufferI++];
            int16_t r = audio::bufferCurrent[audio::bufferI++];
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
        StackProtection::check();
        // TODO make this conditional on something
        return false;
    }

    bool audioPaused() {
        StackProtection::check();
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
        StackProtection::check();
        return audio::mode == audio::Mode::Play;
    }

    bool audioRecording() {
        StackProtection::check();
        return audio::mode == audio::Mode::Record;
    }

    uint8_t audioVolume() {
        StackProtection::check();
        return audio::volume;
    }

    void audioSetVolume(uint8_t value) {
        StackProtection::check();
        if (value > 15)
            value = 15;
        audio::volume = value;
    }

    void audioPlay(uint32_t sampleRate, AudioCallback cb) {
        StackProtection::check();
        if (audio::mode != audio::Mode::Off)
            audioStop();
        {
            SystemMallocGuard g;
            audio::cb = cb;
            audio::bufferCurrent = nullptr;
            audio::bufferNext = nullptr;
            audio::bufferSize = 0;
            audio::bufferI = 0;

            // preload the two buffers with the desired size
            audio::cb(audio::bufferCurrent, audio::bufferSize);
            audio::cb(audio::bufferNext, audio::bufferNextSize);

            audio::stream = LoadAudioStream(sampleRate, 16, 2);
            SetAudioStreamCallback(audio::stream, audioStreamRefill);   
            audio::mode = audio::Mode::Play;
            PlayAudioStream(audio::stream);
        }
    }

    void audioRecordMic([[maybe_unused]] uint32_t sampleRate, [[maybe_unused]] AudioCallback cb) {
        StackProtection::check();
        //UNIMPLEMENTED;
    }

    void audioRecordLineIn([[maybe_unused]] uint32_t sampleRate, [[maybe_unused]] AudioCallback cb) {
        StackProtection::check();
        //UNIMPLEMENTED;
    }

    void audioPause() {
        StackProtection::check();
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
        StackProtection::check();
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
        StackProtection::check();
        switch (audio::mode) {
            case audio::Mode::Play: {
                SystemMallocGuard g;
                StopAudioStream(audio::stream);
                UnloadAudioStream(audio::stream);
                audio::bufferCurrent = nullptr;
                audio::bufferNext = nullptr;
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
        StackProtection::check();
        return sd::numBlocks_;
    }

    bool sdReadBlocks(uint32_t start, uint8_t * buffer, uint32_t numBlocks) {
        StackProtection::check();
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
        StackProtection::check();
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
        StackProtection::check();
        return flash::size_;
    }

    uint32_t cartridgeWriteSize() { 
        StackProtection::check();
        return 256; 
    }

    uint32_t cartridgeEraseSize() { 
        StackProtection::check();
        return 4096; 
    }

    void cartridgeRead(uint32_t start, uint8_t * buffer, uint32_t numBytes) {
        StackProtection::check();
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
        StackProtection::check();
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
        StackProtection::check();
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
        StackProtection::check();
        if (effect.strength == 0)
            LOG(LL_INFO, "rumbler: off");
        else  
            LOG(LL_INFO, "rumbler: " << effect);
    }

    // rgb

    void rgbEffect(uint8_t rgb, RGBEffect const & effect) {
        StackProtection::check();
        LOG(LL_INFO, "rgb: " << rgb << ", effect: " << effect);
    }
    
    void rgbEffects(RGBEffect const & a, RGBEffect const & b, RGBEffect const & dpad, RGBEffect const & sel, RGBEffect const & start) {
        StackProtection::check();
        LOG(LL_INFO, "rgb: a,     effect: " << a);
        LOG(LL_INFO, "rgb: b,     effect: " << b);
        LOG(LL_INFO, "rgb: dpad,  effect: " << dpad);
        LOG(LL_INFO, "rgb: sel,   effect: " << sel);
        LOG(LL_INFO, "rgb: start, effect: " << start);
    }
    
    void rgbOff() {
        StackProtection::check();
        LOG(LL_INFO, "rgb: off");
    }

    // memory

    bool memoryIsImmutable([[maybe_unused]] void const * ptr) {
        return false;
    }

    // budget

    uint32_t budget() {
        StackProtection::check();
        return budgetInfo::budget_;
    }

    uint32_t budgetDaily() {
        StackProtection::check();
        return budgetInfo::budgetDaily_;
    }

    void budgetSet(uint32_t seconds) {
        StackProtection::check();
        budgetInfo::budget_ = seconds;
    }

    void budgetDailySet(uint32_t seconds) {
        StackProtection::check();
        budgetInfo::budgetDaily_ = seconds;
    }

    void budgetReset() {
        StackProtection::check();
        budgetInfo::budget_ = budgetInfo::budgetDaily_; 
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