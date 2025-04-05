/** \page backend_mk3 Mk III 

    Mark III, currently in development whose hardware specifications are still in progress.
*/

#ifndef RCKID_BACKEND_MK3
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
#include "sd/sd.h"
#include "rckid/rckid.h"

#include "rckid/rckid.h"

extern "C" {
    extern uint8_t __cartridge_filesystem_start;
    extern uint8_t __cartridge_filesystem_end;
}

extern "C" {
    // implement not really working entropy function to silence the linker warning
    int _getentropy([[maybe_unused]] void *buffer, [[maybe_unused]] size_t length) {
        errno = ENOSYS;
        return -1;
    }    
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

    namespace filesystem {
        void initialize();
    }

    void fatalError(uint32_t error, uint32_t line, char const * file) {
        // simply go top BSOD - no need for HW cleanup
        bsod(error, line, file);
    }

    Writer debugWrite() {
        return Writer{[](char x) {
#if (defined RCKID_LOG_TO_SERIAL)
            uart_putc(uart0, x);
#else
            if (x == '\n') {
                tud_cdc_write("\r\n", 2);
                tud_cdc_write_flush();            
            } else {
                tud_cdc_write(& x, 1);
            }
#endif
        }};
    }

    uint8_t debugRead(bool echo) {
        UNIMPLEMENTED;
    }

    void initialize([[maybe_unused]] int argc, [[maybe_unused]] char const * argv[]) {
        UNIMPLEMENTED;
    }

    void tick() {
        UNIMPLEMENTED;
    }

    void yield() {
#if (defined RCKID_ENABLE_STACK_PROTECTION)
        memoryCheckStackProtection();
#endif
        tight_loop_contents();
        tud_task();
    }

    uint32_t uptimeUs() {
        return time_us_32();
    }

    // io

    bool btnDown(Btn b) {
        UNIMPLEMENTED;
    }

    bool btnPressed(Btn b) {
        UNIMPLEMENTED;
    }

    bool btnReleased(Btn b) {
        UNIMPLEMENTED;
    }

    int16_t accelX() {
        UNIMPLEMENTED;
    }

    int16_t accelY() {
        UNIMPLEMENTED;
    }

    int16_t accelZ() {
        UNIMPLEMENTED;
    }

    int16_t gyroX() {
        UNIMPLEMENTED;
    }

    int16_t gyroY() {
        UNIMPLEMENTED;
    }

    int16_t gyroZ() {
        UNIMPLEMENTED;
    }

    uint16_t lightAmbient() {
        UNIMPLEMENTED;        
    }
    
    uint16_t lightUV() {
        UNIMPLEMENTED;
    }

    // display

    DisplayRefreshDirection displayRefreshDirection() {
        return ST7789::refreshDirection();
    }

    void displaySetRefreshDirection(DisplayRefreshDirection value) {
        ST7789::setRefreshDirection(value);
    }

    uint8_t displayBrightness() {
        UNIMPLEMENTED;
    }

    void displaySetBrightness(uint8_t value) {
        UNIMPLEMENTED;
    }

    Rect displayUpdateRegion() {
        return ST7789::updateRegion();
    }

    void displaySetUpdateRegion(Rect value) {
        ST7789::setUpdateRegion(value);
    }

    void displaySetUpdateRegion(Coord width, Coord height) {
        displaySetUpdateRegion(Rect::XYWH((RCKID_DISPLAY_WIDTH - width) / 2, (RCKID_DISPLAY_HEIGHT - height) / 2, width, height));
    }

    bool displayUpdateActive() {
        return ST7789::dmaUpdateInProgress();
    }

    void displayWaitUpdateDone() {
        // TODO can we be smarter here and go to sleep?  
        while (displayUpdateActive())
            yield();
    }

    void displayWaitVSync() {
        ST7789::waitVSync();
    }

    void displayUpdate(uint16_t const * pixels, uint32_t numPixels, DisplayUpdateCallback callback) {
        ST7789::dmaUpdateAsync(pixels, numPixels, callback);
    }

    void displayUpdate(uint16_t const * pixels, uint32_t numPixels) {
        ST7789::dmaUpdateAsync(pixels, numPixels);
    }

    // audio

    void audioStreamRefill(void * buffer, unsigned int samples) {
        UNIMPLEMENTED;
    }

    bool audioHeadphones() {
        UNIMPLEMENTED;
    }

    bool audioPaused() {
        UNIMPLEMENTED;
    }

    bool audioPlayback() {
        UNIMPLEMENTED;
    }

    bool audioRecording() {
        UNIMPLEMENTED;
    }

    uint8_t audioVolume() {
        UNIMPLEMENTED;
    }

    void audioSetVolume(uint8_t value) {
        UNIMPLEMENTED;
    }

    void audioPlay(DoubleBuffer<int16_t> & buffer, uint32_t sampleRate, AudioCallback cb) {
        UNIMPLEMENTED;
    }

    void audioPause() {
        UNIMPLEMENTED;
    }

    void audioResume() {
        UNIMPLEMENTED;
    }

    void audioStop() {
        UNIMPLEMENTED;
    }

    // SD Card access is in sd/sd.cpp file

    // uint32_t sdCapacity() {}

    // bool sdReadBlocks(uint32_t start, uint8_t * buffer, uint32_t numBlocks) {}

    // bool sdWriteBlocks(uint32_t start, uint8_t const * buffer, uint32_t numBlocks) {}

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
        LOG(LL_LFS, "flash_range_program(" << offset << ", " << (uint32_t)FLASH_PAGE_SIZE << ") - start " << start);
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
        LOG(LL_LFS, "flash_range_erase(" << offset << ", " << (uint32_t)FLASH_SECTOR_SIZE << ") -- start " << start);
        uint32_t ints = save_and_disable_interrupts();
        flash_range_erase(offset, FLASH_SECTOR_SIZE);
        restore_interrupts(ints);
    }

    // rumbler

    void rumblerEffect(RumblerEffect const & effect) {
        UNIMPLEMENTED;
    }

    // rgb

    void rgbEffect(uint8_t rgb, RGBEffect const & effect) {
        UNIMPLEMENTED;
    }
    
    void rgbEffects(RGBEffect const & a, RGBEffect const & b, RGBEffect const & dpad, RGBEffect const & sel, RGBEffect const & start) {
        UNIMPLEMENTED;
    }
    
    void rgbOff() {
        UNIMPLEMENTED;
    }

}