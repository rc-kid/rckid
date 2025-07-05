/** \page backend_mk3 Mk III 

    Mark III, currently in development whose hardware specifications are still in progress.



    # I2C Communication

    Most of the I2C communication is done in async manner, where the RP can register bytes to send/receive via the I2C and will be notified by an interrupt when this is done. 

*/

#ifndef RCKID_BACKEND_MK3
#error "You are building fantasy (RayLib) backend without the indicator macro"
#endif

#ifdef PICO_RP2350A
#error "Oh noez"
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
#include <platform/peripherals/si4705.h>
#include <platform/peripherals/tlv320.h>

#include <rckid/rckid.h>
#include <rckid/radio.h>
#include "screen/ST7789.h"
#include "sd/sd.h"
#include "i2c.h"
#include <rckid/app.h>
#include <rckid/filesystem.h>
#include <rckid/ui/header.h>

#include "i2s_out16.pio.h"

#include "avr/src/avr_commands.h"
#include "avr/src/avr_state.h"



extern "C" {
    extern uint8_t __cartridge_filesystem_start;
    extern uint8_t __cartridge_filesystem_end;

    // implement not really working entropy function to silence the linker warning
    int _getentropy([[maybe_unused]] void *buffer, [[maybe_unused]] size_t length) {
        errno = ENOSYS;
        return -1;
    }    

    void *__wrap_malloc(size_t numBytes) {
        auto x = save_and_disable_interrupts();
        void * result = rckid::RAMHeap::alloc(numBytes);
        restore_interrupts(x);
        return result;
    }

    void __wrap_free(void * ptr) { 
        auto x = save_and_disable_interrupts();
        if (rckid::RAMHeap::contains(ptr))
            rckid::RAMHeap::free(ptr); 
        restore_interrupts(x);
    }

    void *__wrap_calloc(size_t numBytes) {
        void * result = __wrap_malloc(numBytes);
        memset(result, 0, numBytes);
        return result;
    }
}

/** Static class that is friends with the internal state so it can modify it.
 */
class RCKid {
public:

    static void setButtonState(rckid::Btn b, rckid::AVRState::Status & status, bool value) {
        using namespace rckid;
        switch (b) {
            case Btn::Up: status.setDPadButtons(status.btnLeft(), status.btnRight(), value, status.btnDown()); return;
            case Btn::Down: status.setDPadButtons(status.btnLeft(), status.btnRight(), status.btnUp(), value); return;
            case Btn::Left: status.setDPadButtons(value, status.btnRight(), status.btnUp(), status.btnDown()); return;
            case Btn::Right: status.setDPadButtons(status.btnLeft(), value, status.btnUp(), status.btnDown()); return;
            case Btn::A: status.setABXYButtons(value, status.btnB(), status.btnSelect(), status.btnStart()); return;
            case Btn::B: status.setABXYButtons(status.btnA(), value, status.btnSelect(), status.btnStart()); return;
            case Btn::Select: status.setABXYButtons(status.btnA(), status.btnB(), value, status.btnStart()); return;
            case Btn::Start: status.setABXYButtons(status.btnA(), status.btnB(), status.btnSelect(), value); return;
            case Btn::VolumeUp: status.setControlButtons(status.btnHome(), value, status.btnVolumeDown()); return;
            case Btn::VolumeDown: status.setControlButtons(status.btnHome(), status.btnVolumeUp(), value); return;
            case Btn::Home: status.setControlButtons(value, status.btnVolumeUp(), status.btnVolumeDown()); return;
            default:
                UNREACHABLE;
        }
    }

}; // class RCKid

namespace rckid {

    void memoryReset();

    // various forward declarations

    NORETURN(void bsod(uint32_t error, uint32_t arg,  uint32_t line = 0, char const * file = nullptr));

    void audioPlaybackDMA(uint finished, uint other);

    namespace fs {
        void initialize();
    }

    // internal state

    namespace io {
        Si4705 radio_;
        LTR390UV alsSensor_;

        uint16_t lightAls_ = 0;
        uint16_t lightUV_ = 0;
        AVRState avrState_;
        AVRState::Status lastStatus_;

    } // namespace rckid::io

    namespace time {
        uint64_t nextSecond_ = 0;
        bool idle_ = true;
        uint32_t idleTimeout_ = RCKID_IDLE_TIMETOUT; 
        uint32_t idleTimeoutKeepalive_ = RCKID_IDLE_TIMETOUT_KEEPALIVE;
    }

    namespace audio {
        enum class State {
            Idle, 
            Playback, 
            PlaybackPaused,
            Recording,
            RecordingPaused,
        }; 
        State state_ = State::Idle;
        std::function<uint32_t(int16_t *, uint32_t)> cb_;
        uint8_t volume_ = 10;
        uint playbackSm_;
        uint playbackOffset_;
        uint dma0_ = 0;
        uint dma1_ = 0;
        DoubleBuffer<int16_t> * playbackBuffer_ = nullptr;
        uint32_t sampleRate_ = 44100;

    }

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
        // TODO what if numBytes != status read? 
        i2c::readResponse(reinterpret_cast<uint8_t*>(&status), sizeof(AVRState::Status));
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
        i2c::Packet * p = i2c::transmitNextPacket();
        // call the callback
        if (p->callback != nullptr)
            // can we get the real number of bytes read somehow if there was an error and the number is smaller?
            p->callback(p->readLen);
        delete p;
    }

    void __not_in_flash_func(irqDMADone_)() {
        //gpio::outputHigh(GPIO21);
        unsigned irqs = dma_hw->ints0;
        dma_hw->ints0 = irqs;
        // for audio, reset the DMA start address to the beginning of the buffer and tell the double buffer to refill
        if (audio::state_ == audio::State::Playback) {
            if (irqs & (1u << audio::dma0_))
                audioPlaybackDMA(audio::dma0_, audio::dma1_);
            if (irqs & (1u << audio::dma1_))
                audioPlaybackDMA(audio::dma1_, audio::dma0_);
        }

        // display DMA IRQ
        if (irqs & ( 1u << ST7789::dma_))
            ST7789::irqHandler();
    }

    void __not_in_flash_func(irqGPIO_)(uint pin, uint32_t events) {
        switch (pin) {
            case RP_PIN_AVR_INT:
                requestAvrStatus();
                break;
            default:
                LOG(LL_ERROR, "Unknown GPIO IRQ on pin " << (uint32_t)pin << " with events " << events);
                break;
        }
    }

    // sdk functions 

    void fatalError(uint32_t error, uint32_t arg, uint32_t line, char const * file) {
        // simply go top BSOD - no need for HW cleanup
        // TODO Really?
        // TODO memory reset 
        // TODO reset stack pointer as well 
        bsod(error, arg, line, file);
    }

    bool debugReady = false;

    Writer debugWrite() {
        return Writer{[](char x, void *) {
            if (debugReady == false)
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

    uint8_t debugRead(bool echo) {
        char cmd_ = ' ';
#if (RCKID_LOG_TO_SERIAL == 1)
        cmd_ = uart_getc(uart0);
#else 
        while (tud_cdc_read(& cmd_, 1) != 1) { yield(); };
        if (echo) {
            tud_cdc_write(& cmd_, 1);
            tud_cdc_write_flush();
        }
#endif
        return static_cast<uint8_t>(cmd_);
    }

    void initialize([[maybe_unused]] int argc, [[maybe_unused]] char const * argv[]) {
        board_init();
        // initialize the USB
        // TODO verify in mkIII that initializing the USB does not interfere with detection & stuff with the PMIC chip
        tud_init(BOARD_TUD_RHPORT);
#if (RCKID_LOG_TO_SERIAL == 1)
        // initialize uart0 on pins 16 & 17 as serial out
        uart_init(uart0, RCKID_SERIAL_SPEED);
        gpio_set_function(RCKID_LOG_SERIAL_TX_PIN, GPIO_FUNC_UART);
        gpio_set_function(RCKID_LOG_SERIAL_RX_PIN, GPIO_FUNC_UART);
#endif
        debugReady = true;
        // initialize the I2C bus we use to talk to AVR & peripherals (unlike mkII this is not expected to be user accessible)
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
        // enable DMA IRQ (used by display, audio, etc.)
        irq_set_enabled(DMA_IRQ_0, true);
        // enable GPIO IRQ
        irq_set_enabled(IO_IRQ_BANK0, true);


        LOG(LL_INFO, "\n\n\nSYSTEM RESET DETECTED (RP2350): ");
        LOG(LL_INFO, "RP2350 chip version: " << rp2350_chip_version());
        
        RAMHeap::traceChunks();

        // initialize the display
        ST7789::initialize();

        // initialize next second counter for on device time keeping. This is not 100% precise as we do not know the time at which the second should actually be updated, but will incur in at most 1s imprecision at each start. Those do not add up as AVR keeps track of the time correctly and will reset the RP clock again on next device start
        time::nextSecond_ = uptimeUs64() + 1000000;

#if (RCKID_WAIT_FOR_SERIAL == 1)
        // wait for input on the serial port before proceeding, so that we can debug the boot process
        debugRead(true);
#endif

        // check the I2C devices we expect to find on the bus
        LOG(LL_INFO, "Detecting I2C devices...");
        LOG(LL_INFO, "  AVR (0x43):        " << (::i2c::isPresent(0x43) ? "ok" : "not found"));
        LOG(LL_INFO, "  LTR390UV (0x53):   " << (::i2c::isPresent(0x53) ? "ok" : "not found"));
        LOG(LL_INFO, "  MPU6500 (0x68):    " << (::i2c::isPresent(0x68) ? "ok" : "not found"));
        LOG(LL_INFO, "  BQ25895 (0x6a):    " << (::i2c::isPresent(0x6a) ? "ok" : "not found"));
        LOG(LL_INFO, "  TLV320 (0x18):     " << (::i2c::isPresent(0x18) ? "ok" : "not found"));
        LOG(LL_INFO, "  SI4705 (0x11):     " << (::i2c::isPresent(0x11) ? "ok" : "not found"));

        // initialize the interrupt pins and set the interrupt handlers (enable pull-up as AVR pulls it low or leaves floating)
        gpio_set_irq_callback(irqGPIO_);
        gpio::setAsInputPullUp(RP_PIN_AVR_INT);
        gpio_set_irq_enabled(RP_PIN_AVR_INT, GPIO_IRQ_EDGE_FALL, true);

        // try talking to the AVR chip and see that all is well
        // read the full AVR state (including time information). Do not process the interrupts here, but wait for the first tick, which will or them with the ones obtained here and process when the device is fully initialized
        {
            int n = i2c_read_blocking(i2c0, RCKID_AVR_I2C_ADDRESS, (uint8_t *) & io::avrState_, sizeof(AVRState), false);
            if (n != sizeof(AVRState))
                ERROR(error::HardwareProblem, n);
            LOG(LL_INFO, "AVR uptime: " << io::avrState_.uptime);
            LOG(LL_INFO, "Current time: " << io::avrState_.time);
        }

        Radio::initialize();

        // initialize the audio chips - first radio, then audio chip
        // TODO check that a pullup on the audio codec reset pin is sufficient to start it properly?
        //io::radio_.reset(RP_PIN_RADIO_RESET);
        //cpu::delayMs(100);
        // 

        // do we see the radio chip after reset?
        //LOG(LL_INFO, "  SI4705 (0x11):     " << (::i2c::isPresent(0x11) ? "ok" : "not found"));

        // initialize the SD card
        sdInitialize();

        // initialize the filesystem and mount the SD card
        fs::initialize();

        // initialize the audio output
        audio::playbackSm_ = pio_claim_unused_sm(pio1, true);
        audio::playbackOffset_ = pio_add_program(pio1, & i2s_out16_program);
        audio::dma0_ = dma_claim_unused_channel(true);
        audio::dma1_ = dma_claim_unused_channel(true);
        i2s_out16_program_init(pio1, audio::playbackSm_, audio::playbackOffset_, RP_PIN_I2S_DOUT, RP_PIN_I2S_LRCK);
      

        time::nextSecond_ += 1000000;

        // enable I2C interrupts so that we can start processing the I2C packet queues
        i2c0->hw->intr_mask = I2C_IC_INTR_MASK_M_RX_FULL_BITS | I2C_IC_INTR_MASK_M_TX_ABRT_BITS;

        RAMHeap::traceChunks();


        return;

        uint8_t cmd[] = {0x01, 0b11010000, 0x05};
        ::i2c::masterTransmit(0x11, cmd, 3, nullptr, 0);
        //i2c_write_blocking(i2c0, 0x11, cmd, 3, false);
        while (true) {
            uint8_t res = 0;
            i2c_read_blocking(i2c0, 0x11, & res, 1, false);
            LOG(LL_INFO, "res: " << hex(res));
            if (res >= 0x80)
                break;
            yield();
            cpu::delayMs(50);
            yield();
            yield();
            yield();
            yield();
        }
        LOG(LL_INFO, "Radio chip is ready OK");

        if (false) {
            io::radio_.powerOn();
            cpu::delayMs(500);
            auto res = io::radio_.getStatus();
            LOG(LL_INFO, "get stat: " << bin(res.rawResponse()));
            cpu::delayMs(500);
            res = io::radio_.getStatus();
            LOG(LL_INFO, "get stat: " << bin(res.rawResponse()));
            cpu::delayMs(500);
            res = io::radio_.getStatus();
            LOG(LL_INFO, "get stat: " << bin(res.rawResponse()));
            cpu::delayMs(500);
            res = io::radio_.getStatus();
            LOG(LL_INFO, "get stat: " << bin(res.rawResponse()));
            // TODO this will fail now asthe radio is not powered on yet
            Si4705::VersionInfo v = io::radio_.getVersion();
            LOG(LL_INFO, "Radio chip version: ");
            LOG(LL_INFO, "  response:      " << bin(v.response.rawResponse()));
            LOG(LL_INFO, "  part #:        " << v.partNumber);
            LOG(LL_INFO, "  fw:            " << v.fwMajor << "." << v.fwMinor);
            LOG(LL_INFO, "  patch:         " << v.patch);
            LOG(LL_INFO, "  comp:          " << v.compMajor << "." << v.compMinor);
            LOG(LL_INFO, "  chip revision: " << v.chipRevision);
            LOG(LL_INFO, "  cid:           " << v.cid);
            cpu::delayMs(500);
            res = io::radio_.setGPO1(true);
            LOG(LL_INFO, "set gpo1: " << bin(res.rawResponse()));
            cpu::delayMs(500);
            res = io::radio_.enableGPO1(true);
            LOG(LL_INFO, "enable gpo1: " << bin(res.rawResponse()));
       }

        // the radio chip needs to be reset in order to work properly with the I2C 
        //gpio::outputLow(RP_PIN_RADIO_RESET);
        //cpu::delayMs(10);
        //gpio::setAsInputPullup(RP_PIN_RADIO_RESET);
        // TODO after radio chip is ready, we need to tell it to reset the audio chip as well via its GPIO


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
        fs::initialize();
        // enter base arena for the application
        //Arena::enter();

        // read the full AVR state (including time information). Do not process the interrupts here, but wait for the first tick, which will or them with the ones obtained here and process when the device is fully initialized
        i2c_read_blocking(i2c0, RCKID_AVR_I2C_ADDRESS, (uint8_t *) & io::avrState_, sizeof(AVRState), false);
        
        // set brightness to the last used value
        // or at least 16 so that we can see stuff
        if (io::avrState_.brightness < 16)
            io::avrState_.brightness = 16;
        displaySetBrightness(io::avrState_.brightness);


        // configure the AVR interrupt pin as input pullup (it's pulled down by the AVR when needed, floating otherwise) and connect an interrupt callback on the falling edge
        //gpio::setAsInputPullUp(RP_PIN_AVR_INT);
        //gpio_set_irq_enabled_with_callback(RP_PIN_AVR_INT, GPIO_IRQ_EDGE_FALL, true, [](uint gpio, uint32_t events) {
        //    if (gpio == RP_PIN_AVR_INT)
        //        requestAvrStatus();
        //});

        // TODO radio interrupt
        // TODO audio interrupt
    }

    void tick() {
        yield();
        // advance local time and check idle countdowns
        uint64_t now = time_us_64();
        while (now > time::nextSecond_) {
            time::nextSecond_ += 1000000;
            io::avrState_.time.secondTick();
            if (time::idle_) {
                --time::idleTimeoutKeepalive_;
                --time::idleTimeout_;
                if (time::idleTimeoutKeepalive_ == 0 || time::idleTimeout_ == 0)
                    i2c::sendCommand(cmd::PowerOff{});
            } else {
                time::idle_ = true;
                time::idleTimeout_ = RCKID_IDLE_TIMETOUT;
                time::idleTimeoutKeepalive_ = RCKID_IDLE_TIMETOUT_KEEPALIVE;
            }
            App::onSecondTick();
        }
    }

    void yield() {
        StackProtection::check();
        tight_loop_contents();
        tud_task();
    }

    void keepAlive() {
        StackProtection::check();
        time::idleTimeout_ = RCKID_IDLE_TIMETOUT;
    }

    uint32_t uptimeUs() {
        StackProtection::check();
        return time_us_32();
    }

    uint64_t uptimeUs64() {
        StackProtection::check();
        return time_us_64();
    }

    TinyDateTime timeNow() {
        StackProtection::check();
        // return the current time from the AVR state
        return io::avrState_.time;
    }

    // io

    bool btnDown(Btn b) {
        StackProtection::check();
        return buttonState(b, io::avrState_.status);
    }

    bool btnPressed(Btn b) {
        StackProtection::check();
        return buttonState(b, io::avrState_.status) && ! buttonState(b, io::lastStatus_);
    }

    bool btnReleased(Btn b) {
        StackProtection::check();
        return ! buttonState(b, io::avrState_.status) && buttonState(b, io::lastStatus_);
    }

    void btnClear(Btn b) {
        StackProtection::check();
        RCKid::setButtonState(b, io::lastStatus_, btnDown(b));
    }


    int16_t accelX() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    int16_t accelY() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    int16_t accelZ() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    int16_t gyroX() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    int16_t gyroY() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    int16_t gyroZ() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    uint16_t lightAmbient() {
        StackProtection::check();
        return io::lightAls_;
    }
    
    uint16_t lightUV() {
        StackProtection::check();
        return io::lightUV_;
    }

    // display

    void displayOn() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void displayOff() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void displayClear(ColorRGB color) {
        StackProtection::check();
        ST7789::clear(color.raw16());
    }

    DisplayRefreshDirection displayRefreshDirection() {
        StackProtection::check();
        return ST7789::refreshDirection();
    }

    void displaySetRefreshDirection(DisplayRefreshDirection value) {
        StackProtection::check();
        ST7789::setRefreshDirection(value);
    }

    uint8_t displayBrightness() {
        StackProtection::check();
        return io::avrState_.brightness;
    }

    void displaySetBrightness(uint8_t value) {
        StackProtection::check();
        i2c::sendCommand(cmd::SetBrightness{value});
        // we set the brightness here as it is a simple change outside of the small state we get on interrupts 
        io::avrState_.brightness = value;
    }

    Rect displayUpdateRegion() {
        StackProtection::check();
        return ST7789::updateRegion();
    }

    void displaySetUpdateRegion(Rect value) {
        StackProtection::check();
        ST7789::setUpdateRegion(value);
    }

    void displaySetUpdateRegion(Coord width, Coord height) {
        StackProtection::check();
        displaySetUpdateRegion(Rect::XYWH((RCKID_DISPLAY_WIDTH - width) / 2, (RCKID_DISPLAY_HEIGHT - height) / 2, width, height));
    }

    bool displayUpdateActive() {
        StackProtection::check();
        return ST7789::dmaUpdateInProgress();
    }

    void displayWaitUpdateDone() {
        StackProtection::check();
        // TODO can we be smarter here and go to sleep?  
        while (displayUpdateActive())
            yield();
    }

    void displayWaitVSync() {
        StackProtection::check();
        ST7789::waitVSync();
    }

    void displayUpdate(uint16_t const * pixels, uint32_t numPixels, DisplayUpdateCallback callback) {
        StackProtection::check();
        ST7789::dmaUpdateAsync(pixels, numPixels, callback);
    }

    void displayUpdate(uint16_t const * pixels, uint32_t numPixels) {
        StackProtection::check();
        ST7789::dmaUpdateAsync(pixels, numPixels);
    }

    // audio

    void audioConfigurePlaybackDMA(int dma, int other) {
        auto dmaConf = dma_channel_get_default_config(dma);
        channel_config_set_transfer_data_size(& dmaConf, DMA_SIZE_32); // transfer 32 bits (16 per channel, 2 channels)
        channel_config_set_read_increment(& dmaConf, true);  // increment on read
        channel_config_set_dreq(&dmaConf,  pio_get_dreq(pio1, audio::playbackSm_, true)); // DMA is driven by the I2S playback pio
        channel_config_set_chain_to(& dmaConf, other); // chain to the other channel
        dma_channel_configure(dma, & dmaConf, &pio1->txf[audio::playbackSm_], nullptr, 0, false); // the buffer consists of stereo samples, (32bits), i.e. buffer size / 2
        // enable IRQ0 on the DMA channel (shared with other framework DMA uses such as the display or the SD card)
        dma_channel_set_irq0_enabled(dma, true);
    }

    void __not_in_flash_func(audioPlaybackDMA)(uint finished, [[maybe_unused]] uint other) {
        // reconfigure the currently finished buffer to start from the current front buffer (will be back after the swap) - note the other dma has already been started by the finished one
        dma_channel_set_read_addr(finished, audio::playbackBuffer_->front(), false);
        // now load the front buffer with user data and adjust the audio levels according to the settings and resolution
        uint32_t nextSamples = audio::cb_(audio::playbackBuffer_->front(), audio::playbackBuffer_->size() / 2);
        // configure the DMA to transfer only the correct number of samples
        dma_channel_set_trans_count(finished, nextSamples, false);
        audio::playbackBuffer_->swap();
    }

    void audioStreamRefill(void * buffer, unsigned int samples) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    bool audioHeadphones() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    bool audioPaused() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    bool audioPlayback() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    bool audioRecording() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    uint8_t audioVolume() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void audioSetVolume(uint8_t value) {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void audioPlay(DoubleBuffer<int16_t> & buffer, uint32_t sampleRate, AudioCallback cb) {
        StackProtection::check();
        // 
        audio::playbackBuffer_ = & buffer;
        audio::state_ = audio::State::Playback;
        audio::sampleRate_ = sampleRate;
        audio::cb_ = cb;
        // initialize the DMA channels 
        audioConfigurePlaybackDMA(audio::dma0_, audio::dma1_);
        audioConfigurePlaybackDMA(audio::dma1_, audio::dma0_);
        // now we need to load the buffer's front part with audio data
        audioPlaybackDMA(audio::dma0_, audio::dma1_);
        // enable the first DMA
        dma_channel_start(audio::dma0_);
        // start the pio program with appropriate sample rate speed
        audio::state_ = audio::State::Playback;
        pio_set_clock_speed(pio1, audio::playbackSm_, sampleRate * 34 * 2);
        pio_sm_set_enabled(pio1, audio::playbackSm_, true);
        // now we need to load the second buffer while the first one is playing (reload of the buffers will be done by the IRQ handler)
        audioPlaybackDMA(audio::dma1_, audio::dma0_);
    }

    void audioPause() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void audioResume() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    void audioStop() {
        StackProtection::check();
        UNIMPLEMENTED;
    }

    // SD Card access is in sd/sd.cpp file

    // uint32_t sdCapacity() {}

    // bool sdReadBlocks(uint32_t start, uint8_t * buffer, uint32_t numBlocks) {}

    // bool sdWriteBlocks(uint32_t start, uint8_t const * buffer, uint32_t numBlocks) {}

    // Cartridge filesystem access

    uint32_t cartridgeCapacity() { 
        StackProtection::check();
        return &__cartridge_filesystem_end - &__cartridge_filesystem_start;
    }

    uint32_t cartridgeWriteSize() { 
        StackProtection::check();
        return FLASH_PAGE_SIZE; // 256
    }

    uint32_t cartridgeEraseSize() { 
        StackProtection::check();
        return FLASH_SECTOR_SIZE; // 4096
    }

    void cartridgeRead(uint32_t start, uint8_t * buffer, uint32_t numBytes) {
        StackProtection::check();
        // since flash is memory mapped via XIP, all we need to do is aggregate offset properly 
        memcpy(buffer, XIP_NOCACHE_NOALLOC_BASE + (&__cartridge_filesystem_start - XIP_BASE) + start, numBytes);
    }

    void cartridgeWrite(uint32_t start, uint8_t const * buffer) {
        StackProtection::check();
        ASSERT(start < cartridgeCapacity());
        ASSERT(start + FLASH_PAGE_SIZE <= cartridgeCapacity());
        uint32_t offset = reinterpret_cast<uint32_t>(& __cartridge_filesystem_start) - XIP_BASE + start;
        LOG(LL_LFS, "flash_range_program(" << offset << ", " << (uint32_t)FLASH_PAGE_SIZE << ") - start " << start);
        uint32_t ints = save_and_disable_interrupts();
        flash_range_program(offset, buffer, FLASH_PAGE_SIZE);
        restore_interrupts(ints);
    }

    void cartridgeErase(uint32_t start) {
        StackProtection::check();
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
        StackProtection::check();
       i2c::sendCommand(cmd::Rumbler{effect});
    }

    // rgb

    void rgbEffect(uint8_t rgb, RGBEffect const & effect) {
        StackProtection::check();
        i2c::sendCommand(cmd::SetRGBEffect{rgb, effect});
    }
    
    void rgbEffects(RGBEffect const & a, RGBEffect const & b, RGBEffect const & dpad, RGBEffect const & sel, RGBEffect const & start) {
        StackProtection::check();
        i2c::sendCommand(cmd::SetRGBEffects{a, b, dpad, sel, start});
    }
    
    void rgbOff() {
        StackProtection::check();
        i2c::sendCommand(cmd::RGBOff{});
    }

    // memory

    bool memoryIsImmutable(void const * ptr) {
        StackProtection::check();
        // memory is immutable when it comes from flash
        return (reinterpret_cast<uint32_t>(ptr) < 0x20000000); 
    }

    // budget

    uint32_t budget() {
        StackProtection::check();
        return io::avrState_.budget;
    }

    uint32_t budgetDaily() {
        StackProtection::check();
        return io::avrState_.dailyBudget;
    }

    void budgetSet(uint32_t seconds) {
        StackProtection::check();
        if (seconds == io::avrState_.budget - 1) {
            --io::avrState_.budget;
            i2c::sendCommand(cmd::DecBudget{});
        } else {
            io::avrState_.budget = seconds;
            i2c::sendCommand(cmd::SetBudget{seconds});
        }
    }

    void budgetDailySet(uint32_t seconds) {
        StackProtection::check();
        io::avrState_.dailyBudget = seconds;
        i2c::sendCommand(cmd::SetDailyBudget{seconds});
    }

    void budgetReset() {
        StackProtection::check();
        io::avrState_.budget = io::avrState_.dailyBudget;
        i2c::sendCommand(cmd::ResetBudget{});
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
