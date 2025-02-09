/** \page backend_mk2 Mk II 

    Mark II backend which uses RP2040, PWM audio, PDM microphone and 2.8 320x240 display in 8bit MCU interface. 
*/

#ifndef RCKID_BACKEND_MK2
#error "You are building fantasy (RayLib) backend without the indicator macro"
#endif

#include <pico/rand.h>
#include <bsp/board.h>
#include "tusb_config.h"
#include "tusb.h"

extern "C" {
    #include <hardware/structs/usb.h>
    #include <hardware/uart.h>
    #include <hardware/flash.h>
}

#include "screen/ST7789.h"
#include "rckid/rckid.h"


extern "C" {
    extern uint8_t __cartridge_filesystem_start;
    extern uint8_t __cartridge_filesystem_end;
}

extern "C" {
    void *__wrap_malloc(size_t numBytes) { return rckid::Heap::allocBytes(numBytes); }
    void __wrap_free(void * ptr) { rckid::Heap::free(ptr); }

    void *__wrap_calloc(size_t numBytes) {
        void * result = rckid::Heap::allocBytes(numBytes);
        memset(result, 0, numBytes);
        return result;
    }
}

namespace rckid {

    // forward declaration of the bsod function
    NORETURN(void bsod(uint32_t error, uint32_t line = 0, char const * file = nullptr));


    void fatalError(uint32_t error, uint32_t line, char const * file) {
    }

    Writer debugWrite() {
    }

    void initialize([[maybe_unused]] int argc, [[maybe_unused]] char const * argv[]) {
    }

    void tick() {
    }

    void yield() {
    }

    uint32_t uptimeUs() {
    }

    // io

    bool btnDown(Btn b) {
    }

    bool btnPressed(Btn b) {
    }

    bool btnReleased(Btn b) {
    }

    int16_t accelX() {
    }

    int16_t accelY() {
    }

    int16_t accelZ() {
    }

    int16_t gyroX() {
    }

    int16_t gyroY() {
    }

    int16_t gyroZ() {
    }

    // display

    DisplayResolution displayResolution() {
    }

    void displaySetResolution(DisplayResolution value) {
    }

    DisplayRefreshDirection displayRefreshDirection() {
    }

    void displaySetRefreshDirection(DisplayRefreshDirection value) {
    }

    uint8_t displayBrightness() {
    }

    void displaySetBrightness(uint8_t value) {
    }

    Rect displayUpdateRegion() {
    }

    void displaySetUpdateRegion(Rect value) {
    }

    bool displayUpdateActive() {
    }

    void displayWaitUpdateDone() {
    }

    void displayWaitVSync() {
    }

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels, DisplayUpdateCallback callback) {
    }

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels) {
    }

    // audio

    void audioStreamRefill(void * buffer, unsigned int samples) {
    }

    bool audioHeadphones() {
    }

    bool audioPaused() {
    }

    bool audioPlayback() {
    }

    bool audioRecording() {
    }

    uint8_t audioVolume() {
    }

    void audioSetVolume(uint8_t value) {
    }

    void audioPlay(DoubleBuffer<int16_t> & buffer, uint32_t sampleRate, AudioCallback cb) {
    }

    void audioPause() {
    }

    void audioResume() {
    }

    void audioStop() {
    }

    // SD Card access

    uint32_t sdCapacity() {
    }

    bool sdReadBlocks(uint32_t start, uint8_t * buffer, uint32_t numBlocks) {
    }

    bool sdWriteBlocks(uint32_t start, uint8_t const * buffer, uint32_t numBlocks) {
    }

    // Cartridge filesystem access

    uint32_t cartridgeCapacity() { 
        return &__cartridge_filesystem_end - &__cartridge_filesystem_start;
    }

    uint32_t cartridgeWriteSize() {
        return FLASH_PAGE_SIZE; // 256
    }

    uint32_t cartridgeEraseSize() {
        return FLASH_SECTOR_SIZE; // 4096
    }

    void cartridgeRead(uint32_t start, uint8_t * buffer, uint32_t numBytes) {
        // since flash is memory mapped via XIP, all we need to do is aggregate offset properly 
        memcpy(buffer, XIP_NOCACHE_NOALLOC_BASE + (&__cartridge_filesystem_start - XIP_BASE) + start, numBytes);
    }

    void cartridgeWrite(uint32_t start, uint8_t const * buffer) {
        ASSERT(start < cartridgeCapacity());
        ASSERT(start + FLASH_PAGE_SIZE <= cartridgeCapacity());
        uint32_t offset = reinterpret_cast<uint32_t>(& __cartridge_filesystem_start) - XIP_BASE + start;
        LOG(LL_LFS, "flash_range_program(" << offset << ", " << FLASH_PAGE_SIZE << ") - start " << start);
        uint32_t ints = save_and_disable_interrupts();
        flash_range_program(offset, buffer, FLASH_PAGE_SIZE);
        restore_interrupts(ints);
    }

    void cartridgeErase(uint32_t start) {
        ASSERT(start < cartridgeCapacity());
        ASSERT(start + FLASH_SECTOR_SIZE <= cartridgeCapacity());
        uint32_t offset = reinterpret_cast<uint32_t>(& __cartridge_filesystem_start) - XIP_BASE + start;
        //TRACE_LITTLEFS("cart_fs_start: " << (uint32_t)(& __cartridge_filesystem_start));         
        //TRACE_LITTLEFS("XIP_BASE:      " << (uint32_t)(XIP_BASE));
        LOG(LL_LFS, "flash_range_erase(" << offset << ", " << FLASH_SECTOR_SIZE << ") -- start " << start);
        uint32_t ints = save_and_disable_interrupts();
        flash_range_erase(offset, FLASH_SECTOR_SIZE);
        restore_interrupts(ints);
    }

}