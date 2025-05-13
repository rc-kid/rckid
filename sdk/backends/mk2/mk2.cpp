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

#include <platform/peripherals/bmi160.h>
#include <platform/peripherals/ltr390uv.h>

#include "screen/ST7789.h"
#include "sd/sd.h"
#include "rckid/rckid.h"

#include "avr/src/commands.h"
#include "avr/src/status.h"


extern "C" {
    extern uint8_t __cartridge_filesystem_start;
    extern uint8_t __cartridge_filesystem_end;
}

extern "C" {
    void *__wrap_malloc(size_t numBytes) { return rckid::Heap::allocBytes(numBytes); }
    void __wrap_free(void * ptr) { 
        if (rckid::Heap::contains(ptr))
            rckid::Heap::free(ptr); 
    }

    void *__wrap_calloc(size_t numBytes) {
        void * result = rckid::Heap::allocBytes(numBytes);
        memset(result, 0, numBytes);
        return result;
    }
}

namespace rckid {

    void memoryReset();

    // forward declaration of the bsod function
    NORETURN(void bsod(uint32_t error, uint32_t line = 0, char const * file = nullptr));
    
    // forward declaration of memory stack protection check
    void memoryCheckStackProtection();

    void audioPlaybackDMA(uint finished, uint other);
    namespace filesystem {
        void initialize();
    }
    namespace io {
        BMI160 accelerometer_;
        LTR390UV alsSensor_;
        
        BMI160::State aState_;
        uint16_t lightAls_ = 0;
        uint16_t lightUV_ = 0;
        TransferrableState state_;
        Status lastStatus_;

        // reported battery level to prevent battery reporting glitches
        uint8_t batteryLevel_ = 100;

        static constexpr unsigned TICK_DONE = 0;
        static constexpr unsigned TICK_ALS = 1;
        static constexpr unsigned TICK_UV = 2;
        static constexpr unsigned TICK_ACCEL = 3;
        static constexpr unsigned TICK_AVR = 4;
        static constexpr unsigned TICK_RESET = 5;
        volatile unsigned tickInProgress_ = TICK_DONE;
        uint8_t ticks_ = 0;
    }

    namespace time {
        uint64_t nextSecond_ = 0;

        bool idle_ = true;
        uint32_t idleTimeout_ = IDLE_TIMEOUT; 
        uint32_t idleTimeoutFallback_ = IDLE_TIMEOUT_FALLBACK;
    }

    namespace audio {
        std::function<uint32_t(int16_t *, uint32_t)> cb_;
        uint8_t volume_ = 10;
        bool playback_ = false;
        uint dma0_ = 0;
        uint dma1_ = 0;
        DoubleBuffer<int16_t> * playbackBuffer_;
        uint8_t bitResolution_ = 12;
        uint32_t sampleRate_ = 44100;
    }

    // helper functions

    /** Waits for the end of tick's async operations. 

        This is particularly useful in cases where drawing would be faster, or when the update method would like to issue I2C commands for the AVR, but the I2C bus is still used by the tick async requests. 

        Returns true if there was any wait, false otherwise. 
     */
    bool waitTickEnd() {
        if (io::tickInProgress_ != io::TICK_RESET) {
            // tick reset is here for the unlucky chance of two threads doing the same
            while (io::tickInProgress_ != io::TICK_DONE && io::tickInProgress_ != io::TICK_RESET) 
                yield();
            io::tickInProgress_ = io::TICK_RESET;
            return true;
        } else {
            return false;
        }
    }

    /** Sends given I2C command to the AVR. 
     */
    template<typename T>
    static void sendCommand(T const & cmd) {
        // if we are the first ones waiting, enable the I2C so that the command below will work, otherwise we've already done so in the past
        if (waitTickEnd())
            i2c_init(i2c0, RP_I2C_BAUDRATE); 
        i2c_write_blocking(i2c0, I2C_AVR_ADDRESS, (uint8_t const *) & cmd, sizeof(T), false);
    }    

    void __not_in_flash_func(i2cFillAVRTxBlocks)() {
        static_assert(sizeof(Status) == 4);
        i2c0->hw->enable = 0;
        i2c0->hw->tar = I2C_AVR_ADDRESS;
        i2c0->hw->enable = 1;
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_STOP_BITS; // 1 for read, stop
        i2c0->hw->rx_tl = 3;
    }

    void __not_in_flash_func(i2cFillAccelTxBlocks)() {
        i2c0->hw->enable = 0;
        i2c0->hw->tar = io::accelerometer_.address;
        i2c0->hw->enable = 1;
        i2c0->hw->data_cmd = BMI160::REG_DATA;
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_RESTART_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_STOP_BITS; // 1 for 
        i2c0->hw->rx_tl = 11;
    }

    void __not_in_flash_func(irqI2CDone_)() {
        uint32_t cause = i2c0->hw->intr_stat;
        i2c0->hw->clr_intr;
        if (cause == I2C_IC_INTR_MASK_M_RX_FULL_BITS) {
            switch (io::tickInProgress_) {
                case io::TICK_ALS:
                case io::TICK_UV: {
                    uint16_t value = (i2c0->hw->data_cmd) & 0xff;
                    value += (i2c0->hw->data_cmd & 0xff) * 256;        
                    if (io::tickInProgress_ == io::TICK_ALS)
                        io::lightAls_ = value;
                    else
                        io::lightUV_ = value;
                    i2cFillAccelTxBlocks();
                    io::tickInProgress_ = io::TICK_ACCEL;
                    return;
                }
                case io::TICK_ACCEL: {
                    // store the accelerometer data
                    uint8_t * raw = reinterpret_cast<uint8_t*>(&io::aState_);
                    for (int i = 0; i < 12; ++i)
                        *(raw++) = i2c0->hw->data_cmd;
                    // update the accelerometer X and Y axes
                    int16_t ax = - io::aState_.accelY;
                    int16_t ay = - io::aState_.accelX;
                    io::aState_.accelX = ax;
                    io::aState_.accelY = ay;
                    // and the gyroscope
                    io::aState_.gyroX *= -1;
                    io::aState_.gyroY *= -1;
                    // fill in the AVR data
                    i2cFillAVRTxBlocks();
                    io::tickInProgress_ = io::TICK_AVR;
                    return;
                }
                case io::TICK_AVR: {
                    io::lastStatus_ = io::state_.status;
                    uint8_t * raw = reinterpret_cast<uint8_t*>(&io::state_.status);
                    for (size_t i = 0; i < sizeof(Status); ++i)
                        *(raw++) = i2c0->hw->data_cmd;
                    // update idle settings in this second
                    time::idle_ = time::idle_ & ! io::state_.status.userControlChanged(io::lastStatus_);
                    // update battery level gauge - calculate battery percentage from the battery voltage ranges and determine if we should display it (i.e. when discharging the percentage can only go down and when charging, the percentage can only go up). 
                    unsigned vb = io::state_.status.vBatt();
                    uint8_t bl = 0;
                    if (vb > VOLTAGE_BATTERY_FULL_THRESHOLD) {
                        bl = 100;
                    } else if (vb > VOLTAGE_CRITICAL_THRESHOLD) {
                        bl = static_cast<uint8_t>((vb - VOLTAGE_CRITICAL_THRESHOLD) * 100 / (VOLTAGE_BATTERY_FULL_THRESHOLD - VOLTAGE_CRITICAL_THRESHOLD));
                    }
                    if (io::state_.status.powerDC() || bl < io::batteryLevel_ || bl > io::batteryLevel_ + 1)
                        io::batteryLevel_ = bl;
                } // fallthrough to default handler and to disabling the I2C comms
                default:
                    // we are done with the I2C transfer
                    break;
            }
        } else {
            //++stats::i2cErrors_;
        }
        // everything else than tx empty bits terminates the i2c transfer for the current tick
        io::tickInProgress_ = io::TICK_DONE;
        i2c0->hw->intr_mask = 0;
        // and reset the I2C 
        i2c0->hw->enable = 0;
    }

    void __not_in_flash_func(irqDMADone_)() {
        //gpio::outputHigh(GPIO21);
        unsigned irqs = dma_hw->ints0;
        dma_hw->ints0 = irqs;
        // for audio, reset the DMA start address to the beginning of the buffer and tell the double buffer to refill
        if (audio::playback_) {
            if (irqs & (1u << audio::dma0_))
                audioPlaybackDMA(audio::dma0_, audio::dma1_);
            if (irqs & (1u << audio::dma1_))
                audioPlaybackDMA(audio::dma1_, audio::dma0_);
        }
        // display
        if (irqs & ( 1u << ST7789::dma_))
            ST7789::irqHandler();
        //gpio::outputLow(GPIO21);
    }

    bool buttonState(Btn b, Status & status) {
        switch (b) {
            case Btn::Up: return status.btnUp();
            case Btn::Down: return status.btnDown();
            case Btn::Left: return status.btnLeft();
            case Btn::Right: return status.btnRight();
            case Btn::A: return status.btnA();
            case Btn::B: return status.btnB();
            case Btn::Select: return status.btnSel();
            case Btn::Start: return status.btnStart();
            case Btn::VolumeUp: return status.btnVolumeUp();
            case Btn::VolumeDown: return status.btnVolumeDown();
            case Btn::Home: return status.btnHome();
            default:
                UNREACHABLE;
        }
    }

    void setButtonState(Btn b, Status & status, bool value) {
        switch (b) {
            case Btn::Up: status.setDPadKeys(status.btnLeft(), status.btnRight(), value, status.btnDown()); return;
            case Btn::Down: status.setDPadKeys(status.btnLeft(), status.btnRight(), status.btnUp(), value); return;
            case Btn::Left: status.setDPadKeys(value, status.btnRight(), status.btnUp(), status.btnDown()); return;
            case Btn::Right: status.setDPadKeys(status.btnLeft(), value, status.btnUp(), status.btnDown()); return;
            case Btn::A: status.setABSelStartKeys(value, status.btnB(), status.btnSel(), status.btnStart()); return;
            case Btn::B: status.setABSelStartKeys(status.btnA(), value, status.btnSel(), status.btnStart()); return;
            case Btn::Select: status.setABSelStartKeys(status.btnA(), status.btnB(), value, status.btnStart()); return;
            case Btn::Start: status.setABSelStartKeys(status.btnA(), status.btnB(), status.btnSel(), value); return;
            case Btn::VolumeUp: status.setBtnVolumeUp(value); return;
            case Btn::VolumeDown: status.setBtnVolumeDown(value); return;
            case Btn::Home: status.setBtnHome(value); return;
            default:
                UNREACHABLE;
        }
    }

    // rckid API impleemntation

    void fatalError(uint32_t error, uint32_t line, char const * file) {
        // simply go top BSOD - no need for HW cleanup
        // TODO Really?
        // TODO memory reset 
        // TODO reset stack pointer as well 
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
                // ensure that we pass the buffer to the host
                for (int i = 0; i < 20; ++i)
                    yield();         
            } else {
                tud_cdc_write(& x, 1);
            }
#endif
        }};
    }

    uint8_t debugRead(bool echo) {
        return 0;
    }



    void initialize([[maybe_unused]] int argc, [[maybe_unused]] char const * argv[]) {
        board_init();
        memoryInstrumentStackProtection();
        // TODO in mkII we can't enable the USB in general as it leaks voltage into the USB pwr, which in turn leaks voltage to the battery switch mosfet
        // initialize the USB
        tud_init(BOARD_TUD_RHPORT);
        
        // disable USB -- reset so that we can again detect DC charge
        //memset(reinterpret_cast<uint8_t *>(usb_hw), 0, sizeof(*usb_hw));

        // initialize the I2C bus
        i2c_init(i2c0, RP_I2C_BAUDRATE); 
        i2c0->hw->intr_mask = 0;
        gpio_set_function(RP_PIN_SDA, GPIO_FUNC_I2C);
        gpio_set_function(RP_PIN_SCL, GPIO_FUNC_I2C);
        // Make the I2C pins available to picotool
        bi_decl(bi_2pins_with_func(RP_PIN_SDA, RP_PIN_SCL, GPIO_FUNC_I2C)); 

#if (defined RCKID_LOG_TO_SERIAL)
        // initialize uart0 on pins 16 & 17 as serial out
        uart_init(uart0, 74880);
        gpio_set_function(16, GPIO_FUNC_UART);
        gpio_set_function(17, GPIO_FUNC_UART);
#endif

        //usb_hw->main_ctrl = 0;
        // set the single DMA IRQ 0 handler reserved for the SDK
        irq_set_exclusive_handler(DMA_IRQ_0, irqDMADone_);
        irq_set_exclusive_handler(I2C0_IRQ, irqI2CDone_);
        //irq_set_exclusive_handler(TIMER_IRQ_0, irqBSOD_);
        irq_set_enabled(I2C0_IRQ, true);
        // make the I2C IRQ priority larger than that of the DMA (0x80) to ensure that I2C comms do not have to wait for render done if preparing data takes longer than sending them
        irq_set_priority(I2C0_IRQ, 0x40); 
        //irq_set_priority(SPI0_IRQ, 0x40);
        //irq_set_priority(SPI1_IRQ, 0x40);
        // set TIMER_IRQ_0 used for fatal error BSOD to be the highest
        //irq_set_priority(TIMER_IRQ_0, 0x0);

        // initialize the display
        ST7789::initialize();

        // initialize the accelerometer & uv light sensor
        io::accelerometer_.initialize();
        io::alsSensor_.initialize();
        io::alsSensor_.startALS();

        // initialize audio
        audio::dma0_ = dma_claim_unused_channel(true);
        audio::dma1_ = dma_claim_unused_channel(true);

        // initialize the SD card
        sdInitialize();

        // initialize the filesystem
        filesystem::initialize();

        // enter base arena for the application
        //Arena::enter();

        // read the full AVR state and set last tick for time keeping
        i2c_read_blocking(i2c0, I2C_AVR_ADDRESS, (uint8_t *) & io::state_, sizeof(TransferrableState), false);
        time::nextSecond_ = time_us_64() + 1000000;
        
        // set brightness to 50% by default after startup
        displaySetBrightness(128);

#if (defined RCKID_WAIT_FOR_SERIAL)
        char cmd_ = ' ';
        while (tud_cdc_read(& cmd_, 1) != 1) { yield(); };
        LOG(LL_ERROR, "Received command " << cmd_);
#endif
    }

    void tick() {
        yield();
        //joystickTick();
        waitTickEnd();
        uint64_t now = time_us_64();
        while (now > time::nextSecond_) {
            time::nextSecond_ += 1000000;
            io::state_.time.secondTick();
            if (time::idle_) {
                --time::idleTimeoutFallback_;
                --time::idleTimeout_;
                if (time::idleTimeoutFallback_ == 0 || time::idleTimeout_ == 0)
                    sendCommand(cmd::PowerOff{});
            } else {
                time::idle_ = true;
                time::idleTimeout_ = IDLE_TIMEOUT;
                time::idleTimeoutFallback_ = IDLE_TIMEOUT_FALLBACK;
            }
        }
        ++io::ticks_;
        // make sure the I2C is off, then set it up so that it can talk to the accelerometer
        if (io::ticks_ % 8 == 0) {
            i2c0->hw->enable = 0;
            i2c0->hw->tar = io::alsSensor_.address;
            i2c0->hw->enable = 1;
            i2c0->hw->data_cmd = LTR390UV::REG_CTRL; 
            i2c0->hw->data_cmd = ((io::ticks_ % 16) == 0) ? LTR390UV::CTRL_UV_EN : LTR390UV::CTRL_ALS_EN;
            i2c0->hw->data_cmd = (((io::ticks_ % 16) == 0) ? LTR390UV::REG_DATA_ALS : LTR390UV::REG_DATA_UV) | I2C_IC_DATA_CMD_RESTART_BITS;
            i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_RESTART_BITS; // 1 for read
            i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_STOP_BITS; // 1 for read
            i2c0->hw->rx_tl = 1;
            io::tickInProgress_ = (io::ticks_ % 16 == 0) ? io::TICK_ALS : io::TICK_UV; 
        } else {
            i2cFillAccelTxBlocks();
            io::tickInProgress_ = io::TICK_ACCEL;
        }
        // make the TX_EMPTY irq fire only when the data is actually processed
        //i2c0->hw->con |= I2C_IC_CON_TX_EMPTY_CTRL_BITS;
        // enable the I2C
        i2c0->hw->intr_mask = I2C_IC_INTR_MASK_M_RX_FULL_BITS | I2C_IC_INTR_MASK_M_TX_ABRT_BITS;
    }

    void yield() {
        memoryCheckStackProtection();
        tight_loop_contents();
        tud_task();
    }

    uint32_t uptimeUs() {
        memoryCheckStackProtection();
        return time_us_32();
    }

    // io

    bool btnDown(Btn b) {
        memoryCheckStackProtection();
        return buttonState(b, io::state_.status);
    }

    bool btnPressed(Btn b) {
        memoryCheckStackProtection();
        return buttonState(b, io::state_.status) && ! buttonState(b, io::lastStatus_);
    }

    bool btnReleased(Btn b) {
        memoryCheckStackProtection();
        return ! buttonState(b, io::state_.status) && buttonState(b, io::lastStatus_);
    }

    void btnClear(Btn b) {
        memoryCheckStackProtection();
        setButtonState(b, io::lastStatus_, btnDown(b));
    }

    int16_t accelX() {
        memoryCheckStackProtection();
        return io::aState_.accelX; 
    }

    int16_t accelY() {
        memoryCheckStackProtection();
        return io::aState_.accelY; 
    }

    int16_t accelZ() {
        memoryCheckStackProtection();
        return io::aState_.accelX; 
    }

    int16_t gyroX() {
        memoryCheckStackProtection();
        return io::aState_.gyroX; 
    }

    int16_t gyroY() {
        memoryCheckStackProtection();
        return io::aState_.gyroY; 
    }

    int16_t gyroZ() {
        memoryCheckStackProtection();
        return io::aState_.gyroZ; 
    }

    uint16_t lightAmbient() { 
        memoryCheckStackProtection();
        return io::lightAls_;
    }

    uint16_t lightUV() { 
        memoryCheckStackProtection();
        return io::lightUV_;
    }

    // display

    DisplayRefreshDirection displayRefreshDirection() {
        memoryCheckStackProtection();
        return ST7789::refreshDirection();
    }

    void displaySetRefreshDirection(DisplayRefreshDirection value) {
        memoryCheckStackProtection();
        ST7789::setRefreshDirection(value);
    }

    uint8_t displayBrightness() {
        memoryCheckStackProtection();
        return io::state_.brightness;
    }

    void displaySetBrightness(uint8_t value) {
        sendCommand(cmd::SetBrightness{value});
        memoryCheckStackProtection();
        io::state_.brightness = value;
    }

    Rect displayUpdateRegion() {
        memoryCheckStackProtection();
        return ST7789::updateRegion();
    }

    void displaySetUpdateRegion(Rect value) {
        ST7789::setUpdateRegion(value);
        memoryCheckStackProtection();
    }

    void displaySetUpdateRegion(Coord width, Coord height) {
        displaySetUpdateRegion(Rect::XYWH((RCKID_DISPLAY_WIDTH - width) / 2, (RCKID_DISPLAY_HEIGHT - height) / 2, width, height));
        memoryCheckStackProtection();
    }

    bool displayUpdateActive() {
        memoryCheckStackProtection();
        return ST7789::dmaUpdateInProgress();
    }

    void displayWaitUpdateDone() {
        // TODO can we be smarter here and go to sleep?  
        while (displayUpdateActive())
            yield();
    }

    void displayWaitVSync() {
        ST7789::waitVSync();
        memoryCheckStackProtection();
    }

    void displayUpdate(uint16_t const * pixels, uint32_t numPixels, DisplayUpdateCallback callback) {
        ST7789::dmaUpdateAsync(pixels, numPixels, callback);
        memoryCheckStackProtection();
    }

    void displayUpdate(uint16_t const * pixels, uint32_t numPixels) {
        ST7789::dmaUpdateAsync(pixels, numPixels);
        memoryCheckStackProtection();
    }

    /** Audio
     
        Audio playback is done via two PWM pins. This means that maximum sample rate & bit resolution the RP2040 can provide is 12bits @ 44.1 kHz when overclocked. When running at normal speed (125MHz) this is downgraded to 11bit resolution at the same sample rate. 

        Note that higher quality audio output can be achieved with either I2S, or pio and delta sigma modulation. The delta-sigma can be useful also for microphone reading.  

        Audio operates with two buffers and two DMAs. While one buffer is being streamed to the audio output via its DMA, the second buffer can be filled by the callback function. When the buffer is done, we switch immediately to the second buffer and then do callback to refill the already finished buffer. For this we can use the double buffer provided by the application.  
     */
    void audioConfigurePlaybackDMA(int dma, int other) {
        auto dmaConf = dma_channel_get_default_config(dma);
        channel_config_set_transfer_data_size(& dmaConf, DMA_SIZE_32); // transfer 32 bits (16 per channel, 2 channels)
        channel_config_set_read_increment(& dmaConf, true);  // increment on read
        channel_config_set_dreq(&dmaConf, pwm_get_dreq(RP_AUDIO_PWM_SLICE));// DMA is driven by the PWM slice overflowing
        channel_config_set_chain_to(& dmaConf, other); // chain to the other channel
        dma_channel_configure(dma, & dmaConf, &pwm_hw->slice[RP_AUDIO_PWM_SLICE].cc, nullptr, 0, false); // the buffer consists of stereo samples, (32bits), i.e. buffer size / 2
        // enable IRQ0 on the DMA channel (shared with other framework DMA uses such as the display or the SD card)
        dma_channel_set_irq0_enabled(dma, true);
    }

    void adjustAudioBuffer(int16_t * buffer, uint32_t bufferLength) {
        if (audio::volume_ == 0) {
            // TODO this can be fast memfill
            for (uint32_t i = 0; i < bufferLength; ++i)
                 buffer[i] = 0;
        } else if (audio::bitResolution_ == 12) {
            for (uint32_t i = 0; i < bufferLength; ++i)
                buffer[i] = (buffer[i] >> (14 - audio::volume_)) + 2048;
        } else {
            ASSERT(audio::bitResolution_ == 11);
            for (uint32_t i = 0; i < bufferLength; ++i)
                buffer[i] = (buffer[i] >> (15 - audio::volume_)) + 1024;

        }
    }

    void __not_in_flash_func(audioPlaybackDMA)(uint finished, [[maybe_unused]] uint other) {
        // reconfigure the currently finished buffer to start from the current front buffer (will be back after the swap) - note the other dma has already been started by the finished one
        dma_channel_set_read_addr(finished, audio::playbackBuffer_->front(), false);
        // now load the front buffer with user data and adjust the audio levels according to the settings and resolution
        uint32_t nextSamples = audio::cb_(audio::playbackBuffer_->front(), audio::playbackBuffer_->size() / 2);
        adjustAudioBuffer(audio::playbackBuffer_->front(), nextSamples * 2);
        // configure the DMA to transfer only the correct number of samples
        dma_channel_set_trans_count(finished, nextSamples, false);
        audio::playbackBuffer_->swap();
    }

    void audioEnable() {
        if (!io::state_.status.audioEnabled())
            sendCommand(cmd::AudioOn());
        memoryCheckStackProtection();
    }

    void audioDisable() {
        if (io::state_.status.audioEnabled())
            sendCommand(cmd::AudioOff());
        memoryCheckStackProtection();
    }

    void audioStop(bool audioOff) {
        if (audio::playback_) {
            uint32_t ii = save_and_disable_interrupts();
            dma_channel_abort(audio::dma0_);        
            dma_channel_abort(audio::dma1_);
            pwm_set_enabled(RP_AUDIO_PWM_SLICE, false);
            audio::playback_ = false;
            restore_interrupts(ii);
        }
        if (audioOff)
            audioDisable();
        memoryCheckStackProtection();
    }

    bool audioHeadphones() {
        memoryCheckStackProtection();
        return io::state_.status.audioHeadphones();
    }

    bool audioPaused() {
        memoryCheckStackProtection();
        if (audio::playback_) {
            return ! pwm_is_enabled(RP_AUDIO_PWM_SLICE);
        } else {
            return false;
        }
    }

    bool audioPlayback() {
        memoryCheckStackProtection();
        return audio::playback_;
    }

    bool audioRecording() {
        memoryCheckStackProtection();
        return false;
    }

    uint8_t audioVolume() {
        memoryCheckStackProtection();
        return audio::volume_;
    }

    void audioSetVolume(uint8_t value) {
        memoryCheckStackProtection();
        if (value > 10)
            value = 10;
        audio::volume_ = value;
    }

    void audioPlay(DoubleBuffer<int16_t> & buffer, uint32_t sampleRate, AudioCallback cb) {
        audioEnable();
        // stop the previous playback if any
        if (audio::playback_)
            audioStop(false);
        // set the audio playback buffer
        audio::playbackBuffer_ = & buffer;
        audio::playback_ = true;
        audio::sampleRate_ = sampleRate;
        audio::cb_ = cb;
        // set the left and right pins to be used by the PWM
        gpio_set_function(RP_PIN_PWM_RIGHT, GPIO_FUNC_PWM); 
        gpio_set_function(RP_PIN_PWM_LEFT, GPIO_FUNC_PWM);
        // initialize the DMA channels 
        audioConfigurePlaybackDMA(audio::dma0_, audio::dma1_);
        audioConfigurePlaybackDMA(audio::dma1_, audio::dma0_);
        // now we need to load the buffer's front part with audio data
        audioPlaybackDMA(audio::dma0_, audio::dma1_);
        // enable the first DMA
        dma_channel_start(audio::dma0_);
        // and finally the timers
        float clkdiv = cpu::clockSpeed() / (4096.0 * sampleRate);
        if (clkdiv > 1) { // 12 bit sound
            audio::bitResolution_ = 12;
            pwm_set_wrap(RP_AUDIO_PWM_SLICE, 4096); // set wrap to 12bit sound levels
        } else { // 11bit sound
            clkdiv = cpu::clockSpeed() / (2048.0 * sampleRate);
            audio::bitResolution_ = 11;
            pwm_set_wrap(RP_AUDIO_PWM_SLICE, 2048); // set wrap to 12bit sound levels
        }
        ASSERT(clkdiv > 1); // otherwise we won't be able to
        pwm_set_clkdiv(RP_AUDIO_PWM_SLICE, clkdiv);
        pwm_set_enabled(RP_AUDIO_PWM_SLICE, true);
        // now we need to load the second buffer while the first one is playing (reload of the buffers will be done by the IRQ handler)
        audioPlaybackDMA(audio::dma1_, audio::dma0_);
        memoryCheckStackProtection();
    }

    void audioPause() {
        if (audio::playback_)  
            pwm_set_enabled(RP_AUDIO_PWM_SLICE, false);
        memoryCheckStackProtection();
    }

    void audioResume() {
        if (audio::playback_)  
            pwm_set_enabled(RP_AUDIO_PWM_SLICE, true);
        memoryCheckStackProtection();
    }

    void audioStop() {
        audioStop(true);
        memoryCheckStackProtection();
    }

    // SD Card access is in sd/sd.cpp file

    // Cartridge filesystem access

    uint32_t cartridgeCapacity() { 
        memoryCheckStackProtection();
        return &__cartridge_filesystem_end - &__cartridge_filesystem_start;
    }

    uint32_t cartridgeWriteSize() {
        memoryCheckStackProtection();
        return FLASH_PAGE_SIZE; // 256
    }

    uint32_t cartridgeEraseSize() {
        memoryCheckStackProtection();
        return FLASH_SECTOR_SIZE; // 4096
    }

    void cartridgeRead(uint32_t start, uint8_t * buffer, uint32_t numBytes) {
        // since flash is memory mapped via XIP, all we need to do is aggregate offset properly 
        memcpy(buffer, XIP_NOCACHE_NOALLOC_BASE + (&__cartridge_filesystem_start - XIP_BASE) + start, numBytes);
        memoryCheckStackProtection();
    }

    void cartridgeWrite(uint32_t start, uint8_t const * buffer) {
        ASSERT(start < cartridgeCapacity());
        ASSERT(start + FLASH_PAGE_SIZE <= cartridgeCapacity());
        uint32_t offset = reinterpret_cast<uint32_t>(& __cartridge_filesystem_start) - XIP_BASE + start;
        LOG(LL_LFS, "flash_range_program(" << offset << ", " << (uint32_t)FLASH_PAGE_SIZE << ") - start " << start);
        uint32_t ints = save_and_disable_interrupts();
        flash_range_program(offset, buffer, FLASH_PAGE_SIZE);
        restore_interrupts(ints);
        memoryCheckStackProtection();
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
        memoryCheckStackProtection();
    }

    // memory

    bool memoryIsImmutable(void const * ptr) {
        memoryCheckStackProtection();
        // TODO enable immutable memory from ROM
        return false;
    }

}