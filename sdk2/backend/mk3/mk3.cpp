
#ifndef RCKID_BACKEND_MK3
#error "You are building fantasy (RayLib) backend without the indicator macro"
#endif

#ifdef PICO_RP2350A
#error "Invalid build settings, Rpi Pico SDK must be set to RP2350B"
#endif


#include <rckid/hal.h>
#include <rckid/error.h>
#include <rckid/graphics/color.h>

#define RCKID_DISPLAY_ZOOM 4

extern "C" {

    extern uint8_t __cartridge_filesystem_start;
    extern uint8_t __cartridge_filesystem_end;
    
}

namespace rckid::internal {

    namespace io {
        hal::State state;
    }
} // namespace rckid::internal

namespace rckid::hal {

    namespace device {

        void initialize() {
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
        }

        void fatalError(char const * file, uint32_t line, char const * msg, uint32_t payload) {
            // fatal error is simple on fantasy console as we do not have to worry about weird hardware states
            // stop audio playback, which is the only async stuff we can have
            audio::stop();
            // and call the SDKs default handler
            onFatalError(file, line, msg, payload);
        }

        Writer debugWrite() {
        }

        uint8_t debugRead() {
        }

    } // namespace rckid::hal::device

    namespace time {

        uint64_t uptimeUs() {
        }

        TinyDateTime now() {
        }

    } // namespace rckid::hal::time

    namespace io {

        State state() {
        }

        Point3D accelerometerState() {

        }

        Point3D gyroscopeState() {

        }

    } // namespace rckid::hal::io

    namespace display {

        void enable(Rect rect, RefreshDirection direction) {
        }

        void disable() {
        }

        void setBrightness(uint8_t value) {
        }

        bool vSync() {
        }

        void update(Callback callback) {
        }

        bool updateActive() {
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
            UNIMPLEMENTED;
        }

        uint8_t * heapEnd() {
            UNIMPLEMENTED;
        }

    } // namespace rckid::hal::memory

} // namespace rckid::hal

