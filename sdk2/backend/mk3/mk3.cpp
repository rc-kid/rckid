
#ifndef RCKID_BACKEND_MK3
#error "You are building fantasy (RayLib) backend without the indicator macro"
#endif

#ifdef PICO_RP2350A
#error "Invalid build settings, Rpi Pico SDK must be set to RP2350B"
#endif

#include <platform.h>


#include <pico/time.h>
#include <pico/rand.h>
#include <bsp/board.h>

#include <hardware/uart.h>
#include <hardware/clocks.h>
#include <hardware/flash.h>


#include <rckid/hal.h>
#include <rckid/error.h>
#include <rckid/memory.h>
#include <rckid/graphics/color.h>

#include "tusb_config.h"
#include "tusb.h"

#include "screen/ST7789.h"


namespace rckid::fs {
    void initializeFilesystem();
}

namespace rckid::internal {

    namespace device {
        // set to true once we have debugging cpability (otherwise if there were debugging prints in static initializations, they crash the device)
        bool debugReady = false;
    }

    namespace time {
        TinyDateTime now;
    } // rckid::internal::time

    namespace display {

    }

    namespace fs {

    }

    void irqDMADone() {

    }

} // namespace rckid::internal

extern "C" {

    extern uint8_t __bss_end__;
    extern uint8_t __StackTop;

    extern uint8_t __cartridge_filesystem_start;
    extern uint8_t __cartridge_filesystem_end;
    
    // implement not really working entropy function to silence the linker warning
    /*
    int _getentropy([[maybe_unused]] void *buffer, [[maybe_unused]] size_t length) {
        errno = ENOSYS;
        return -1;
    } 
        */   

    void *__wrap_malloc(size_t numBytes) {
        auto x = save_and_disable_interrupts();
        void * result = rckid::Heap::alloc(numBytes);
        restore_interrupts(x);
        return result;
    }

    void __wrap_free(void * ptr) { 
        auto x = save_and_disable_interrupts();
        if (rckid::Heap::contains(ptr))
            rckid::Heap::free(ptr); 
        restore_interrupts(x);
    }

    void *__wrap_calloc(size_t numElements, size_t elementSize) {
        size_t numBytes = numElements * elementSize;
        void * result = __wrap_malloc(numBytes);
        memset(result, 0, numBytes);
        return result;
    }

}

namespace rckid::internal {

    namespace io {
        hal::State state;
    }
} // namespace rckid::internal

namespace rckid::hal {

    namespace device {

        void initialize() {
            // first initailize the board and USB stack / debugging for some basic output capability
            board_init();
            tud_init(BOARD_TUD_RHPORT);
#if (RCKID_LOG_TO_SERIAL == 1)
            // initialize uart0 on pins 16 & 17 as serial out
            uart_init(uart0, RCKID_SERIAL_SPEED);
            gpio_set_function(RCKID_LOG_SERIAL_TX_PIN, GPIO_FUNC_UART);
            gpio_set_function(RCKID_LOG_SERIAL_RX_PIN, GPIO_FUNC_UART);
#endif
            internal::device::debugReady = true;

            // set DMA IRQ0 as exclusive for the SDK (shared between display, audio, etc.), enable its IRQ and set the handler
            irq_set_exclusive_handler(DMA_IRQ_0, internal::irqDMADone);
            // enable DMA IRQ (used by display, audio, etc.)
            irq_set_enabled(DMA_IRQ_0, true);

            // TODO enable the async I2C driver

            // enable the screen
            ST7789::reset();            








            // old not sure if we need in this reqrite

            // enable GPIO IRQ
            irq_set_enabled(IO_IRQ_BANK0, true);


            // TODO

            rckid::fs::initializeFilesystem();
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
            tight_loop_contents();
            tud_task();
        }

        void fatalError(char const * file, uint32_t line, char const * msg, uint32_t payload) {
            // fatal error is simple on fantasy console as we do not have to worry about weird hardware states
            // stop audio playback, which is the only async stuff we can have
            audio::stop();
            // and call the SDKs default handler
            onFatalError(file, line, msg, payload);
        }

        Writer debugWrite() {
            return Writer{[](char x) {
                if (internal::device::debugReady == false)
                    return;
#if (RCKID_LOG_TO_SERIAL == 1)
                if (x == '\n')
                    uart_putc(uart0, '\r');
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

        uint8_t debugRead() {
            char cmd_ = ' ';
#if (RCKID_LOG_TO_SERIAL == 1)
            cmd_ = uart_getc(uart0);
#else 
            while (tud_cdc_read(& cmd_, 1) != 1) { yield(); };
#endif
            return static_cast<uint8_t>(cmd_);
        }

    } // namespace rckid::hal::device

    namespace time {

        uint64_t uptimeUs() {
            return time_us_64();

        }

        TinyDateTime now() {
            return internal::time::now;
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

        void sdReadBlocks(uint32_t blockNum, uint8_t * buffer, uint32_t numBlocks) {

        }

        void sdWriteBlocks(uint32_t blockNum, uint8_t const * buffer, uint32_t numBlocks) {

        }

        uint32_t cartridgeCapacityBytes() {
            return &__cartridge_filesystem_end - &__cartridge_filesystem_start;
        }

        uint32_t cartridgeWriteSizeBytes() {
            static_assert(FLASH_PAGE_SIZE == 256);
            return FLASH_PAGE_SIZE;
        }

        uint32_t cartridgeEraseSizeBytes() {
            static_assert(FLASH_SECTOR_SIZE == 4096);
            return FLASH_SECTOR_SIZE; 
        }

        void cartridgeRead(uint32_t start, uint8_t * buffer, uint32_t numBytes) {
            // since flash is memory mapped via XIP, all we need to do is aggregate offset properly 
            memcpy(buffer, XIP_NOCACHE_NOALLOC_BASE + (&__cartridge_filesystem_start - XIP_BASE) + start, numBytes);
        }

        void cartridgeWrite(uint32_t start, uint8_t const * buffer, uint32_t numBytes) {
            ASSERT(start < cartridgeCapacityBytes());
            ASSERT(start + FLASH_PAGE_SIZE <= cartridgeCapacityBytes());
            uint32_t offset = reinterpret_cast<uint32_t>(& __cartridge_filesystem_start) - XIP_BASE + start;
            LOG(LL_LFS, "flash_range_program(" << offset << ", " << (uint32_t)FLASH_PAGE_SIZE << ") - start " << start);
            uint32_t ints = save_and_disable_interrupts();
            flash_range_program(offset, buffer, FLASH_PAGE_SIZE);
            restore_interrupts(ints);
        }

        void cartridgeErase(uint32_t start) {
            ASSERT(start < cartridgeCapacityBytes());
            ASSERT(start + FLASH_SECTOR_SIZE <= cartridgeCapacityBytes());
            uint32_t offset = reinterpret_cast<uint32_t>(& __cartridge_filesystem_start) - XIP_BASE + start;
            //TRACE_LITTLEFS("cart_fs_start: " << (uint32_t)(& __cartridge_filesystem_start));         
            //TRACE_LITTLEFS("XIP_BASE:      " << (uint32_t)(XIP_BASE));
            LOG(LL_LFS, "flash_range_erase(" << offset << ", " << (uint32_t)FLASH_SECTOR_SIZE << ") -- start " << start);
            uint32_t ints = save_and_disable_interrupts();
            flash_range_erase(offset, FLASH_SECTOR_SIZE);
            restore_interrupts(ints);
        }

    } // namespace rckid::hal::fs

    namespace memory {

        uint8_t * heapStart() {
            return & __bss_end__;
        }

        uint8_t * heapEnd() {
            return & __StackTop;
        }

        bool isImmutableDataPtr(void const * ptr) {
            uint32_t addr = reinterpret_cast<uint32_t>(ptr);
            return ((addr >= 0x10000000) && (addr < 0x20000000)) || (ptr == nullptr);
        }



    } // namespace rckid::hal::memory

} // namespace rckid::hal

