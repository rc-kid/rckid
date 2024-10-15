#ifndef ARCH_RCKID_2
#error "You are building RCKid mk II backend without the indicator macro"
#endif

#include <pico/rand.h>
#include <bsp/board.h>
#include "tusb_config.h"
#include "tusb.h"
#include <hardware/structs/usb.h>
#include <hardware/uart.h>

#include <platform/peripherals/bmi160.h>
#include <platform/peripherals/ltr390uv.h>
#include <platform/buffer.h>

#include "rckid/rckid.h"
#include "rckid/internals.h"

#include "screen/ST7789.h"
#include "sd/sd.h"

#include "avr/src/commands.h"
#include "avr/src/status.h"

/** 
    \section rckid_mk2_backend RCKid mk II Backend 
    \addtogroup backends
 
    NOTE: This is a temporary backend that uses the older V2 revision (RP2040 and ATTiny) to allow running the basic SDK on the previous RCKid hardware version. Once the V3 hardware is built and tested, this code will be obsoleted and removed from the repository. 

    The V2 backend is rather complicated because of the small number of RP2040 IO pins required an additional MCU - ATTiny3217 that controls power management and input/output (buttons, LEDs, rumbler). See \ref RP2040Pinout for more details on the hardware connections.
 */

namespace rckid {

    namespace {
        static constexpr unsigned TICK_DONE = 0;
        static constexpr unsigned TICK_ALS = 1;
        static constexpr unsigned TICK_UV = 2;
        static constexpr unsigned TICK_ACCEL = 3;
        static constexpr unsigned TICK_AVR = 4;
        volatile unsigned tickInProgress_ = TICK_DONE;

        platform::BMI160 accelerometer_;
        platform::LTR390UV alsSensor_;
        platform::BMI160::State aState_;
        uint16_t lightAls_ = 0;
        uint16_t lightUV_ = 0;
        Status status_; 
        Status lastStatus_;
        uint8_t ticks_ = 0;
        // reported battery level to prevent battery reporting glitches
        uint8_t batteryLevel_ = 100;


        namespace audio {
            uint8_t volume_ = 128;
            bool playback_ = false;
            uint dma0_ = 0;
            uint dma1_ = 0;
            DoubleBuffer * playbackBuffer_;
            uint8_t bitResolution_ = 12;
            uint32_t sampleRate_ = 44100;
        }

    }

    /** Sends given I2C command to the AVR. 
     */
    template<typename T>
    static void sendCommand(T const & cmd) {
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
        i2c0->hw->tar = accelerometer_.address;
        i2c0->hw->enable = 1;
        i2c0->hw->data_cmd = platform::BMI160::REG_DATA;
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
            switch (tickInProgress_) {
                case TICK_ALS:
                case TICK_UV: {
                    uint16_t value = (i2c0->hw->data_cmd) & 0xff;
                    value += (i2c0->hw->data_cmd & 0xff) * 256;        
                    if (tickInProgress_ == TICK_ALS)
                        lightAls_ = value;
                    else
                        lightUV_ = value;
                    i2cFillAccelTxBlocks();
                    tickInProgress_ = TICK_ACCEL;
                    return;
                }
                case TICK_ACCEL: {
                    // store the accelerometer data
                    uint8_t * raw = reinterpret_cast<uint8_t*>(&aState_);
                    for (int i = 0; i < 12; ++i)
                        *(raw++) = i2c0->hw->data_cmd;
                    // update the accelerometer X and Y axes
                    int16_t ax = - aState_.accelY;
                    int16_t ay = - aState_.accelX;
                    aState_.accelX = ax;
                    aState_.accelY = ay;
                    // and the gyroscope
                    aState_.gyroX *= -1;
                    aState_.gyroY *= -1;
                    // fill in the AVR data
                    i2cFillAVRTxBlocks();
                    tickInProgress_ = TICK_AVR;
                    return;
                }
                case TICK_AVR: {
                    lastStatus_ = status_;
                    uint8_t * raw = reinterpret_cast<uint8_t*>(&status_);
                    for (size_t i = 0; i < sizeof(Status); ++i)
                        *(raw++) = i2c0->hw->data_cmd;
                    // update battery level gauge - calculate battery percentage from the battery voltage ranges and determine if we should display it (i.e. when discharging the percentage can only go down and when charging, the percentage can only go up). 
                    unsigned vb = status_.vBatt();
                    uint8_t bl = 0;
                    if (vb > VOLTAGE_BATTERY_FULL_THRESHOLD)
                        bl = 100;
                    else if (vb > VOLTAGE_CRITICAL_THRESHOLD)
                        bl = static_cast<uint8_t>((vb - VOLTAGE_CRITICAL_THRESHOLD) * 100 / (VOLTAGE_BATTERY_FULL_THRESHOLD - VOLTAGE_CRITICAL_THRESHOLD));
                    if (status_.charging()) {
                        if (bl > batteryLevel_)
                            batteryLevel_ = bl;
                    } else if (bl < batteryLevel_) {
                        batteryLevel_ = bl;
                    }
                } // fallthrough to default handler and to disabling the I2C comms
                default:
                    // we are done with the I2C transfer
                    break;
            }
        } else {
            //++stats::i2cErrors_;
        }
        // everything else than tx empty bits terminates the i2c transfer for the current tick
        tickInProgress_ = TICK_DONE;
        i2c0->hw->intr_mask = 0;
        // and reset the I2C 
        i2c0->hw->enable = 0;
    }


    /** Audio DMA playback handler.
     
        When this fucntion is called, the other DMA is already running (it was started by the finished DMA immediately). Our task is to tell the finished DMA to again start the other one immediately after it finishes, then call the refill function for the buffer DMA's buffer area and convert the filled data from int16_t to 12bit uint16_t.  
     */
    void __not_in_flash_func(audioPlaybackDMA)(uint finished, [[maybe_unused]] uint other) {
        // reconfigure the currently finished buffer to start from the current front buffer (will be back after the swap) - note the other dma has already been started by the finished one
        dma_channel_set_read_addr(finished, audio::playbackBuffer_->getFrontBuffer(), false);
        // swap the buffers (this also calls the callback that fills the front part of the buffer with data)
        audio::playbackBuffer_->swap();
        // finally, while in the IRQ we must ensure that the data filled by the app in the callback conforms to what we expect. This means adjusting the volume and converting from int16_t to uint16_t centered at half the bit resolution
        int16_t * buf = reinterpret_cast<int16_t*>(audio::playbackBuffer_->getBackBuffer());
        if (audio::bitResolution_ == 12) {
            for (uint32_t i = 0, e = audio::playbackBuffer_->size() / 2; i < e; ++i)
                buf[i] = (buf[i] >> 4) + 2048;
        } else {
            ASSERT(audio::bitResolution_ == 11);
            for (uint32_t i = 0, e = audio::playbackBuffer_->size() / 2; i < e; ++i)
                buf[i] = (buf[i] >> 5) + 1024;
        }
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

    void initialize() {
        board_init();
        /* TODO enable the cartridge uart port based on some preprocessor flag. WHen enabled, make all logs, traces and debugs go to the cartridge port as well. 
        stdio_uart_init_full(
            RP_DEBUG_UART, 
            RP_DEBUG_UART_BAUDRATE, 
            RP_DEBUG_UART_TX_PIN, 
            RP_DEBUG_UART_RX_PIN
        ); */
        // initialize the USB
        tud_init(BOARD_TUD_RHPORT);

        // initialize the I2C bus
        i2c_init(i2c0, RP_I2C_BAUDRATE); 
        i2c0->hw->intr_mask = 0;
        gpio_set_function(RP_PIN_SDA, GPIO_FUNC_I2C);
        gpio_set_function(RP_PIN_SCL, GPIO_FUNC_I2C);
        // Make the I2C pins available to picotool
        bi_decl(bi_2pins_with_func(RP_PIN_SDA, RP_PIN_SCL, GPIO_FUNC_I2C));  

#if (defined RP_LOG_TO_SERIAL)
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
        accelerometer_.initialize();
        alsSensor_.initialize();
        alsSensor_.startALS();

        // initialize audio
        audio::dma0_ = dma_claim_unused_channel(true);
        audio::dma1_ = dma_claim_unused_channel(true);

        // initialize the SD card
        sdInitialize();

        // enter base arena for the application
        memoryEnterArena();
        
        // set brightness to 50% by default after startup
        displaySetBrightness(128);
    }

    void tick() {
        while (tickInProgress_ != TICK_DONE)
            yield();
        ++ticks_;
        // make sure the I2C is off, then set it up so that it can talk to the accelerometer
        if (ticks_ % 8 == 0) {
            i2c0->hw->enable = 0;
            i2c0->hw->tar = alsSensor_.address;
            i2c0->hw->enable = 1;
            i2c0->hw->data_cmd = platform::LTR390UV::REG_CTRL; 
            i2c0->hw->data_cmd = ((ticks_ % 16) == 0) ? platform::LTR390UV::CTRL_UV_EN : platform::LTR390UV::CTRL_ALS_EN;
            i2c0->hw->data_cmd = (((ticks_ % 16) == 0) ? platform::LTR390UV::REG_DATA_ALS : platform::LTR390UV::REG_DATA_UV) | I2C_IC_DATA_CMD_RESTART_BITS;
            i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_RESTART_BITS; // 1 for read
            i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_STOP_BITS; // 1 for read
            i2c0->hw->rx_tl = 1;
            tickInProgress_ = (ticks_ % 16 == 0) ? TICK_ALS : TICK_UV; 
        } else {
            i2cFillAccelTxBlocks();
            tickInProgress_ = TICK_ACCEL;
        }
        // make the TX_EMPTY irq fire only when the data is actually processed
        //i2c0->hw->con |= I2C_IC_CON_TX_EMPTY_CTRL_BITS;
        // enable the I2C
        i2c0->hw->intr_mask = I2C_IC_INTR_MASK_M_RX_FULL_BITS | I2C_IC_INTR_MASK_M_TX_ABRT_BITS;
    }

    /** Waits for the tick to be done (and the I2C bus being freed). This is mkII specific hack to ensure that I2C bus is available for the application during the update() method. 
     */
    void rckid_mkII_waitTickDone() {
        while (tickInProgress_ != TICK_DONE)
            yield();
    }

    void yield() {
        tight_loop_contents();
        tud_task();
    }

    void fatalError(uint32_t error, uint32_t line, char const * file) {
        // TODO ensure that there is enough stack for our functions
        // clear all memory arenas to clean up space, this is guarenteed to succeed as the SDK creates memory arena when it finishes initialization    
        while (memoryInsideArena())
            memoryLeaveArena();
        bsod(error, line, file, nullptr);
        // use yield so that we can keep the USB active as well
        // TODO monitor reset, etc 
        while (true)
            yield();
    }

    void fatalError(Error error, uint32_t line, char const * file) {
        fatalError(static_cast<uint32_t>(error), line, file);
    }

    uint32_t uptimeUs() { return time_us_32(); }
    
    uint32_t random() { return get_rand_32(); }

    Writer debugWrite() {
        return Writer{[](char x) {
#if (defined RP_LOG_TO_SERIAL)
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

    // io

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


    bool btnDown(Btn b) {
        return buttonState(b, status_);

    }

    bool btnPressed(Btn b) {
        return buttonState(b, status_) && ! buttonState(b, lastStatus_);

    }

    bool btnReleased(Btn b) {
        return ! buttonState(b, status_) && buttonState(b, lastStatus_);
    }

    int16_t accelX() { return aState_.accelX; }
    int16_t accelY() { return aState_.accelY; }
    int16_t accelZ() { return aState_.accelZ; }

    int16_t gyroX() { return aState_.gyroX; }
    int16_t gyroY() { return aState_.gyroY; }
    int16_t gyroZ() { return aState_.gyroZ; }

    uint16_t lightAmbient() { return lightAls_; }
    uint16_t lightUV() { return lightUV_; }

    int16_t tempAvr() { return status_.temp(); }

    // power management

    void sleep() {
        UNIMPLEMENTED;
    }

    bool charging() { 
        return status_.charging();
    }

    bool dcPower() {
        return status_.powerDC();
    }

    unsigned vBatt() {
        return status_.vBatt();
    }

    unsigned batteryLevel() {
        return batteryLevel_;
    }

    // display

    DisplayMode displayMode() { 
        return ST7789::displayMode();
     }

    void displaySetMode(DisplayMode mode) {
        ST7789::setDisplayMode(mode);
    }

    uint8_t displayBrightness() { 
        // TODO TODO TODO 
        return 255;
        //return status_.brightness();
    }

    void displaySetBrightness(uint8_t value) {  
        sendCommand(cmd::SetBrightness{value});
    }

    Rect displayUpdateRegion() {    
        return ST7789::updateRegion();
    }

    void displaySetUpdateRegion(Rect region) { 
        ST7789::setUpdateRegion(region);
    }

    bool displayUpdateActive() {
        return ST7789::dmaUpdateInProgress();
    }

    void displayWaitVSync() { 
        ST7789::waitVSync();
    }

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels) {
        ST7789::dmaUpdateAsync(pixels, numPixels);
    }

    void displayUpdate(ColorRGB const * pixels, uint32_t numPixels, DisplayUpdateCallback callback) {
        ST7789::dmaUpdateAsync(pixels, numPixels, callback);
    }

    /** Audio
     
        Audio playback is done via two PWM pins. This means that maximum sample rate & bit resolution the RP2040 can provide is 12bits @ 44.1 kHz when overclocked. When running at normal speed (125MHz) this is downgraded to 11bit resolution at the same sample rate. 

        Note that higher quality audio output can be achieved with either I2S, or pio and delta sigma modulation. The delta-sigma can be useful also for microphone reading.  
     */

    void audioConfigurePlaybackDMA(int dma, int other, uint8_t * buffer, size_t bufferSize) {
        auto dmaConf = dma_channel_get_default_config(dma);
        channel_config_set_transfer_data_size(& dmaConf, DMA_SIZE_32); // transfer 32 bits (16 per channel, 2 channels)
        channel_config_set_read_increment(& dmaConf, true);  // increment on read
        channel_config_set_dreq(&dmaConf, pwm_get_dreq(RP_AUDIO_PWM_SLICE));// DMA is driven by the PWM slice overflowing
        channel_config_set_chain_to(& dmaConf, other); // chain to the other channel
        dma_channel_configure(dma, & dmaConf, &pwm_hw->slice[RP_AUDIO_PWM_SLICE].cc, buffer, bufferSize / 4, false); // the buffer consists of stereo samples, (32bits), i.e. buffer size / 2
        // enable IRQ0 on the DMA channel (shared with other framework DMA uses such as the display or the SD card)
        dma_channel_set_irq0_enabled(dma, true);
    }

    void audioOn() {
        sendCommand(cmd::AudioOn{});
    }

    void audioOff() {
        sendCommand(cmd::AudioOff{});
    }

    bool audioEnabled() {
        return status_.audioEnabled();
    }

    bool audioHeadphones() {
        return status_.audioHeadphones();
    }

    uint8_t audioVolume() {
        return audio::volume_;
    }

    uint32_t audioSampleRate() {
        return audio::sampleRate_;
    }

    void audioSetVolume(uint8_t value) {
        audio::volume_ = value;

    }

    void audioPlay(DoubleBuffer & data, uint32_t sampleRate) {
        // stop the previous playback if any
        if (audio::playback_)
            audioStop();
        // set the audio playback buffer
        audio::playbackBuffer_ = & data;
        audio::playback_ = true;
        audio::sampleRate_ = sampleRate;
        // reload the next buffer so that the whole buffer is ready and can be swapped immediately by the DMA
        data.swap();
        // set the left and right pins to be used by the PWM
        gpio_set_function(RP_PIN_PWM_RIGHT, GPIO_FUNC_PWM); 
        gpio_set_function(RP_PIN_PWM_LEFT, GPIO_FUNC_PWM);
        // initialize the DMA channels 
        audioConfigurePlaybackDMA(audio::dma0_, audio::dma1_, data.getFrontBuffer(), data.size());
        audioConfigurePlaybackDMA(audio::dma1_, audio::dma0_, data.getBackBuffer(), data.size());
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
    }

    void audioRecord(DoubleBuffer & data, uint32_t sampleRate) {
        audio::sampleRate_ = sampleRate;
        UNIMPLEMENTED;
    }

    void audioPause() {
        if (audio::playback_) {
            if (pwm_is_enabled(RP_AUDIO_PWM_SLICE)) {
                pwm_set_enabled(RP_AUDIO_PWM_SLICE, false);
            } else {
                pwm_set_enabled(RP_AUDIO_PWM_SLICE, true);
            }
        }        
    }

    void audioStop() {
        if (audio::playback_) {
            dma_channel_abort(audio::dma0_);        
            dma_channel_abort(audio::dma1_);
            pwm_set_enabled(RP_AUDIO_PWM_SLICE, false);
            audio::playback_ = false;
        }

    }

    // RGB LEDs

    void ledsOff() {
        sendCommand(cmd::RGBOff());
    }


    uint8_t buttonLedIndex(Btn b) {
        switch (b) {
            case Btn::A:
                return 0;
            case Btn::B:
                return 1;
            case Btn::Left:
            case Btn::Right:
            case Btn::Up:
            case Btn::Down:
                return 3;
            case Btn::Select:
                return 4;
            case Btn::Start:
                return 5;
            default:
                return 2; 
        }
    }    

    void ledSetEffect(Btn b, RGBEffect const & effect) {
        sendCommand(cmd::SetRGBEffect{buttonLedIndex(b), effect});
    }

    void ledSetEffects(RGBEffect const & dpad, RGBEffect const & a, RGBEffect const & b, RGBEffect const & select, RGBEffect const & start) {
        sendCommand(cmd::SetRGBEffects(a, b, dpad, select, start));
    }

    // Rumbler

    void rumble(RumblerEffect const & effect) {
        sendCommand(cmd::Rumbler(effect));
    }

    // accelerated functions
    #include "rckid/accelerated.inc.h"
    MEM_FILL_8
    MEM_FILL_16
    MEM_FILL_32

}