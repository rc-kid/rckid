/** \page backend_mk3 Mk III 

    Mark III, currently in development whose hardware specifications are still in progress.



    # I2C Communication

    Most of the I2C communication is done in async manner, where the RP can register bytes to send/receive via the I2C and will be notified by an interrupt when this is done. 

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

#include <platform/peripherals/ltr390uv.h>

#include "screen/ST7789.h"
#include "sd/sd.h"
#include "rckid/rckid.h"

#include "rckid/rckid.h"
#include "avr/src/avr_commands.h"
#include "avr/src/avr_state.h"

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

    // various forward declarations

    NORETURN(void bsod(uint32_t error, uint32_t line = 0, char const * file = nullptr));

    void memoryCheckStackProtection();

    namespace filesystem {
        void initialize();
    }

    // internal state

    namespace io {
        LTR390UV alsSensor_;

        uint16_t lightAls_ = 0;
        uint16_t lightUV_ = 0;
        AVRState avrState_;
        AVRState::Status lastStatus_;

    } // namespace rckid::io

    namespace i2c {
        class Packet {
        public:
            uint8_t address;
            uint8_t writeLen;
            uint8_t readLen;
            Packet * next = nullptr;
            void (* callback)(uint8_t) = nullptr;

            Packet(uint8_t addr, uint8_t wlen, uint8_t const * wdata, uint8_t rlen, void (* cb)(uint8_t) = nullptr):
                address{addr}, 
                writeLen{wlen}, 
                readLen{rlen}, 
                callback{cb} {
                if (writeLen <= 4) {
                    uint8_t * x = writeData();
                    for (size_t i = 0; i < writeLen; ++i) {
                        x[i] = wdata[i];
                    }
                } else {
                    writeData_ = new uint8_t[writeLen];
                    memcpy(writeData_, wdata, writeLen);
                }
            }

            ~Packet() {
                if (writeLen > 4)
                    delete [] writeData_;
            }

            void transmit() {
                i2c0->hw->enable = 0;
                i2c0->hw->tar = address;
                i2c0->hw->enable = 1;
                uint8_t * x = writeData();
                for (size_t i = 0; i < writeLen; ++i)
                    i2c0->hw->data_cmd = x[i] | (readLen == 0) && i == writeLen - 1 ? I2C_IC_DATA_CMD_STOP_BITS : 0;
                for (size_t i = 0; i < readLen; ++i)
                    i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | (i == 0 && writeLen != 0) ? I2C_IC_DATA_CMD_RESTART_BITS : 0;
                i2c0->hw->rx_tl = readLen == 0 ? 0 : readLen - 1;
            }

            uint8_t * writeData() {
                if (writeLen <= 4) {
                    return reinterpret_cast<uint8_t *>(&writeData_);
                } else {
                    return writeData_;
                }
            }

        private:
            uint8_t * writeData_;
        }; // i2c::Packet

        Packet * currentPacket = nullptr;
        Packet * lastPacket = nullptr;

        void enqueue(Packet * p) {
            if (lastPacket == nullptr) {
                // TODO no ISR
                currentPacket = p;
                lastPacket = p;
                currentPacket->transmit();
            } else {
                // TODO no ISR
                lastPacket->next = p;
                lastPacket = p;
            }
        }

        template<typename T>
        void sendCommand(T const & cmd) {
            Packet * p = new Packet(
                RCKID_AVR_I2C_ADDRESS, 
                sizeof(T), 
                reinterpret_cast<uint8_t const *>(& cmd), 
                0
            );
            enqueue(p);
        }

    } // namespace rckid::i2c

    bool buttonState(Btn b, AVRState::Status & status) {
        // TODO this can be made more efficient if we just get the value in the status and or the bits 
        switch (b) {
            case Btn::Up: return status.btnUp();
            case Btn::Down: return status.btnDown();
            case Btn::Left: return status.btnLeft();
            case Btn::Right: return status.btnRight();
            case Btn::A: return status.btnA();
            case Btn::B: return status.btnB();
            case Btn::Select: return status.btnSelect();
            case Btn::Start: return status.btnStart();
            case Btn::VolumeUp: return status.btnVolumeUp();
            case Btn::VolumeDown: return status.btnVolumeDown();
            case Btn::Home: return status.btnHome();
            default:
                UNREACHABLE;
        }
    }

    /** Updates the AVR status based on the status information received in the I2C buffer (callback function for getting AVR status). 
     */
    void updateAvrStatus([[maybe_unused]] uint8_t numBytes) {
        AVRState::Status status;
        uint8_t * raw = reinterpret_cast<uint8_t*>(&status);
        for (unsigned i = 0; i < sizeof(AVRState::Status); ++i) {
            raw[i] = i2c0->hw->data_cmd;;
        }
        // archive the old status
        io::lastStatus_ = io::avrState_.status;
        // copy the new one, we take the buttons (first 2 bytes as is) and or the interrupts to make sure none is ever lost, the process them immediately
        io::avrState_.status.updateWith(status);
        // if second tick interrupt is on, we must advance our timekeeping. Note that it is remotely possible that we will get an extra second tick interrupt when synchronizing the clock (we'll transmit the updated value, as well as the second tick at the same time) so that time on RP can be one second off at worst. 
        if (status.secondInt()) {
            io::avrState_.time.secondTick();
        }
        // TODO alarm
        // TODO pwr
        // TODO accel
        // finally clear the interrupts once we have processed them
        io::avrState_.status.clearInterrupts();
    }

    /** Requests AVR status update. 
     
        Enqueues AVR status read packet with the updateAVRStatus as callback function. 
     */
    void requestAvrStatus() {
        i2c::enqueue(new i2c::Packet(
            RCKID_AVR_I2C_ADDRESS, 
            0, 
            nullptr, 
            sizeof(AVRState::Status), 
            updateAvrStatus
        ));
    }
    
    void __not_in_flash_func(irqI2CDone_)() {
        uint32_t cause = i2c0->hw->intr_stat;
        i2c0->hw->clr_intr;
        // remove the packet from the queue and start transmitting the next one, if any
        i2c::Packet * p = i2c::currentPacket;
        if (p->next != nullptr) {
            i2c::currentPacket = p->next;
            i2c::currentPacket->transmit();
        } else {
            i2c::currentPacket = nullptr;
            i2c::lastPacket = nullptr;
        }
        // call the callback
        if (p->callback != nullptr)
            // can we get the real number of bytes read somehow if there was an error and the number is smaller?
            p->callback(p->readLen);
        delete p;
    }

    void __not_in_flash_func(irqDMADone_)() {
        //gpio::outputHigh(GPIO21);
        //unsigned irqs = dma_hw->ints0;
        //dma_hw->ints0 = irqs;
    }


    // sdk functions 

    void fatalError(uint32_t error, uint32_t line, char const * file) {
        // simply go top BSOD - no need for HW cleanup
        bsod(error, line, file);
    }

    Writer debugWrite() {
        return Writer{[](char x) {
#if (RCKID_LOG_TO_SERIAL == 1)
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
        board_init();
#if (RCKID_ENABLE_STACK_PROTECTION == 1)
        memoryInstrumentStackProtection();
#endif
        // initialize the USB
        // TODO verify in mkIII that initializing the USB does not interfere with detection & stuff with the PMIC chip
        tud_init(BOARD_TUD_RHPORT);
#if (RCKID_LOG_TO_SERIAL == 1)
        // initialize uart0 on pins 16 & 17 as serial out
        uart_init(uart0, RCKID_SERIAL_SPEED);
        gpio_set_function(RCKID_LOG_SERIAL_TX_PIN, GPIO_FUNC_UART);
        gpio_set_function(RCKID_LOG_SERIAL_RX_PIN, GPIO_FUNC_UART);
#endif
        // initialize the I2C bus we use to talk to AVR & peripherals (unlike mkIII this is not expected to be user accessible)
        i2c_init(i2c0, RCKID_I2C_SPEED); 
        i2c0->hw->intr_mask = 0; // disable interrupts for now
        gpio_set_function(RP_PIN_I2C_SDA, GPIO_FUNC_I2C);
        gpio_set_function(RP_PIN_I2C_SCL, GPIO_FUNC_I2C);
        // set the single DMA IRQ 0 handler reserved for the SDK
        irq_set_exclusive_handler(DMA_IRQ_0, irqDMADone_);
        irq_set_exclusive_handler(I2C0_IRQ, irqI2CDone_);
        //irq_set_exclusive_handler(TIMER_IRQ_0, irqBSOD_);
        irq_set_enabled(I2C0_IRQ, true);
        // make the I2C IRQ priority larger than that of the DMA (0x80) to ensure that I2C comms do not have to wait for render done if preparing data takes longer than sending them
        irq_set_priority(I2C0_IRQ, 0x40); 

        // initialize the display
        ST7789::initialize();

        // initialize the accelerometer & uv light sensor
        // TODO
        //io::accelerometer_.initialize();
        io::alsSensor_.initialize();
        io::alsSensor_.startALS();

        // initialize audio
        // TODO
        //audio::dma0_ = dma_claim_unused_channel(true);
        //audio::dma1_ = dma_claim_unused_channel(true);

        // initialize the SD card
        sdInitialize();

        // initialize the filesystem
        filesystem::initialize();

        // enter base arena for the application
        //Arena::enter();

        // read the full AVR state (including time information). Do not process the interrupts here, but wait for the first tick, which will or them with the ones obtained here and process when the device is fully initialized
        i2c_read_blocking(i2c0, RCKID_AVR_I2C_ADDRESS, (uint8_t *) & io::avrState_, sizeof(AVRState), false);
        
        // set brightness to the last used value
        // or at least 16 so that we can see stuff
        if (io::avrState_.brightness < 16)
            io::avrState_.brightness = 16;
        displaySetBrightness(io::avrState_.brightness);

#if (defined RCKID_WAIT_FOR_SERIAL)
        char cmd_ = ' ';
        while (tud_cdc_read(& cmd_, 1) != 1) { yield(); };
        LOG(LL_DEBUG, "Received command " << cmd_);
#endif

        // configure the AVR interrupt pin as input pullup (it's pulled down by the AVR when needed, floating otherwise) and connect an interrupt callback on the falling edge
        gpio::setAsInputPullup(RP_PIN_AVR_INT);
        gpio_set_irq_enabled_with_callback(RP_PIN_AVR_INT, GPIO_IRQ_EDGE_FALL, true, [](uint gpio, uint32_t events) {
            if (gpio == RP_PIN_AVR_INT)
                requestAvrStatus();
        });
        // TODO radio interrupt
        // TODO audio interrupt
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
        return buttonState(b, io::avrState_.status);
    }

    bool btnPressed(Btn b) {
        return buttonState(b, io::avrState_.status) && ! buttonState(b, io::lastStatus_);
    }

    bool btnReleased(Btn b) {
        return ! buttonState(b, io::avrState_.status) && buttonState(b, io::lastStatus_);
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
        return io::lightAls_;
    }
    
    uint16_t lightUV() {
        return io::lightUV_;
    }

    // display

    DisplayRefreshDirection displayRefreshDirection() {
        return ST7789::refreshDirection();
    }

    void displaySetRefreshDirection(DisplayRefreshDirection value) {
        ST7789::setRefreshDirection(value);
    }

    uint8_t displayBrightness() {
        return io::avrState_.brightness;
    }

    void displaySetBrightness(uint8_t value) {
        i2c::sendCommand(cmd::SetBrightness{value});
        // we set the brightness here as it is a simple change outside of the small state we get on interrupts 
        io::avrState_.brightness = value;
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
       i2c::sendCommand(cmd::Rumbler{effect});
    }

    // rgb

    void rgbEffect(uint8_t rgb, RGBEffect const & effect) {
        i2c::sendCommand(cmd::SetRGBEffect{rgb, effect});
    }
    
    void rgbEffects(RGBEffect const & a, RGBEffect const & b, RGBEffect const & dpad, RGBEffect const & sel, RGBEffect const & start) {
        i2c::sendCommand(cmd::SetRGBEffects{a, b, dpad, sel, start});
    }
    
    void rgbOff() {
        i2c::sendCommand(cmd::RGBOff{});
    }

}