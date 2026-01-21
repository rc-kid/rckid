#include <chrono>
// TODO delete
#include <iostream>
#include <raylib.h>

#undef BLACK
#define BLACK (::Color){0, 0, 0, 255}
#undef WHITE
#define WHITE (::Color){255, 255, 255, 255}


#include <rckid/hal.h>
#include <rckid/error.h>
#include <rckid/graphics/color.h>

#define RCKID_DISPLAY_ZOOM 4


namespace rckid::internal {

    namespace time {
        TinyDateTime now;
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    } // rckid::internal::time

    namespace io {
        hal::State state;
    }

    namespace display {
        hal::display::RefreshDirection direction;
        Rect rect;
        
        Image img;
        Texture texture;
        int16_t x;
        int16_t y;
        uint8_t brightness;

        uint64_t lastRefresh = 0;

        void refresh() {
            UpdateTexture(texture, img.data);
            BeginDrawing();
            DrawTextureEx(texture, {0,0}, 0, RCKID_DISPLAY_ZOOM, WHITE);

            // TODO print FPS, memory or some other stat overlays we might want to 

            EndDrawing();
            SwapScreenBuffer();
            // set last refresh time so that we can calculate vsync
            lastRefresh = hal::time::uptimeUs();
        }

        void writePixel(uint16_t pixel) {
            // write the pixel
            Color c = Color::RGB565(pixel);
            // TODO and change according to brightness value
            ImageDrawPixel(&img, x, y, { c.r, c.g, c.b, 255});
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
    } // rckid::internal::display

    namespace memory {

        uint8_t heap[512 * 1024];

    } // rckid::internal::memory

} // namespace rckid::internal

namespace rckid::hal {

    namespace device {

        void initialize() {
            InitWindow(320 * RCKID_DISPLAY_ZOOM, 240 * RCKID_DISPLAY_ZOOM, "RCKid");
            internal::display::img = GenImageColor(display::WIDTH, display::HEIGHT, BLACK);
            internal::display::texture = LoadTextureFromImage(internal::display::img);

        }

        void powerOff() {
            UNIMPLEMENTED;
        }

        void sleep() {
            UNIMPLEMENTED;
        }

        void scheduleWakeup(uint32_t timeoutSeconds, uint32_t payload) {
            UNIMPLEMENTED;
        }

        void onTick() {

        }

        void onYield() {
            // do nothing on yield
        }

        void fatalError(char const * file, uint32_t line, char const * msg, uint32_t payload) {
            // fatal error is simple on fantasy console as we do not have to worry about weird hardware states
            // stop audio playback, which is the only async stuff we can have
            audio::stop();
            // and call the SDKs default handler
            onFatalError(file, line, msg, payload);
        }

        Writer debugWrite() {
            return Writer{[](char c) {
                std::cout << c;
                if (c == '\n')
                    std::cout << std::flush;
            }};
        }

        uint8_t debugRead() {
            while (true) {
                int x = GetCharPressed();
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

        State state() {
            return internal::io::state;
        }

        Point3D accelerometerState() {

        }

        Point3D gyroscopeState() {

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

        }

        void setBrightness(uint8_t value) {
            internal::display::brightness = value;
        }

        bool vSync() {
            // simulate the mkIII display driver by mimicking the vsync timing
            // TODO
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

        bool updateActive() {
            // in fantasy backend, update is always synchronous with the main thread, so it is *never* active when this function can be called
            return false;
        }

    } // namespace rckid::hal::display

    namespace audio {

        void setVolumeHeadphones(uint8_t value) {

        }

        void setVolumeSpeaker(uint8_t value) {

        }

        void play(uint32_t sampleRate, Callback cb) {

        }

        void recordMic(uint32_t sampleRate, Callback cb) {

        }

        void recordLineIn(uint32_t sampleRate, Callback cb) {

        }

        void pause() {

        }

        void resume() {

        }

        void stop() {

        }

    } // namespace rckid::hal::audio

    namespace fs {

        uint32_t sdCapacityBlocks() {

        }

        bool sdReadBlocks(uint32_t blockNum, uint8_t * buffer, uint32_t numBlocks) {

        }

        bool sdWriteBlocks(uint32_t blockNum, uint8_t const * buffer, uint32_t numBlocks) {

        }

        uint32_t cartridgeCapacityBytes() {

        }

        uint32_t cartridgeWriteSizeBytes() {

        }

        uint32_t cartridgeEraseSize() {

        }

        void cartridgeRead(uint32_t start, uint8_t * buffer, uint32_t numBytes) {

        }

        void cartridgeWrite(uint32_t start, uint8_t const * buffer) {

        }

        void cartridgeErase(uint32_t start) {

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

    } // namespace rckid::hal::memory

} // namespace rckid::hal

