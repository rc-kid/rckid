
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

#include <platform/peripherals/lsm6dsv.h>
#include <platform/peripherals/ltr390uv.h>

#include <rckid/hal.h>
#include <rckid/rckid.h>
#include <rckid/error.h>
#include <rckid/memory.h>
#include <rckid/graphics/color.h>

#include <rckid/apps/splashscreen.h>

#include "i2c.h"
#include "tusb_config.h"
#include "tusb.h"
#include "sd/sd.h"

#include "screen/ST7789.h"
#include "ST7789_rgb16.pio.h"

#include "audio/codec.h"

#include "avr/src/avr-commands.h"

namespace rckid::fs {
    void initializeFilesystem();
}

namespace rckid::internal {

    namespace device {
        // set to true once we have debugging cpability (otherwise if there were debugging prints in static initializations, they crash the device)
        volatile bool debugReady = false;

        PowerMode powerMode = PowerMode::Normal;

    }

    namespace io {

        DeviceState state;

        // accelerometer
        LSM6DSV accel;
        LSM6DSV::Orientation3D accelState;
        volatile uint32_t pedometerCount;

        LTR390UV light;

        void initialize() {
            // read the full state first
            TransferrableState ts;
            int n = i2c_read_blocking(i2c0, RCKID_AVR_I2C_ADDRESS, (uint8_t *) & ts, TS_SIZE, false);
            if (n != TS_SIZE)
                FATAL_ERROR("AVR READ", n);
            if (ts.version != RCKID_AVR_FIRMWARE_VERSION)
                FATAL_ERROR("AVR VERSION", ts.version);
            // the data received is correct, initialize own structures
            state = ts.state;
            LOG(LL_INFO, "AVR version:    " << ts.version);
            LOG(LL_INFO, "Device uptime:  " << ts.uptime);
            LOG(LL_INFO, "Wakeup reason:  " << ts.wakeupReason);
            LOG(LL_INFO, "Wakeup counter: " << ts.wakeupCounter);
            LOG(LL_INFO, "AVR temp:       " << ts.temp);
            
            // initialize I2C devices
            LOG(LL_INFO, "Detecting I2C devices...");
            LOG(LL_INFO, "  AVR (0x43):        " << (::i2c::isPresent(RCKID_AVR_I2C_ADDRESS) ? "ok" : "not found"));
            LOG(LL_INFO, "  LSM6DSV (0x6a):    " << (::i2c::isPresent(0x6a) ? "ok" : "not found"));
            LOG(LL_INFO, "  NAU88C22YG (0x1a): " << (::i2c::isPresent(RCKID_AUDIO_CODEC_I2C_ADDRESS) ? "ok" : "not found"));
            LOG(LL_INFO, "  LTR390UV (0x53):   " << (::i2c::isPresent(0x53) ? "ok" : "not found"));

            // initialize the accelerometer. This is more complex as we only want to initialize the accelerometer when not initialized already as the initialization also resets the pedometer count
            if (io::accel.isAccelerometerEnabled()) {
                LOG(LL_INFO, "Accelerometer already enabled, skipping initialization");
                if (! io::accel.isPedometerEnabled())
                    LOG(LL_ERROR, "Pedometer not enabled");
            } else if (! io::accel.initialize()) {
                LOG(LL_ERROR, "Failed to initialize LSM6DSV accelerometer");
            } else {
                LOG(LL_INFO, "LSM6DSV accelerometer initialized");
                io::accel.enableAccelerometer(true);
                io::accel.enablePedometer(true);
            }
            io::pedometerCount = io::accel.readStepCount();
            io::accelState = io::accel.readAccelerometerRaw();
            // fix the X axis orientation
            io::accelState.x *= -1;

            // initialize the LTR390UV light sensor
            // TODO

            // initialize the audio codec
            // TODO



            
            // TODO
        }

        void updateAvrStatus(int32_t numBytes) {
            if (numBytes != sizeof(DeviceState)) {
                LOG(LL_ERROR, "Corrupted AVR state: " << numBytes);
                return;
            }
            DeviceState ds;
            i2c::getTransactionResponse(reinterpret_cast<uint8_t*>(&ds), sizeof(DeviceState));
            state.updateWith(ds);
            // determine the headphones
            state.setHeadphonesConnected(gpio::read(RP_PIN_HEADSET_DETECT) == true);
        }

        void updateAccelStatus(int32_t numBytes) {
            if (numBytes != sizeof(LSM6DSV::Orientation3D)) {
                LOG(LL_ERROR, "Corrupted accel state: " << numBytes);
                return;
            }
            i2c::getTransactionResponse(reinterpret_cast<uint8_t*>(&accelState), sizeof(LSM6DSV::Orientation3D));
            // fix the X axis orientation
            accelState.x *= -1;
        }

    } // namespace rckid::internal::io

    /** Display Rendering
     
        
     */
    namespace display {
        hal::display::Callback cb;
        int32_t sm;
        int32_t pioOffset;
        uint32_t pixelsToWrite = 0;

        int32_t dmaChannel = -1;
        Color::RGB565 * buffer = nullptr;
        uint32_t bufferSize = 0;
        Color::RGB565 * backBuffer = nullptr;
        uint32_t backBufferSize = 0;

        void enterCommandMode() {
            if (pixelsToWrite > 0) {
                ASSERT(pio_sm_is_enabled(RCKID_ST7789_PIO, sm));
                // drop any ongoing transfer, this is fine as mostly 
                {
                    cpu::DisableInterruptsGuard g_;
                    dma_channel_set_irq0_enabled(dmaChannel, false);
                }
                dma_channel_abort(dmaChannel);
                // reset update counters & buffers
                pixelsToWrite = 0;
                buffer = nullptr;
                bufferSize = 0;
                backBuffer = nullptr;
                backBufferSize = 0;
            }
            pio_sm_set_enabled(RCKID_ST7789_PIO, sm, false);
            // initialize bitbanging driver and exit the RAMWR command
            ST7789::initializePinsBitBang();
            ST7789::leaveUpdateMode();
        }

        void enterUpdateMode() {
            ASSERT(! pio_sm_is_enabled(RCKID_ST7789_PIO, sm));
            // start the RAMWR command in bitbank mode
            ST7789::enterUpdateMode();
            // configure the DMA channel, but do not start it yet
            auto dmaConf = dma_channel_get_default_config(dmaChannel);
            channel_config_set_transfer_data_size(&dmaConf, DMA_SIZE_16); // transfer 16 bits at a time (single pixel)
            channel_config_set_read_increment(&dmaConf, true); // increment the read address (pixel buffer) after each transfer
            channel_config_set_write_increment(&dmaConf, false); // do not increment the write address (PIO FIFO)
            channel_config_set_dreq(&dmaConf, pio_get_dreq(RCKID_ST7789_PIO, sm, true));
            dma_channel_configure(dmaChannel, & dmaConf, &RCKID_ST7789_PIO->txf[sm], nullptr, 0, false);
            dma_channel_set_irq0_enabled(dmaChannel, true);
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
            dmaChannel = dma_claim_unused_channel(true);
            LOG(LL_INFO, "sm: " << sm);
            LOG(LL_INFO, "offset: " << pioOffset);
            LOG(LL_INFO, "dma: " << dmaChannel);
            // enter update mode for full screen 320x240 col-first (native) 
            enterUpdateMode();
        }

        void adjustSpeed() {
            pio_sm_set_clock_speed(RCKID_ST7789_PIO, sm, RCKID_ST7789_SPEED * 4);
        }
    }

    namespace audio {

        rckid::audio::Callback callback;

        struct DMA {
            int32_t channel = -1;
            int16_t * buffer = nullptr;
            uint32_t stereoSamples = 0;

            void stop() {
                dma_channel_abort(channel);
                buffer = nullptr;
                stereoSamples = 0;
            }

            void update() {
                if (Codec::isPlaybackI2S()) {
                    callback(buffer, stereoSamples);
                    if (buffer == nullptr) {
                        hal::audio::stop();
                    } else {
                        dma_channel_set_read_addr(channel, buffer, false);
                        dma_channel_set_trans_count(channel, stereoSamples, false);
                    }
                } else {
                    ASSERT(Codec::isRecordingI2S());
                    callback(buffer, stereoSamples);
                    dma_channel_set_write_addr(channel, buffer, false);
                    dma_channel_set_trans_count(channel, stereoSamples, false);
                }
            }

            void configurePlayback(DMA & other) {
                auto dmaConf = dma_channel_get_default_config(channel);
                channel_config_set_transfer_data_size(& dmaConf, DMA_SIZE_32); // transfer 32 bits (16 per channel, 2 channels)
                channel_config_set_read_increment(& dmaConf, true);  // increment on read
                channel_config_set_write_increment(& dmaConf, false);  // do not increment on write
                channel_config_set_dreq(&dmaConf, Codec::playbackDReq()); // DMA is driven by the I2S playback pio
                channel_config_set_chain_to(& dmaConf, other.channel); // chain to the other channel
                dma_channel_configure(channel, & dmaConf, Codec::playbackTxFifo(), buffer, stereoSamples, false); // the buffer consists of stereo samples, (32bits)
                // enable IRQ0 on the DMA channel (shared with other framework DMA uses such as the display or the SD card)
                dma_channel_set_irq0_enabled(channel, true);
            }

            void configureRecord(DMA & other) {
                // TODO this is weird and should be checked before recording will be tested (!)
                auto dmaConf = dma_channel_get_default_config(channel);
                channel_config_set_transfer_data_size(& dmaConf, DMA_SIZE_32); // transfer 32 bits (16 per channel, 2 channels)
                channel_config_set_read_increment(& dmaConf, false);  // do not increment on read
                channel_config_set_write_increment(& dmaConf, true);  // increment on write
                channel_config_set_dreq(&dmaConf, Codec::recordDReq()); // DMA is driven by the I2S record pio
                channel_config_set_chain_to(& dmaConf, other.channel); // chain to the other channel
                dma_channel_configure(channel, & dmaConf, buffer, Codec::recordRxFifo(), stereoSamples, false); // the buffer consists of stereo samples, (32bits), i.e. buffer size / 2
                // enable IRQ0 on the DMA channel (shared with other framework DMA uses such as the display or the SD card)
                dma_channel_set_irq0_enabled(channel, true);
            }

        };


        DMA dma0;
        DMA dma1;

        void initialize() {
            LOG(LL_INFO, "Initializing audio codec");
            dma0.channel = dma_claim_unused_channel(true);
            dma1.channel = dma_claim_unused_channel(true);
            LOG(LL_INFO, "  audio dma0: " << static_cast<uint32_t>(audio::dma0.channel));
            LOG(LL_INFO, "  audio dma1: " << static_cast<uint32_t>(audio::dma1.channel));
            // initialize the codec firmaware
            Codec::initialize();
            Codec::reset();
            Codec::powerUp();
            // initialize headphones pin as input, no pullups
            gpio::setAsInput(RP_PIN_HEADSET_DETECT);
       }

    }

    namespace fs {

    }

    void __not_in_flash_func(irqDMADone)() {
        unsigned irqs = dma_hw->ints0;
        dma_hw->ints0 = irqs;
        if (irqs & (1u << display::dmaChannel)) {
            display::pixelsToWrite -= display::bufferSize;
            if (display::pixelsToWrite <= 0) {
                display::buffer = nullptr;
                display::bufferSize = 0;
                display::backBuffer = nullptr;
                display::backBufferSize = 0;                
            } else {
                std::swap(display::buffer, display::backBuffer);
                std::swap(display::bufferSize, display::backBufferSize);
                ASSERT(display::buffer != nullptr);
                dma_channel_set_read_addr(display::dmaChannel, display::buffer, false);
                dma_channel_set_transfer_count(display::dmaChannel, display::bufferSize, true);
                if (display::pixelsToWrite > display::bufferSize)
                    display::cb(display::backBuffer, display::backBufferSize);
            }
        }
        // audio
        if (irqs & (1u << audio::dma0.channel)) {
            audio::dma0.update();
        }
        if (irqs & (1u << audio::dma1.channel)) {
            audio::dma1.update();
        }
    }

    void initializeDrivers() {
        LOG(LL_INFO, "Initializing drivers");

        internal::audio::initialize();

        // initialize the SD card, if present and the filesystem module
        internal::sd::initialize();
        internal::sd::initializeCard();
        rckid::fs::initializeFilesystem();






        // old not sure if we need in this reqrite

        // enable GPIO IRQ
        //irq_set_enabled(IO_IRQ_BANK0, true);


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


namespace rckid::hal {

    namespace device {

        void initialize() {
            // first initailize the board and USB stack / debugging for some basic output capability
            board_init();
            tud_init(BOARD_TUD_RHPORT);
#if (RCKID_LOG_TO_SERIAL == 1)
            // initialize only TX out on GPIO2
            uart_init(uart0, RCKID_RP_SERIAL_SPEED);
            gpio_set_function(RP_PIN_RP_TX, GPIO_FUNC_UART_AUX);
#endif
            internal::device::debugReady = true;

            // set DMA IRQ0 as exclusive for the SDK (shared between display, audio, etc.), enable its IRQ and set the handler
            irq_set_exclusive_handler(DMA_IRQ_0, internal::irqDMADone);
            // enable DMA IRQ (used by display, audio, etc.)
            irq_set_enabled(DMA_IRQ_0, true);

            // enable the screen
            internal::display::initialize();
            // enable the async I2C driver 
            i2c::initialize();
            // initalize the IO module (talk to the AVR)
            internal::io::initialize();

            App::run<SplashScreen>(internal::initializeDrivers);

            // reset the background now that we have initialized the card, etc
            // TODO this is likely too hacky and should be automatic-ish
            ui::RootWidget::releaseResources();
            ui::Style::clearDefaultStyle();
            LOG(LL_INFO, "Init done");
        }

        void setPowerMode(PowerMode mode) {
            if (internal::device::powerMode == mode)
                return;
            // make sure all peripherals that are speed dependent are not actively used
            rckid::display::waitUpdateDone();
            // switch the clock speed
            switch (mode) {
                case PowerMode::Normal:
                    if (! set_sys_clock_khz(150000, true))
                        return;
                    break;
                case PowerMode::Boost:
                    if (! set_sys_clock_khz(250000, true))
                        return;
                    break;
                default:
                    UNREACHABLE;
            }
            // set mode
            internal::device::powerMode = mode;
            // readjust clock rate for HW systems that require it
            Codec::adjustSpeed();
            internal::display::adjustSpeed();
            internal::sd::adjustSpeed();
        }

        void setDebugMode(bool value) {
            i2c::sendAvrCommand(cmd::SetDebugMode{value});
            if (value == false)
                i2c::sendAvrCommand(cmd::SetUartDebug{false});
            internal::io::state.setDebugMode(value);
        }

        /** Sends the immediate power off to AVR every 50ms waiting in between. This is to ensure that if the command is lost, it will be re-issued.
         */
        void powerOff() {
            while (true) {
                i2c::sendAvrCommand(cmd::PowerOff{});
                for (uint32_t i = 0; i < 50; ++i) {
                    yield();
                    cpu::delayMs(1);
                }
            }
        }

        void sleep() {
            UNIMPLEMENTED;
        }

        void scheduleWakeup(uint32_t timeoutSeconds, uint32_t payload) {
            UNIMPLEMENTED;
        }

        void onTick() {
            onYield();
            // TODO add stuff for checking hardware events in the latest state? 


            // enqueue avr, accel and light sensor status updates
            i2c::transmitAsync(RCKID_AVR_I2C_ADDRESS, nullptr, 0, sizeof(DeviceState), internal::io::updateAvrStatus);
            // accel
            uint8_t cmd = LSM6DSV::REG_OUTX_L_A; 
            i2c::transmitAsync(LSM6DSV::I2C_ADDRESS, & cmd, 1, sizeof(LSM6DSV::Orientation3D), internal::io::updateAccelStatus);
            // light
            // TODO
        }

        void onYield() {
            tight_loop_contents();
            tud_task();
            i2c::onYield();
        }

        void fatalError(char const * file, uint32_t line, char const * msg, uint32_t payload) {
            LOG(LL_ERROR, "FATAL: " << file << ":" << line);
            LOG(LL_ERROR, msg << "(payload " << payload << ")");
            LOG(LL_ERROR, internal::display::pixelsToWrite);
            LOG(LL_ERROR, dma_channel_is_busy(internal::display::dmaChannel));
            LOG(LL_ERROR, pio_sm_is_stalled(RCKID_ST7789_PIO, internal::display::sm));
            LOG(LL_ERROR, internal::display::buffer << " - " << internal::display::bufferSize);
            LOG(LL_ERROR, internal::display::backBuffer << " - " << internal::display::backBufferSize);
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
            TinyDateTime result;
            i2c::enqueue([& result](int32_t){
                TransferrableState ts;
                int n = i2c_read_blocking(i2c0, RCKID_AVR_I2C_ADDRESS, (uint8_t *) & ts, TS_SIZE, false);
                if (n != TS_SIZE)
                    FATAL_ERROR("AVR READ", n);
                if (ts.version != RCKID_AVR_FIRMWARE_VERSION)
                    FATAL_ERROR("AVR VERSION", ts.version);
                result = ts.time;
            });
            return result;
        }

        void setTime(TinyDateTime dt) {
            i2c::sendAvrCommand(cmd::SetTime{dt});
        }

    } // namespace rckid::hal::time

    namespace io {

        DeviceState state() {
            DeviceState result = internal::io::state;
            internal::io::state.clearInterrupts();
            // acknowledge the power off iterrupt when the state has been requested. We do this on state request, not state read, to ensure that it's reset only when the user code running, not the core driver is inspecting the state
            if (result.powerOffInterrupt())
                i2c::sendAvrCommand(cmd::PowerOffAck());
            return result;
        }

        Point3D accelerometerState() {
            using namespace internal::io;
            return Point3D{accelState.x, accelState.y, accelState.z};
        }

        Point3D gyroscopeState() {
            UNREACHABLE;
        }

    } // namespace rckid::hal::io

    namespace display {

        void enable(Rect rect, RefreshDirection direction) {
            LOG(LL_INFO, "Display reset " << rect);
            rckid::display::waitUpdateDone();
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
            i2c::sendAvrCommand(cmd::SetBrightness{value});
        }

        bool vSync() {
            // vSync is active when the TE pin is high
            return gpio_get(RP_PIN_DISP_TE) == 1;
        }

        void update(Callback callback) {
            using namespace internal::display;
            rckid::display::waitUpdateDone();
            ASSERT(! updateActive());
            ASSERT(buffer == nullptr);
            ASSERT(backBuffer == nullptr);
            cb = callback;
            pixelsToWrite = ST7789::updateRegion().w * ST7789::updateRegion().h;
            cb(buffer, bufferSize);
            if (pixelsToWrite > bufferSize)
                cb(backBuffer, backBufferSize);
            dma_channel_set_read_addr(dmaChannel, buffer, false);
            dma_channel_set_transfer_count(dmaChannel, bufferSize, true);
        }

        void update(Color::RGB565 const * buffer, uint32_t bufferSize) {
            using namespace internal::display;
            rckid::display::waitUpdateDone();
            ASSERT(! updateActive());
            ASSERT(internal::display::buffer == nullptr);
            ASSERT(internal::display::backBuffer == nullptr);
            cb = nullptr;
            internal::display::buffer = const_cast<Color::RGB565 *>(buffer);
            internal::display::bufferSize = bufferSize;
            pixelsToWrite = bufferSize;
            dma_channel_set_read_addr(dmaChannel, buffer, false);
            dma_channel_set_transfer_count(dmaChannel, bufferSize, true);
        }

        void update(Color::RGB565 const * buffer1, uint32_t bufferSize1, Color::RGB565 const * buffer2, uint32_t bufferSize2) {
            using namespace internal::display;
            rckid::display::waitUpdateDone();
            ASSERT(! updateActive());
            ASSERT(buffer == nullptr);
            ASSERT(backBuffer == nullptr);
            cb = nullptr;
            internal::display::buffer = const_cast<Color::RGB565 *>(buffer1);
            internal::display::bufferSize = bufferSize1;
            internal::display::backBuffer = const_cast<Color::RGB565 *>(buffer2);
            internal::display::backBufferSize = bufferSize2;
            pixelsToWrite = bufferSize1 + bufferSize2;
            dma_channel_set_read_addr(dmaChannel, internal::display::buffer, false);
            dma_channel_set_transfer_count(dmaChannel, internal::display::bufferSize, true);
        }

        bool updateActive() {
            using namespace internal::display;
            return pixelsToWrite > 0;
        }

    } // namespace rckid::hal::display

    namespace audio {

        void setVolumeHeadphones(uint8_t value) {
            if (value > 15)
                value = 15;
            Codec::setVolumeHeadphones((value << 2) | (value & 3));
        }

        void setVolumeSpeaker(uint8_t value) {
            if (value > 15)
                value = 15;
            Codec::setVolumeSpeaker((value << 2) | (value & 3));
        }

        void play(uint32_t sampleRate, Callback cb) {
            using namespace internal::audio;
            stop(); 
            callback = std::move(cb);
            // refill the buffers
            callback(dma0.buffer, dma0.stereoSamples);
            callback(dma1.buffer, dma1.stereoSamples);
            // configure the DMA
            dma0.configurePlayback(dma1);
            dma1.configurePlayback(dma0);
            // start the playback on the codec
            Codec::playbackI2S(sampleRate);
            // and start the first DMA
            dma_channel_start(dma0.channel);
        }

        void recordMic(uint32_t sampleRate, Callback cb) {
            using namespace internal::audio;
            stop();
            callback = std::move(cb);
            // get buffers from the callback (those are empty now)
            callback(dma0.buffer, dma0.stereoSamples);
            callback(dma1.buffer, dma1.stereoSamples);
            // configure the DMA
            dma0.configureRecord(dma1);
            dma1.configureRecord(dma0);
            // enable the first DMA
            dma_channel_start(dma0.channel);
            // instruct the codec to start the playback at given sample rate
            Codec::recordMic(sampleRate);
        }

        void recordLineIn(uint32_t sampleRate, Callback cb) {
            using namespace internal::audio;
            stop();
            callback = std::move(cb);
            // get buffers from the callback (those are empty now)
            callback(dma0.buffer, dma0.stereoSamples);
            callback(dma1.buffer, dma1.stereoSamples);
            // configure the DMA
            dma0.configureRecord(dma1);
            dma1.configureRecord(dma0);
            // enable the audio path from cartridge to the codec line-in
            UNIMPLEMENTED;
            // enable the first DMA
            dma_channel_start(dma0.channel);
            // instruct the codec to start the playback at given sample rate
            Codec::recordLineIn(sampleRate);
        }   

        void pause() {
            Codec::pause();
        }

        void resume() {
            Codec::resume();
        }

        void stop() {
            Codec::stop();
            internal::audio::dma0.stop();
            internal::audio::dma1.stop();
        }

        bool isPlaying() {
            return Codec::isPlaybackI2S();
        }

        bool isRecording() {
            return Codec::isRecordingI2S();
        }

        bool isPaused() {
            return Codec::isPaused();
        }


    } // namespace rckid::hal::audio

    namespace fs {

        bool sdCardDetect() {
            return ! gpio::read(RP_PIN_SD_CD);
        }

        uint32_t sdCapacityBlocks() {
            return internal::sd::sdNumBlocks_;
        }

        void sdReadBlocks(uint32_t blockNum, uint8_t * buffer, uint32_t numBlocks) {
            internal::sd::readBlocks(blockNum, buffer, numBlocks);
        }

        void sdWriteBlocks(uint32_t blockNum, uint8_t const * buffer, uint32_t numBlocks) {
            internal::sd::writeBlocks(blockNum, buffer, numBlocks);
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

    namespace storage {

        /** Saves the provided bytes to AVR memory.
         
            The buffer is chunked into multiple write message bytes
         */
        void save(uint16_t start, uint8_t const * buffer, uint32_t numBytes) {
            while (numBytes > 0) {
                uint32_t tx = numBytes < 12 ? numBytes : 12;
                i2c::sendAvrCommand(cmd::WriteStorage{start, buffer, tx});
                numBytes -= tx;
                start += tx;
                buffer += tx;
            }
        }

        /** Reads the AVR storage in one read. We use the write command write followed by I2C restart read.
         */
        void load(uint16_t start, uint8_t * buffer, uint32_t numBytes) {
            // enqueue with callback and use first address set, then wait enough time for the command to finish and then perform the actual read
            i2c::enqueue([start, buffer, numBytes](int32_t){
                cmd::ReadStorage cmd{start};
                i2c_write_blocking(i2c0, RCKID_AVR_I2C_ADDRESS, reinterpret_cast<uint8_t const *>(& cmd), sizeof(cmd), false);
                cpu::delayMs(5);
                i2c_read_blocking(i2c0, RCKID_AVR_I2C_ADDRESS, buffer, numBytes, false);
            });
        }
    } // namespace rckid::hal::storage

    namespace rumbler {
        void setEffect(RumblerEffect const & effect) {
            i2c::sendAvrCommand(cmd::SetRumblerEffect{effect});
        }
    } // namespace rckid::hal::rumbler

    namespace rgb {
        void setEffect(uint8_t index, RGBEffect const & effect) {
            i2c::sendAvrCommand(cmd::SetRGBEffect{index, effect});
        }

        void setEffectAll(RGBEffect const & effect) {
            i2c::sendAvrCommand(cmd::SetRGBEffectAll{effect});
        }
    } // namespace rckid::hal::rgb

} // namespace rckid::hal

