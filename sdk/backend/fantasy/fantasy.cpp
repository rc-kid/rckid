#include <chrono>
// TODO delete
#include <iostream>
#include <raylib.h>

#undef BLACK
#define BLACK (::Color){0, 0, 0, 255}
#undef WHITE
#define WHITE (::Color){255, 255, 255, 255}
#undef RED
#define RED (::Color){255, 0, 0, 255}


#include <rckid/hal.h>
#include <rckid/error.h>
#include <rckid/memory.h>
#include <rckid/graphics/color.h>

#include "system_malloc_guard.h"

#define RCKID_DISPLAY_ZOOM 4

extern "C" {
#ifndef RCKID_NO_RODATA_BOUNDARIES            
    extern uint8_t __start_rodata;
    extern uint8_t __stop_rodata;
#endif    
}

namespace rckid::fs {
    void initializeFilesystem();
}

namespace rckid::internal {

    namespace memory {

        uint8_t heap[512 * 1024];

        uint32_t useSystemMalloc = 1;

    } // rckid::internal::memory

    namespace storage {
        uint8_t data[1024];
    } // rckid::internal::storage

    namespace device {
        // part of a mechanism to exit the app during a yield in fatal error mode. fatalError sets this to true if we do not have window (indicating cli execution) and then onYield kills the app immediately.
        // has no effect in GUI mode when we simulate the BSOD from the device
        bool exitAtYield = false;

    } // rckid::internal::device

    namespace time {
        TinyDateTime now;
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    } // rckid::internal::time

    namespace io {
        DeviceState state;
    }

    namespace display {
        bool noWindow = false;
        hal::display::RefreshDirection direction;
        Rect rect;
        
        Image img;
        Texture texture;
        int16_t x;
        int16_t y;
        uint8_t brightness;

        uint64_t lastRefresh = 0;

        void refresh() {
            internal::memory::SystemMallocGuard g_;
            UpdateTexture(texture, img.data);
            BeginDrawing();
            DrawTextureEx(texture, {0,0}, 0, RCKID_DISPLAY_ZOOM, WHITE);

            // TODO print FPS, memory or some other stat overlays we might want to 

            DrawText(TextFormat("Heap reserved: %d", Heap::reservedBytes() / 1024), 0, 20 * RCKID_DISPLAY_ZOOM + 5, 20, RED);
            DrawText(TextFormat("Heap used:     %d", Heap::usedBytes() / 1024), 0, 20 * RCKID_DISPLAY_ZOOM + 25, 20, RED);

            EndDrawing();
            SwapScreenBuffer();
            // set last refresh time so that we can calculate vsync
            lastRefresh = hal::time::uptimeUs();
        }

        void writePixel(uint16_t pixel) {
            // write the pixel
            Color c = Color::RGB565(pixel);
            // TODO and change according to brightness value
            {
                internal::memory::SystemMallocGuard g_;
                ImageDrawPixel(&img, x, y, { c.r, c.g, c.b, 255});
            }
            // update the x & y coordinates and refresh if necessary
            switch (direction) {
                case hal::display::RefreshDirection::ColumnFirst:
                    if (++y >= rect.bottom()) {
                        y = rect.top();
                        if (--x < rect.left()) {
                            x = (rect.right() - 1);
                            refresh();
                        }
                    }
                    break;
                case hal::display::RefreshDirection::RowFirst:
                    if (++x >= rect.right()) {
                        x = rect.left();
                        if (++y >= rect.bottom()) {
                            y = rect.top();
                            refresh();
                        }
                    }
                    break;
            }
        }
    } // namespace rckid::internal::display

    namespace audio {

        AudioStream stream;
        hal::audio::Callback cb;        

        int16_t * currentBuffer = nullptr;
        uint32_t currentBufferSize = 0;
        int16_t * nextBuffer = nullptr;
        uint32_t nextBufferSize = 0;

        uint32_t currentBufferIndex = 0;

        uint8_t volume = 10;

        bool playbackShouldStop_ = false;

        /** Raylib stream refill callback.
         */
        void refillStream(void * buffer, unsigned int samples) {
            // get the stereo view of the input buffer
            int16_t * stereo = reinterpret_cast<int16_t*>(buffer);
            while (samples-- != 0) {
                if (currentBufferIndex >= currentBufferSize * 2) {
                    // go for the next buffer which should already be preloaded
                    std::swap(currentBuffer, nextBuffer);
                    std::swap(currentBufferSize, nextBufferSize);
                    if (currentBuffer == nullptr) {
                        // require playback stop (we can't stop immediately as the refill function runs from the audio thread and raylib would deadlock)
                        playbackShouldStop_ = true;
                        return;
                    }
                    currentBufferIndex = 0;
                    // inform the callback that we have retired the current buffer and ask for replacement, which will be our next buffer
                    audio::cb(nextBuffer, nextBufferSize);
                }
                int16_t l = currentBuffer[currentBufferIndex++];
                int16_t r = currentBuffer[currentBufferIndex++];
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
        

    } // namespace rckid::internal::audio

    namespace fs {
        std::fstream sd_;
        uint32_t sdBlocks_ = 0;
        std::fstream cartridge_;
        uint32_t cartridgeSize_ = 0;

    } // namespace rckid::internal::fs

} // namespace rckid::internal


namespace rckid::hal {

    namespace device {

        /** Initialize version useful for tests that initializes the fantasy backend, but without the visible window. Very useful for tests. 
         */
        void initializeNoWindow() {
            internal::display::noWindow = true;
#ifndef RCKID_NO_RODATA_BOUNDARIES            
            LOG(LL_INFO, "Immutable memory: " << hex(& __start_rodata) << " - " << hex(& __stop_rodata));
#endif
#ifndef RCKID_CUSTOM_FILESYSTEM
            // if we are not using custom filesystem, see if we have the image files available, open them for read/write access and initialize the hal::fs mechanics
            internal::fs::sd_.open("sd.iso", std::ios::in |  std::ios::out | std::ios::binary);
            if (internal::fs::sd_.is_open()) {
                internal::fs::sd_.seekg(0,  std::ios::end);
                size_t sizeBytes = internal::fs::sd_.tellg();
                LOG(LL_INFO, "sd.iso file found, mounting SD card - " << sizeBytes << " bytes");
                if (sizeBytes % 512 == 0 && sizeBytes != 0) {
                    internal::fs::sdBlocks_ = static_cast<uint32_t>(sizeBytes / 512);
                    LOG(LL_INFO, "    blocks: " << internal::fs::sdBlocks_);
                } else {
                    LOG(LL_INFO, "    invalid file size (multiples of 512 bytes allowed)");
                }
            }
            internal::fs::cartridge_.open("flash.iso", std::ios::in |  std::ios::out | std::ios::binary);
            if (internal::fs::cartridge_.is_open()) {
                internal::fs::cartridge_.seekg(0,  std::ios::end);
                size_t sizeBytes = internal::fs::cartridge_.tellg();
                LOG(LL_INFO, "flash.iso file found, mounting cartridge store - " << sizeBytes << " bytes");
                if (sizeBytes % 4096 == 0 && sizeBytes != 0) {
                    internal::fs::cartridgeSize_ = static_cast<uint32_t>(sizeBytes);
                    LOG(LL_INFO, "    size: " << internal::fs::cartridgeSize_);
                } else {
                    LOG(LL_INFO, "    invalid file size (multiples of 4096 bytes allowed)");
                }
            }
#endif
            // load the avr storage if we have the file available
            std::fstream storageFile("avr-storage.dat", std::ios::in | std::ios::binary);
            if (storageFile.is_open()) {
                storageFile.read(reinterpret_cast<char*>(internal::storage::data), sizeof(internal::storage::data));
                LOG(LL_INFO, "avr-storage.dat file found, loaded " << storageFile.gcount() << " bytes of persistent storage");
            } else {
                memset(internal::storage::data, 0, sizeof(internal::storage::data));
            }
            internal::memory::useSystemMalloc = 0;
            rckid::fs::initializeFilesystem();
        }

        void initialize() {
            InitWindow(320 * RCKID_DISPLAY_ZOOM, 240 * RCKID_DISPLAY_ZOOM, "RCKid");
            internal::display::img = GenImageColor(display::WIDTH, display::HEIGHT, BLACK);
            internal::display::texture = LoadTextureFromImage(internal::display::img);
            // initialize the audio device
            InitAudioDevice();

            initializeNoWindow();
        }

        void setPowerMode([[maybe_unused]] PowerMode mode) {
            // no power management on fantasy console
        }

        void powerOff() {
            LOG(LL_INFO, "Shutting down");
            std::exit(0);
        }

        void sleep() {
            UNIMPLEMENTED;
        }

        void scheduleWakeup([[maybe_unused]] uint32_t timeoutSeconds, [[maybe_unused]] uint32_t payload) {
            UNIMPLEMENTED;
        }

        void onTick() {
            internal::memory::SystemMallocGuard g;
            if (WindowShouldClose())
                internal::io::state.setPowerOffInterrupt(true);
        }

        void onYield() {
            if (internal::device::exitAtYield)
                exit(-1);
            if (internal::audio::playbackShouldStop_)
                audio::stop();
        }

        void fatalError(char const * file, uint32_t line, char const * msg, uint32_t payload) {
            // fatal error is simple on fantasy console as we do not have to worry about weird hardware states
            // stop audio playback, which is the only async stuff we can have
            audio::stop();
            // deallocate SD and cartridge filesystems
#ifndef RCKID_CUSTOM_FILESYSTEM            
            if (internal::fs::sd_.good())   
                internal::fs::sd_.close();
            if (internal::fs::cartridge_.good())
                internal::fs::cartridge_.close();
#endif
            // and call the SDKs default handler, setting exitAtYield to true if we lack app window
            internal::device::exitAtYield = internal::display::noWindow;
            onFatalError(file, line, msg, payload);
        }

        Writer debugWrite() {
            return Writer{[](char c) {
                internal::memory::SystemMallocGuard g_;
                std::cout << c;
                if (c == '\n')
                    std::cout << std::flush;
            }};
        }

        uint8_t debugRead() {
            while (true) {
                int x;
                {
                    internal::memory::SystemMallocGuard g_;
                    x = GetCharPressed();
                }
                // skip non ASCII characters on the input
                if (x > 0xff)
                    continue;
                return static_cast<uint8_t>(x);
            }
        }

    } // namespace rckid::hal::device

    namespace time {

        uint64_t uptimeUs() {
            using namespace std::chrono;
            return static_cast<uint64_t>(duration_cast<microseconds>(steady_clock::now() - internal::time::start).count()); 
        }

        TinyDateTime now() {
            return internal::time::now;
        }

    } // namespace rckid::hal::time

    namespace io {

        DeviceState state() {
            internal::memory::SystemMallocGuard g;
            PollInputEvents();
            internal::io::state.setButton(Btn::Up, IsKeyDown(KEY_UP));
            internal::io::state.setButton(Btn::Down, IsKeyDown(KEY_DOWN));
            internal::io::state.setButton(Btn::Left, IsKeyDown(KEY_LEFT));
            internal::io::state.setButton(Btn::Right, IsKeyDown(KEY_RIGHT));
            internal::io::state.setButton(Btn::A, IsKeyDown(KEY_A));
            internal::io::state.setButton(Btn::B, IsKeyDown(KEY_B));
            internal::io::state.setButton(Btn::Select, IsKeyDown(KEY_SPACE));
            internal::io::state.setButton(Btn::Start, IsKeyDown(KEY_ENTER));    
            internal::io::state.setButton(Btn::Home, IsKeyDown(KEY_H));
            internal::io::state.setButton(Btn::VolumeUp, IsKeyDown(KEY_PAGE_UP));
            internal::io::state.setButton(Btn::VolumeDown, IsKeyDown(KEY_PAGE_DOWN));
            // in fantasy mode, certain keys can be used to simulate other hardware events, namely:
            if (IsKeyPressed(KEY_ONE)) {
                uint32_t vcc = internal::io::state.vcc();
                vcc = (vcc >= 10) ? (vcc - 10) : 0;
                internal::io::state.setVcc(vcc);
                LOG(LL_INFO, "VCC: " << internal::io::state.vcc());
            }
            if (IsKeyPressed(KEY_TWO)) {
                uint32_t vcc = internal::io::state.vcc();
                vcc = (vcc >= 300) ? (vcc + 10) : 300;
                internal::io::state.setVcc(vcc);
                LOG(LL_INFO, "VCC: " << internal::io::state.vcc());
            }
            if (IsKeyPressed(KEY_THREE)) {
                internal::io::state.setHeadphonesConnected(!internal::io::state.headphonesConnected());
                LOG(LL_INFO, "Headphones: " << (internal::io::state.headphonesConnected() ? "connected" : "disconnected"));
            }
            if (IsKeyPressed(KEY_FOUR)) {
                internal::io::state.setCharging(!internal::io::state.charging());
                LOG(LL_INFO, "Charging: " << (internal::io::state.charging() ? "on" : "off"));
            }
            // and return
            auto result = internal::io::state;
            internal::io::state.clearInterrupts();
            return result;
        }

        Point3D accelerometerState() {
            UNIMPLEMENTED;

        }

        Point3D gyroscopeState() {
            UNIMPLEMENTED;
        }

    } // namespace rckid::hal::io

    namespace display {

        void enable(Rect rect, RefreshDirection direction) {
            internal::display::rect = rect;
            internal::display::direction = direction;
            switch (direction) {
                case RefreshDirection::ColumnFirst:
                    internal::display::x = rect.right() - 1;
                    internal::display::y = rect.top();
                    break;
                case RefreshDirection::RowFirst:
                    internal::display::x = rect.left();
                    internal::display::y = rect.top();
                    break;
            }
        }

        void disable() {
            UNIMPLEMENTED;
        }

        void setBrightness(uint8_t value) {
            internal::display::brightness = value;
        }

        bool vSync() {
            // TODO
            // simulate the mkIII display driver by mimicking the vsync timing. For now we just alternate between vsync and not vsync to simulate vsync in progress and done
            static bool value = true;
            value = ! value;
            return value;
        }

        void update(Callback callback) {
            Color::RGB565 * buffer = nullptr;
            uint32_t bufferSize = 0;
            while (true) {
                callback(buffer, bufferSize);
                if (buffer == nullptr)
                    break;
                // append the pixels from the buffer, 
                for (uint32_t i = 0; i < bufferSize; ++i)
                    internal::display::writePixel(buffer[i]);
            }
        }

        void update(Color::RGB565 const * buffer, uint32_t bufferSize) {
            for (uint32_t i = 0; i < bufferSize; ++i)
                internal::display::writePixel(buffer[i]);
        }

        void update(Color::RGB565 const * buffer1, uint32_t bufferSize1, Color::RGB565 const * buffer2, uint32_t bufferSize2) {
            update(buffer1, bufferSize1);
            update(buffer2, bufferSize2);
        }

        bool updateActive() {
            // in fantasy backend, update is always synchronous with the main thread, so it is *never* active when this function can be called
            return false;
        }

    } // namespace rckid::hal::display

    namespace audio {

        void setVolumeHeadphones(uint8_t value) {
            internal::audio::volume = value;
        }

        void setVolumeSpeaker(uint8_t value) {
            internal::audio::volume = value;
        }

        void play(uint32_t sampleRate, Callback cb) {
            stop();
            internal::audio::cb = cb;
            internal::audio::currentBuffer = nullptr;
            internal::audio::currentBufferSize = 0;
            internal::audio::nextBuffer = nullptr;
            internal::audio::nextBufferSize = 0;
            internal::audio::currentBufferIndex = 0;

            cb(internal::audio::currentBuffer, internal::audio::currentBufferSize);
            ASSERT(internal::audio::currentBuffer != nullptr);
            ASSERT(internal::audio::currentBufferSize != 0);
            cb(internal::audio::nextBuffer, internal::audio::nextBufferSize);
            {
                internal::memory::SystemMallocGuard g;
                internal::audio::stream = LoadAudioStream(sampleRate, 16, 2);
                SetAudioStreamCallback(internal::audio::stream, internal::audio::refillStream);   
                PlayAudioStream(internal::audio::stream);
            }
        }

        void recordMic([[maybe_unused]] uint32_t sampleRate, [[maybe_unused]] Callback cb) {
            UNIMPLEMENTED;
        }

        void recordLineIn([[maybe_unused]] uint32_t sampleRate, [[maybe_unused]] Callback cb) {
            UNIMPLEMENTED;
        }

        void pause() {
            internal::memory::SystemMallocGuard g;
            if (IsAudioStreamValid(internal::audio::stream))
                PauseAudioStream(internal::audio::stream);
        }

        void resume() {
            internal::memory::SystemMallocGuard g;
            if (IsAudioStreamValid(internal::audio::stream))
                ResumeAudioStream(internal::audio::stream);
        }

        // TODO when called multiple times, the function segfaults on stream uload
        void stop() {
            internal::memory::SystemMallocGuard g;
            if (IsAudioStreamValid(internal::audio::stream)) {
                StopAudioStream(internal::audio::stream);
                UnloadAudioStream(internal::audio::stream);
                internal::audio::stream.buffer = nullptr;
                internal::audio::playbackShouldStop_ = false;
            }
        }

        bool isPlaying() {
            return IsAudioStreamValid(internal::audio::stream);
        }

        bool isRecording(){
            return false;
        }

        bool isPaused() {
            if (isPlaying())
                return IsAudioStreamPlaying(internal::audio::stream) == false;
            return false;
        }


    } // namespace rckid::hal::audio

    namespace fs {

        uint32_t sdCapacityBlocks() {
            return internal::fs::sdBlocks_;
        }

        void sdReadBlocks(uint32_t blockNum, uint8_t * buffer, uint32_t numBlocks) {
            ASSERT(internal::fs::sd_.good());
            try {
                internal::memory::SystemMallocGuard g;
                internal::fs::sd_.seekg(static_cast<std::streamoff>(blockNum) * 512);
                internal::fs::sd_.read(reinterpret_cast<char *>(buffer), static_cast<std::streamsize>(numBlocks) * 512);
            } catch (std::exception const & e) {
                LOG(LL_ERROR, "SD card read error: " << e.what());
                FATAL_ERROR("io");
            }
        }

        void sdWriteBlocks(uint32_t blockNum, uint8_t const * buffer, uint32_t numBlocks) {
            ASSERT(internal::fs::sd_.good());
            try {
                internal::memory::SystemMallocGuard g;
                internal::fs::sd_.seekp(static_cast<std::streamoff>(blockNum) * 512);
                internal::fs::sd_.write(reinterpret_cast<char const *>(buffer), static_cast<std::streamsize>(numBlocks) * 512);
            } catch (std::exception const & e) {
                LOG(LL_ERROR, "SD card read error: " << e.what());
                FATAL_ERROR("io");
            }
        }

        uint32_t cartridgeCapacityBytes() {
            return internal::fs::cartridgeSize_;
        }

        uint32_t cartridgeWriteSizeBytes() {
            return 256; // default value on mkIII
        }

        uint32_t cartridgeEraseSizeBytes() {
            return 4096; // defult valye on mkIII
        }

        void cartridgeRead(uint32_t start, uint8_t * buffer, uint32_t numBytes) {
            ASSERT(internal::fs::cartridge_.good());
            try {
                internal::memory::SystemMallocGuard g;
                internal::fs::cartridge_.seekg(start);
                internal::fs::cartridge_.read(reinterpret_cast<char *>(buffer), numBytes);
            } catch (std::exception const & e) {
                LOG(LL_ERROR, "SD card read error: " << e.what());
                FATAL_ERROR("io");
            }
        }

        void cartridgeWrite(uint32_t start, uint8_t const * buffer, uint32_t numBytes) {
            ASSERT(internal::fs::cartridge_.good());
            try {
                internal::memory::SystemMallocGuard g;
                internal::fs::cartridge_.seekp(start);
                internal::fs::cartridge_.write(reinterpret_cast<char const *>(buffer), numBytes);
            } catch (std::exception const & e) {
                LOG(LL_ERROR, "SD card read error: " << e.what());
                FATAL_ERROR("io");
            }
        }

        void cartridgeErase(uint32_t start) {
            ASSERT(start % 4096 == 0);
            ASSERT(start + 4096 <= internal::fs::cartridgeSize_);
            try {
                internal::memory::SystemMallocGuard g;
                internal::fs::cartridge_.seekp(start);
                uint8_t buffer[4096];
                std::memset(buffer, 4096, 0xff);
                internal::fs::cartridge_.write(reinterpret_cast<char const *>(buffer), 4096);
            } catch (std::exception const & e) {
                LOG(LL_ERROR, "Cartridge flash erase error: " << e.what());
                FATAL_ERROR("io");
            }
        }

    } // namespace rckid::hal::fs

    namespace memory {

        uint8_t * heapStart() {
            return internal::memory::heap;
        }

        uint8_t * heapEnd() {
            // TODO should we account for stack size here as well? 
            return internal::memory::heap + sizeof(internal::memory::heap);
        }

        bool isImmutableDataPtr(void const * ptr) {
#ifndef RCKID_NO_RODATA_BOUNDARIES            
            return (ptr >= & __start_rodata) && (ptr < & __stop_rodata);
#else
            // if we do not have rodata boundaries, we can only check if the pointer is outside of the heap, which is a good heuristic as in fantasy console all non-heap memory is immutable
            return (ptr < heapStart()) || (ptr >= heapEnd());
#endif
        }


    } // namespace rckid::hal::memory

    namespace storage {

        void load(uint16_t start, uint8_t * buffer, uint32_t numBytes) {
            ASSERT(start + numBytes <= sizeof(internal::storage::data));
            std::memcpy(buffer, internal::storage::data + start, numBytes);
        }

        void save(uint16_t start, uint8_t const * buffer, uint32_t numBytes) {
            ASSERT(start + numBytes <= sizeof(internal::storage::data));
            std::memcpy(internal::storage::data + start, buffer, numBytes);
            // save to the file system to preserve persistent storage across runs
            std::fstream storageFile("avr-storage.dat", std::ios::out | std::ios::binary);
            if (storageFile.is_open()) {
                storageFile.write(reinterpret_cast<char const *>(internal::storage::data), sizeof(internal::storage::data));
                LOG(LL_INFO, "Saved " << sizeof(internal::storage::data) << " bytes of persistent storage to avr-storage.dat file");
            } else {
                LOG(LL_ERROR, "Failed to save persistent storage to avr-storage.dat file"); 
            }
        }
    } // namespace rckid::hal::storage

} // namespace rckid::hal

