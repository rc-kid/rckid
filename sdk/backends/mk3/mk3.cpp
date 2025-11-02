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

#if (PICO_CYW43_SUPPORTED == 1)
#include <pico/cyw43_arch.h>
#endif

extern "C" {
    #include <hardware/structs/usb.h>
    #include <hardware/uart.h>
    #include <hardware/flash.h>
    #include <hardware/clocks.h>
}

#include <platform/peripherals/ltr390uv.h>
#include <platform/peripherals/si4705.h>

#include <rckid/rckid.h>
#include <rckid/radio.h>
#include <rckid/wifi.h>
#include "screen/ST7789.h"
#include "sd/sd.h"
#include "i2c.h"
#include "audio/codec.h"
#include <rckid/app.h>
#include <rckid/filesystem.h>
#include <rckid/ui/header.h>
#include <rckid/ui/style.h>

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

    // various forward & extern declarations

    void memoryReset();

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
        uint32_t idleTimeout_ = RCKID_IDLE_TIMEOUT; 
        uint32_t idleTimeoutKeepalive_ = RCKID_IDLE_TIMEOUT_KEEPALIVE;
        uint32_t numTicks_ = 0;
    }

    // TODO move this to the audio codec
    namespace audio {
        AudioCallback cb_;
        uint8_t volume_ = 10;
        uint dma0_ = 0;
        uint dma1_ = 0;
        uint32_t sampleRate_ = 44100;


        int16_t * buffer0_ = nullptr;
        int16_t * buffer1_ = nullptr;
        uint32_t stereoSamples0_ = 0;
        uint32_t stereoSamples1_ = 0;
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
        i2c::getTransactionResponse(reinterpret_cast<uint8_t*>(&status), sizeof(AVRState::Status));
        // archive the old status
        io::lastStatus_ = io::avrState_.status;
        if (time::numTicks_ % RCKID_DEFAULT_RAPIDFIRE_TICKS == 0)
            io::lastStatus_.clearButtons();
        // copy the new one, we take the buttons as is and or the interrupts to make sure none is ever lost, then process them immediately
        io::avrState_.status.updateWith(status);
        // if second tick interrupt is on, we must advance our timekeeping. Note that it is remotely possible that we will get an extra second tick interrupt when synchronizing the clock (we'll transmit the updated value, as well as the second tick at the same time) so that time on RP can be one second off at worst. 
        if (status.secondInt()) {
            io::avrState_.time.inc();
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
        i2c::enqueue(RCKID_AVR_I2C_ADDRESS, nullptr, 0, sizeof(AVRState::Status), updateAvrStatus);
    }

    void __not_in_flash_func(irqDMADone_)() {
        //gpio::outputHigh(GPIO21);
        unsigned irqs = dma_hw->ints0;
        dma_hw->ints0 = irqs;
        // for audio, reset the DMA start address to the beginning of the buffer and tell the double buffer to refill
        if (irqs & (1u << audio::dma0_)) {
            if (audioPlayback()) {
                audio::cb_(audio::buffer0_, audio::stereoSamples0_);
                dma_channel_set_read_addr(audio::dma0_, audio::buffer0_, false);
                dma_channel_set_trans_count(audio::dma0_, audio::stereoSamples0_, false);
            } else if (audioRecording()) {
                audio::cb_(audio::buffer0_, audio::stereoSamples0_);
                dma_channel_set_write_addr(audio::dma0_, audio::buffer0_, false);
                dma_channel_set_trans_count(audio::dma0_, audio::stereoSamples0_, false);
            } else
                UNREACHABLE; // not expected to have IRQ when audio is idle
        }
        if (irqs & (1u << audio::dma1_)) {
            if (audioPlayback()) {
                audio::cb_(audio::buffer1_, audio::stereoSamples1_);
                dma_channel_set_read_addr(audio::dma1_, audio::buffer1_, false);
                dma_channel_set_trans_count(audio::dma1_, audio::stereoSamples1_, false);
            } else if (audioRecording()) {
                audio::cb_(audio::buffer1_, audio::stereoSamples1_);
                dma_channel_set_write_addr(audio::dma1_, audio::buffer1_, false);
                dma_channel_set_trans_count(audio::dma1_, audio::stereoSamples1_, false);
            } else 
                UNREACHABLE; // not expected to have IRQ when audio is idle
        }
        // display DMA IRQ
        if (irqs & ( 1u << ST7789::dma_))
            ST7789::irqHandler();
    }

    void __not_in_flash_func(irqGPIO_)(uint pin, uint32_t events) {
        switch (pin) {
            case RP_PIN_RADIO_INT:
                Radio::irqHandler();
                break;
            case RP_PIN_HEADSET_DETECT:
                if (events & GPIO_IRQ_EDGE_FALL)
                    LOG(LL_INFO, "HI");
                else if (events & GPIO_IRQ_EDGE_RISE)
                    LOG(LL_INFO, "HO");
                break;

            default:
                LOG(LL_ERROR, "Unknown GPIO IRQ on pin " << (uint32_t)pin << " with events " << events);
                break;
        }
    }

    // sdk functions 

    void Error::setFatal(Error err) {
        // simply go top BSOD - no need for HW cleanup
        // TODO Really?
        // TODO memory reset 
        // TODO reset stack pointer as well 
        set(err);
        bsod();
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

    uint8_t debugRead([[maybe_unused]] bool echo) {
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
        // set the single DMA IRQ 0 handler reserved for the SDK
        irq_set_exclusive_handler(DMA_IRQ_0, irqDMADone_);
        //irq_set_exclusive_handler(TIMER_IRQ_0, irqBSOD_);
        // enable DMA IRQ (used by display, audio, etc.)
        irq_set_enabled(DMA_IRQ_0, true);
        // enable GPIO IRQ
        irq_set_enabled(IO_IRQ_BANK0, true);

        i2c::initialize();


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

        // initialize the audio codec and power it up so that we are ready to play sounds when required
        LOG(LL_INFO, "Initializing codec...");
        audio::dma0_ = dma_claim_unused_channel(true);
        audio::dma1_ = dma_claim_unused_channel(true);
        LOG(LL_INFO, "  audio dma0: " << static_cast<uint32_t>(audio::dma0_));
        LOG(LL_INFO, "  audio dma1: " << static_cast<uint32_t>(audio::dma1_));
        Codec::initialize();
        Codec::reset();
        Codec::powerUp();

        // reset radio chip so that we can detect its presence - note this must happen after codec initialization as codec's GPIO1 is wired to the radio reset pin
        Radio::reset();

        // check the I2C devices we expect to find on the bus
        LOG(LL_INFO, "Detecting I2C devices...");
        LOG(LL_INFO, "  AVR (0x43):        " << (::i2c::isPresent(0x43) ? "ok" : "not found"));
        LOG(LL_INFO, "  LSM6DSV (0x6a):    " << (::i2c::isPresent(0x6a) ? "ok" : "not found"));
        LOG(LL_INFO, "  NAU88C22YG (0x1a): " << (::i2c::isPresent(0x1a) ? "ok" : "not found"));
        LOG(LL_INFO, "  SI4705 (0x11):     " << (::i2c::isPresent(0x11) ? "ok" : "not found"));

        // try talking to the AVR chip and see that all is well
        // read the full AVR state (including time information). Do not process the interrupts here, but wait for the first tick, which will or them with the ones obtained here and process when the device is fully initialized
        {
            int n = i2c_read_blocking(i2c0, RCKID_AVR_I2C_ADDRESS, (uint8_t *) & io::avrState_, sizeof(AVRState), false);
            if (n != sizeof(AVRState))
                FATAL_ERROR(Error::hardwareFailure, static_cast<uint32_t>(n));
            if (io::avrState_.avrVersion != AVR_VERSION) {
                LOG(LL_ERROR, "Incompatible AVR version detected: " << static_cast<uint32_t>(io::avrState_.avrVersion) << ", expected " << static_cast<uint32_t>(AVR_VERSION));
                FATAL_ERROR(Error::hardwareFailure, static_cast<uint32_t>(io::avrState_.avrVersion), "Incompatible AVR version");
            }
            LOG(LL_INFO, "AVR uptime: " << io::avrState_.uptime);
            LOG(LL_INFO, "Current time: " << io::avrState_.time);
            // update the volume on the audio codec based on the values received from AVR (last settings)
            Codec::setVolumeSpeaker(io::avrState_.audio.volumeSpeaker());
            Codec::setVolumeHeadphones(io::avrState_.audio.volumeHeadphones());
        }

        // initialize the other peripherals we have
        Radio::initialize();
        // TODO initialize the accelerometer 


        // initialize the SD card communication & sd card itself if present
        sdInitialize();
        sdInitializeCard();


        // initialize the filesystem and mount the SD card
        fs::initialize();

        // initalize the ui style
        ui::Style::load();


        time::nextSecond_ += 1000000;

        // enable I2C interrupts so that we can start processing the I2C packet queues
        //i2c0->hw->intr_mask = I2C_IC_INTR_MASK_M_RX_FULL_BITS | I2C_IC_INTR_MASK_M_TX_ABRT_BITS;

        RAMHeap::traceChunks();


        // initialize the interrupt pins and set the interrupt handlers (enable pull-up as AVR pulls it low or leaves floating)
        gpio_set_irq_callback(irqGPIO_);
        gpio::setAsInputPullUp(RP_PIN_AVR_INT);
        // enable headset detection
        // TODO the external pullup is too weak (100kOhm, enabling pull-up will help)
        gpio::setAsInputPullUp(RP_PIN_HEADSET_DETECT);
        gpio_set_irq_enabled(RP_PIN_HEADSET_DETECT, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

        LOG(LL_INFO, "Initialization done");
    }

    uint32_t speedPct() {
        return clock_get_hz(clk_sys) / 1500000;
    }

    bool setSpeedPct(uint32_t pct) { 
        if (pct > 166)
            return false;
        // disable all peripherals running off the system clock
        ST7789::waitUpdateDone();
        // should we ever overvolt the chip? 
        // if (overvolt) {
        //     vreg_set_voltage(VREG_VOLTAGE_1_20);
        //     sleep_ms(10);
        // }
        if (! set_sys_clock_khz(pct * 1500, true))
            return false;
        // re-enable the peripherals
        Codec::adjustSpeed();
        ST7789::adjustSpeed();
        sdAdjustSpeed();
        return true;
    }

    void setSpeedMax() {
        setSpeedPct(166);
    }

    void tick() {
        ++time::numTicks_;
        requestAvrStatus();
        yield();
        // advance local time and check idle countdowns
        uint64_t now = time_us_64();
        while (now > time::nextSecond_) {
            time::nextSecond_ += 1000000;
            if (time::idle_) {
                --time::idleTimeoutKeepalive_;
                --time::idleTimeout_;
                if (time::idleTimeoutKeepalive_ == 0 || time::idleTimeout_ == 0)
                    i2c::sendAvrCommand(cmd::PowerOff{});
            } else {
                time::idle_ = true;
                time::idleTimeout_ = RCKID_IDLE_TIMEOUT;
                time::idleTimeoutKeepalive_ = RCKID_IDLE_TIMEOUT_KEEPALIVE;
            }
            App::onSecondTick();
        }
    }

    void yield() {
        StackProtection::check();
        tight_loop_contents();
        tud_task();
#if (PICO_CYW43_SUPPORTED == 1)
        if (WiFi::hasInstance())
            cyw43_arch_poll();
#endif
    }

    void lock() {
        displayOff();
        rgbOff();
        LOG(LL_INFO, "Entering display lock mode");
        while (true) {
            cpu::delayMs(100);
            yield();
            tick();
            if (btnPressed(Btn::Home)) {
                LOG(LL_INFO, "Lock mode wakeup");
                btnClear(Btn::Home);
                displayOn();
                break;
            }
        }
    }

    void sleep() {
        StackProtection::check();
        // TODO disable all peripherals, audio, display, etc. 
        // tell AVR that we are going to sleep, AVR will wake us up when home button will be pressed
        //i2c::sendAvrCommand(cmd::Sleep{});
    }

    void powerOff() {
        StackProtection::check();
        i2c::sendAvrCommand(cmd::PowerOff{});
    }

    uint16_t powerVcc() {
        StackProtection::check();
        return io::avrState_.status.vcc();
    }

    bool powerUsbConnected() {
        StackProtection::check();
        return io::avrState_.status.vusb();
    }

    bool powerCharging() {
        StackProtection::check();
        return io::avrState_.status.charging();
    }

    bool debugMode() {
        StackProtection::check();
        return io::avrState_.status.debugMode();
    }

    void setDebugMode(bool value) {
        StackProtection::check();
        if (value)
            i2c::sendAvrCommand(cmd::DebugModeOn{});
        else
            i2c::sendAvrCommand(cmd::DebugModeOff{});
        // don't set the debug mode in state (will be updated automatically on next AVR status read)
    }

    void keepAlive() {
        StackProtection::check();
        time::idleTimeout_ = RCKID_IDLE_TIMEOUT;
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

    void setTimeNow(TinyDateTime const & dt) {
        StackProtection::check();
        i2c::sendAvrCommand(cmd::SetTime{dt});
        io::avrState_.time = dt;
    }

    TinyAlarm timeAlarm() {
        StackProtection::check();
        return io::avrState_.alarm;
    }

    void timeSetAlarm(TinyAlarm const & alarm) {
        StackProtection::check();
        i2c::sendAvrCommand(cmd::SetAlarm{alarm});
        io::avrState_.alarm = alarm;
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

    void btnClear() {
        StackProtection::check();
        io::lastStatus_ = io::avrState_.status;
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
        i2c::sendAvrCommand(cmd::SetBrightness{io::avrState_.brightness});
    }

    void displayOff() {
        StackProtection::check();
        i2c::sendAvrCommand(cmd::SetBrightness{0});
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
        i2c::sendAvrCommand(cmd::SetBrightness{value});
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

    void audioConfigurePlaybackDMA(int dma, int16_t * buffer, uint32_t stereoSamples, int other) {
        auto dmaConf = dma_channel_get_default_config(dma);
        channel_config_set_transfer_data_size(& dmaConf, DMA_SIZE_32); // transfer 32 bits (16 per channel, 2 channels)
        channel_config_set_read_increment(& dmaConf, true);  // increment on read
        channel_config_set_write_increment(& dmaConf, false);  // do not increment on write
        channel_config_set_dreq(&dmaConf, Codec::playbackDReq()); // DMA is driven by the I2S playback pio
        channel_config_set_chain_to(& dmaConf, other); // chain to the other channel
        dma_channel_configure(dma, & dmaConf, Codec::playbackTxFifo(), buffer, stereoSamples, false); // the buffer consists of stereo samples, (32bits)
        // enable IRQ0 on the DMA channel (shared with other framework DMA uses such as the display or the SD card)
        dma_channel_set_irq0_enabled(dma, true);
    }

    void audioConfigureRecordDMA(int dma, int16_t * writeTo, uint32_t stereoSamples, int other) {
        auto dmaConf = dma_channel_get_default_config(dma);
        channel_config_set_transfer_data_size(& dmaConf, DMA_SIZE_32); // transfer 32 bits (16 per channel, 2 channels)
        channel_config_set_read_increment(& dmaConf, false);  // do not increment on read
        channel_config_set_write_increment(& dmaConf, true);  // increment on write
        channel_config_set_dreq(&dmaConf, Codec::recordDReq()); // DMA is driven by the I2S record pio
        channel_config_set_chain_to(& dmaConf, other); // chain to the other channel
        dma_channel_configure(dma, & dmaConf, writeTo, Codec::recordRxFifo(), stereoSamples, false); // the buffer consists of stereo samples, (32bits), i.e. buffer size / 2
        // enable IRQ0 on the DMA channel (shared with other framework DMA uses such as the display or the SD card)
        dma_channel_set_irq0_enabled(dma, true);
    }

    bool audioHeadphones() {
        StackProtection::check();
        return gpio::read(RP_PIN_HEADSET_DETECT) == 0;
    }

    bool audioPaused() {
        StackProtection::check();
        return Codec::isPaused();
    }

    bool audioPlayback() {
        StackProtection::check();
        return Codec::isPlaybackI2S();
    }

    bool audioRecording() {
        StackProtection::check();
        return Codec::isRecordingI2S();
    }

    uint8_t audioVolume() {
        StackProtection::check();
        return audioHeadphones() ? io::avrState_.audio.volumeHeadphones() : io::avrState_.audio.volumeSpeaker();
    }

    void audioSetVolume(uint8_t value) {
        StackProtection::check();
        if (value > 15)
            value = 15;
        uint8_t rawValue = (value << 2) | (value & 0x3);
        if (audioHeadphones()) {
            Codec::setVolumeHeadphones(rawValue);
            io::avrState_.audio.setVolumeHeadphones(value);
        } else {
            Codec::setVolumeSpeaker(rawValue);
            io::avrState_.audio.setVolumeSpeaker(value);
        }
        // tell AVR that we change the volume
        i2c::sendAvrCommand(cmd::SetAudioSettings{io::avrState_.audio});
    }

    void audioPlay(uint32_t sampleRate, AudioCallback cb) {
        StackProtection::check();
        audioStop();

        audio::cb_ = cb;
        audio::cb_(audio::buffer0_, audio::stereoSamples0_);
        audio::cb_(audio::buffer1_, audio::stereoSamples1_);
        audioConfigurePlaybackDMA(audio::dma0_, audio::buffer0_, audio::stereoSamples0_, audio::dma1_);
        audioConfigurePlaybackDMA(audio::dma1_, audio::buffer1_, audio::stereoSamples1_, audio::dma0_);
        Codec::playbackI2S(sampleRate);
        dma_channel_start(audio::dma0_);
    }

    void audioRecordMic(uint32_t sampleRate, AudioCallback cb) {
        audioStop();

        audio::sampleRate_ = sampleRate;
        audio::cb_ = cb;
        // get buffers from callback
        audio::cb_(audio::buffer0_, audio::stereoSamples0_);
        audio::cb_(audio::buffer1_, audio::stereoSamples1_);
        // initialize the DMA channels 
        audioConfigureRecordDMA(audio::dma0_, audio::buffer0_, audio::stereoSamples0_, audio::dma1_);
        audioConfigureRecordDMA(audio::dma1_, audio::buffer1_, audio::stereoSamples1_, audio::dma0_);
        // enable the first DMA
        dma_channel_start(audio::dma0_);
        // instruct the codec to start the playback at given sample rate
        Codec::recordMic(sampleRate);
    }

    void audioRecordLineIn(uint32_t sampleRate, AudioCallback cb) {
        audioStop();

        audio::sampleRate_ = sampleRate;
        audio::cb_ = cb;
        // get buffers from callback
        audio::cb_(audio::buffer0_, audio::stereoSamples0_);
        audio::cb_(audio::buffer1_, audio::stereoSamples1_);
        // initialize the DMA channels
        audioConfigureRecordDMA(audio::dma0_, audio::buffer0_, audio::stereoSamples0_, audio::dma1_);
        audioConfigureRecordDMA(audio::dma1_, audio::buffer1_, audio::stereoSamples1_, audio::dma0_);
        // enable the first DMA
        dma_channel_start(audio::dma0_);
        // instruct the codec to start the playback at given sample rate
        Codec::recordLineIn(sampleRate);
    }

    void audioPause() {
        StackProtection::check();
        Codec::pause();
    }

    void audioResume() {
        StackProtection::check();
        Codec::resume();
    }

    void audioStop() {
        StackProtection::check();
        Codec::stop();
        dma_channel_abort(audio::dma0_);
        dma_channel_abort(audio::dma1_);
        audio::buffer0_ = nullptr;
        audio::buffer1_ = nullptr;
        audio::stereoSamples0_ = 0;
        audio::stereoSamples1_ = 0;
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
       i2c::sendAvrCommand(cmd::Rumbler{effect});
    }

    // rgb

    void rgbEffect(uint8_t rgb, RGBEffect const & effect) {
        StackProtection::check();
        i2c::sendAvrCommand(cmd::SetRGBEffect{rgb, effect});
    }
    
    void rgbEffects(RGBEffect const & a, RGBEffect const & b, RGBEffect const & dpad, RGBEffect const & sel, RGBEffect const & start) {
        StackProtection::check();
        rgbEffect(0, dpad);
        rgbEffect(1, dpad);
        rgbEffect(2, dpad);
        rgbEffect(3, dpad);
        rgbEffect(4, sel);
        rgbEffect(5, start);
        rgbEffect(6, b);
        rgbEffect(7, a);
        // TODO can't send log commands
        //i2c::sendAvrCommand(cmd::SetRGBEffects{a, b, dpad, sel, start});
    }
    
    void rgbOff() {
        StackProtection::check();
        i2c::sendAvrCommand(cmd::RGBOff{});
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
            i2c::sendAvrCommand(cmd::DecBudget{});
        } else {
            io::avrState_.budget = seconds;
            i2c::sendAvrCommand(cmd::SetBudget{seconds});
        }
    }

    void budgetDailySet(uint32_t seconds) {
        StackProtection::check();
        io::avrState_.dailyBudget = seconds;
        i2c::sendAvrCommand(cmd::SetDailyBudget{seconds});
    }

    void budgetReset() {
        StackProtection::check();
        io::avrState_.budget = io::avrState_.dailyBudget;
        i2c::sendAvrCommand(cmd::ResetBudget{});
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
