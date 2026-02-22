
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
#include "ST7789_rgb16.pio.h"

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
        hal::display::Callback cb;
        int32_t sm;
        int32_t pioOffset;
        int32_t pixelsToWrite = 0;


        struct DMA {
            int32_t channel = -1;
            Color::RGB565 * buffer = nullptr;
            uint32_t bufferSize = 0;

            void reset() {
                buffer = nullptr;
                bufferSize = 0;
            }

            void configure(DMA & other) {
                auto dmaConf = dma_channel_get_default_config(channel);
                channel_config_set_transfer_data_size(&dmaConf, DMA_SIZE_16); // transfer 16 bits at a time (single pixel)
                channel_config_set_read_increment(&dmaConf, true); // increment the read address (pixel buffer) after each transfer
                channel_config_set_write_increment(&dmaConf, false); // do not increment the write address (PIO FIFO)
                channel_config_set_chain_to(&dmaConf, other.channel);
                channel_config_set_dreq(&dmaConf, pio_get_dreq(RCKID_ST7789_PIO, sm, true));
                dma_channel_configure(channel, & dmaConf, &RCKID_ST7789_PIO->txf[sm], nullptr, 0, false);
                dma_channel_set_irq0_enabled(channel, true);
            }

            void update(DMA & other) {
                /*
                dma_channel_set_read_addr(channel, buffer, false);
                dma_channel_set_transfer_count(channel, bufferSize, false);
                dma_channel_set_chain_to(channel, other.channel);
                */
                auto dmaConf = dma_get_channel_config(channel);
                channel_config_set_chain_to(&dmaConf, (pixelsToWrite > 0) ? other.channel : channel);
                dma_channel_configure(channel, & dmaConf, &RCKID_ST7789_PIO->txf[sm], buffer, bufferSize, false);
            }

        };

        DMA dma1;
        DMA dma2;

        void enterCommandMode() {
            // drop any ongoing transfer
            if (pio_sm_is_enabled(RCKID_ST7789_PIO, sm)) {
                {
                    cpu::DisableInterruptsGuard g_;
                    dma_channel_set_irq0_enabled(dma1.channel, false);
                    dma_channel_set_irq0_enabled(dma2.channel, false);
                }
                dma_channel_abort(dma1.channel);
                dma_channel_abort(dma2.channel);
                pio_sm_set_enabled(RCKID_ST7789_PIO, sm, false);
            }
            // initialize bitbanging driver and exit the RAMWR command
            ST7789::initializePinsBitBang();
            ST7789::leaveUpdateMode();
        }

        void enterUpdateMode() {
            ASSERT(! pio_sm_is_enabled(RCKID_ST7789_PIO, sm));
            // start the RAMWR command in bitbank mode
            ST7789::enterUpdateMode();
            // configure the DMA channels, but do not start them
            dma1.configure(dma2);
            dma2.configure(dma1);
            // initialize the PIO program
            ST7789_rgb16_program_init(RCKID_ST7789_PIO, sm, pioOffset, RP_PIN_DISP_WRX, RP_PIN_DISP_DB15);
            // and start the pio, which will put it immediately into a stall mode on tx
            pio_sm_set_enabled(RCKID_ST7789_PIO, sm, true);
        }

        void initialize() {
            LOG(LL_INFO, "display::initialize");
            // reset the physical display
            ST7789::reset();
            pio_set_gpio_base(RCKID_ST7789_PIO, 16);
            sm = pio_claim_unused_sm(RCKID_ST7789_PIO, true);
            pioOffset = pio_add_program(RCKID_ST7789_PIO, & ST7789_rgb16_program);
            dma1.channel = dma_claim_unused_channel(true);
            dma2.channel = dma_claim_unused_channel(true);
            LOG(LL_INFO, "sm: " << sm);
            LOG(LL_INFO, "offset: " << pioOffset);
            LOG(LL_INFO, "dma1: " << dma1.channel);
            LOG(LL_INFO, "dma2: " << dma2.channel);
            // enter update mode for full screen 320x240 col-first (native) 
            enterUpdateMode();
        }
    }

    namespace audio {

        uint32_t sampleRate = 44100;

        struct DMA {
            int32_t channel = -1;
            int16_t * buffer = nullptr;
            uint32_t bufferSize = 0;

            void reset() {
                buffer = nullptr;
                bufferSize = 0;
            }

            void configure(DMA & other) {
            }

            void update(DMA & other) {
            }

        };

        DMA dma1;
        DMA dma2;

    }

    namespace fs {

    }

    void __not_in_flash_func(irqDMADone)() {
        unsigned irqs = dma_hw->ints0;
        dma_hw->ints0 = irqs;
        if (display::pixelsToWrite > 0) {
            if (irqs & (1u << display::dma1.channel)) {
                display::cb(display::dma1.buffer, display::dma1.bufferSize);
                display::pixelsToWrite -= display::dma1.bufferSize;
                display::dma1.update(display::dma2);
            }
            if (irqs & (1u << display::dma2.channel)) {
                display::cb(display::dma2.buffer, display::dma2.bufferSize);
                display::pixelsToWrite -= display::dma2.bufferSize;
                display::dma2.update(display::dma1);
            }
        }    
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
        cpu::DisableInterruptsGuard g_;
        void * result = rckid::Heap::alloc(numBytes);
        return result;
    }

    void __wrap_free(void * ptr) { 
        cpu::DisableInterruptsGuard g_;
        if (rckid::Heap::contains(ptr))
            rckid::Heap::free(ptr); 
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
            internal::display::initialize();








            // old not sure if we need in this reqrite

            // enable GPIO IRQ
            irq_set_enabled(IO_IRQ_BANK0, true);


            // TODO

            //rckid::fs::initializeFilesystem();
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
            onYield();
            // TODO add more
        }

        void onYield() {
            tight_loop_contents();
            tud_task();
        }

        void fatalError(char const * file, uint32_t line, char const * msg, uint32_t payload) {
            // fatal error is simple on fantasy console as we do not have to worry about weird hardware states
            // stop audio playback, which is the only async stuff we can have
            audio::stop();
            // reset display driver
            internal::display::enterCommandMode();
            ST7789::reset();
            internal::display::enterUpdateMode();
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
            return internal::io::state;
        }

        Point3D accelerometerState() {
            UNREACHABLE;
        }

        Point3D gyroscopeState() {
            UNREACHABLE;
        }

    } // namespace rckid::hal::io

    namespace display {

        void enable(Rect rect, RefreshDirection direction) {
            // cancel any ongoing DMA update
            internal::display::enterCommandMode();
            // leave the update mode so that we can send commands
            ST7789::setRefreshDirection(direction);
            ST7789::setUpdateRegion(rect);
            // and enter the update mode again
            internal::display::enterUpdateMode();
        }

        void disable() {
            // TODO
            UNIMPLEMENTED;
        }

        void setBrightness(uint8_t value) {
            // TODO
            UNIMPLEMENTED;
        }

        bool vSync() {
            // vSync is active when the TE pin is high
            return gpio_get(RP_PIN_DISP_TE) == 1;
        }

        void update(Callback callback) {
            using namespace internal::display;
            while (updateActive())
                yield();
            ASSERT(! updateActive());
            cb = callback;
            dma1.reset();
            dma2.reset();
            pixelsToWrite = ST7789::updateRegion().w * ST7789::updateRegion().h;
            LOG(LL_INFO, pixelsToWrite);
            cb(dma1.buffer, dma1.bufferSize);
            pixelsToWrite -= dma1.bufferSize;
            LOG(LL_INFO, pixelsToWrite);
            dma1.update(dma2);
            if (pixelsToWrite > 0) {
                LOG(LL_INFO, pixelsToWrite);
                cb(dma2.buffer, dma2.bufferSize);
                dma2.update(dma1);
                pixelsToWrite -= dma2.bufferSize;
            }
            dma_channel_start(dma1.channel);
        }

        bool updateActive() {
            using namespace internal::display;
            // if we have pixels to write, we better be active
            if (pixelsToWrite > 0)
                return true;
            // if any of the display DMAs are running, we are active
            if (dma_channel_is_busy(dma1.channel) || dma_channel_is_busy(dma2.channel))
                return true;
            // if the dma channels are idle, the sm must still be sending the last pixels
            return ! pio_sm_is_stalled(RCKID_ST7789_PIO, sm);
        }

    } // namespace rckid::hal::display

    namespace audio {

        void setVolumeHeadphones(uint8_t value) {
            UNIMPLEMENTED;

        }

        void setVolumeSpeaker(uint8_t value) {
            UNIMPLEMENTED;
        }

        void play(uint32_t sampleRate, Callback cb) {
            UNIMPLEMENTED;
        }

        void recordMic(uint32_t sampleRate, Callback cb) {
            UNIMPLEMENTED;
        }

        void recordLineIn(uint32_t sampleRate, Callback cb) {
            UNIMPLEMENTED;
        }   

        void pause() {
            UNIMPLEMENTED;
        }

        void resume() {
            UNIMPLEMENTED;
        }

        void stop() {
            UNIMPLEMENTED;
        }

        bool isPlaying() {
            UNIMPLEMENTED;
        }

        bool isRecording(){
            UNIMPLEMENTED;
        }

        bool isPaused() {
            UNIMPLEMENTED;
        }


    } // namespace rckid::hal::audio

    namespace fs {

        uint32_t sdCapacityBlocks() {
            UNIMPLEMENTED;
        }

        void sdReadBlocks(uint32_t blockNum, uint8_t * buffer, uint32_t numBlocks) {
            UNIMPLEMENTED;
        }

        void sdWriteBlocks(uint32_t blockNum, uint8_t const * buffer, uint32_t numBlocks) {
            UNIMPLEMENTED;
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
            {
                cpu::DisableInterruptsGuard g_;
                flash_range_program(offset, buffer, FLASH_PAGE_SIZE);
            }
        }

        void cartridgeErase(uint32_t start) {
            ASSERT(start < cartridgeCapacityBytes());
            ASSERT(start + FLASH_SECTOR_SIZE <= cartridgeCapacityBytes());
            uint32_t offset = reinterpret_cast<uint32_t>(& __cartridge_filesystem_start) - XIP_BASE + start;
            //TRACE_LITTLEFS("cart_fs_start: " << (uint32_t)(& __cartridge_filesystem_start));         
            //TRACE_LITTLEFS("XIP_BASE:      " << (uint32_t)(XIP_BASE));
            LOG(LL_LFS, "flash_range_erase(" << offset << ", " << (uint32_t)FLASH_SECTOR_SIZE << ") -- start " << start);
            {
                cpu::DisableInterruptsGuard g_;
                flash_range_erase(offset, FLASH_SECTOR_SIZE);
            }
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

