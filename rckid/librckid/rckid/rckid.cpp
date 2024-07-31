#include "rckid.h"

#if (defined ARCH_RP2040)

#include "bsp/board.h"
#include "tusb_config.h"
#include "tusb.h"
#include "hardware/structs/usb.h"

#include "FatFS/ff.h"
#include "FatFS/diskio.h"

#include <pico/time.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/pwm.h>
#include <hardware/dma.h>
#include <pico/multicore.h>


#include "rckid.h"
#include "fs/sd.h"
#include "assets/all.h"
#include "graphics/ST7789.h"
#include "graphics/png.h"
#include "graphics/framebuffer.h"
#include "audio/audio.h"


#include "ST7789_rgb.pio.h"
#include "ST7789_rgb_double.pio.h"
//#include "mic_clk.pio.h"
#include "mic_pdm.pio.h"


#include <platform/peripherals/ltr390uv.h>
#include <platform/peripherals/bmi160.h>


platform::LTR390UV alsSensor_{};
platform::BMI160 accelerometer_{};

static constexpr unsigned TICK_DONE = 0;
static constexpr unsigned TICK_ALS = 1;
static constexpr unsigned TICK_UV = 2;
static constexpr unsigned TICK_ACCEL = 3;
static constexpr unsigned TICK_AVR = 4;

volatile unsigned tickInProgress_ = TICK_DONE;
static uint32_t errorCode_ = 0;
static uint32_t errorLine_ = 0;
static const char * errorFile_ = nullptr;

extern "C" {
    void resetHeap();
}

// cartridge interface
namespace rckid::cartridge {
    void initialize();
    void yield();
}

namespace rckid {

    /** The blue screen of death fatal error handler. 
     */
    void irqBSOD_() {
        // TODO reset SP to some good value to ensure we can call functions
        hw_clear_bits(&timer_hw->intr, 1);

        multicore_reset_core1();
        resetHeap();
        FrameBuffer<ColorRGB> fb{};
        fb.fill(ColorRGB::Blue());
        Writer w = fb.textMultiline(10,20);
        w << ":( Fatal error: " << errorCode_ << "\n\n" ;
        if (errorFile_ != nullptr) {
            w << "   line: " << errorLine_ << "\n"
            << "   file: " << errorFile_ << "\n\n";
        }
        w << "   Long press home button to turn off,\n   then restart.\n\n";
        w << "free heap: " << getFreeHeap() << ", malloc " << getMallocCalls() << ", free " << getFreeCalls();
        // reset the display and draw the framebuffer
        ST7789::reset();
        fb.enable();
        fb.render();
        // enter busy wait loop - we need to be restarted now
        while (true) {}
    }

    void __not_in_flash_func(i2cFillAVRTxBlocks)() {
        i2c0->hw->enable = 0;
        i2c0->hw->tar = AVR_I2C_ADDRESS;
        i2c0->hw->enable = 1;
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_STOP_BITS; // 1 for read, stop
        i2c0->hw->rx_tl = 7;
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

    void __not_in_flash_func(irqDMADone_)() {
        //gpio::outputHigh(GPIO21);
        unsigned irqs = dma_hw->ints0;
        dma_hw->ints0 = irqs;
        // for audio, reset the DMA start address to the beginning of the buffer and tell the stream to refill
        if (irqs & (1u << audio::dma0_))
            audio::irqHandler1();
        if (irqs & (1u << audio::dma1_))
            audio::irqHandler2();
        // display
        if (irqs & ( 1u << ST7789::dma_))
            ST7789::irqHandler();
        //gpio::outputLow(GPIO21);
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
                        DeviceWrapper::lightALS_ = value;
                    else
                        DeviceWrapper::lightUV_ = value;
                    i2cFillAccelTxBlocks();
                    tickInProgress_ = TICK_ACCEL;
                    return;
                }
                case TICK_ACCEL: {
                    // store the accelerometer data
                    uint8_t * raw = reinterpret_cast<uint8_t*>(&DeviceWrapper::aState_);
                    for (int i = 0; i < 12; ++i)
                        *(raw++) = i2c0->hw->data_cmd;
                    // update the accelerometer X and Y axes
                    int16_t ax = - DeviceWrapper::aState_.accelY;
                    int16_t ay = - DeviceWrapper::aState_.accelX;
                    DeviceWrapper::aState_.accelX = ax;
                    DeviceWrapper::aState_.accelY = ay;
                    // and the gyroscope
                    DeviceWrapper::aState_.gyroX *= -1;
                    DeviceWrapper::aState_.gyroY *= -1;
                    // fill in the AVR data
                    i2cFillAVRTxBlocks();
                    tickInProgress_ = TICK_AVR;
                    return;
                }
                case TICK_AVR: {
                    DeviceWrapper::lastState_ = DeviceWrapper::state_.state;
                    uint8_t * raw = reinterpret_cast<uint8_t*>(&DeviceWrapper::state_);
                    for (int i = 0; i < 8; ++i)
                        *(raw++) = i2c0->hw->data_cmd;
                    LOG("AVR: " << Writer::hex(reinterpret_cast<uint8_t*>(&DeviceWrapper::state_), 8));
                    DeviceWrapper::lastState_ = DeviceWrapper::state_.state;
                    // update battery level gauge
                    unsigned battPct = vBatt();
                    if (battPct <= VCC_CRITICAL_THRESHOLD)
                        battPct = 0;
                    else if (battPct >= VBATT_FULL_THRESHOLD)
                        battPct = 100;
                    else 
                        battPct = (battPct - VCC_CRITICAL_THRESHOLD) * 100 / (VBATT_FULL_THRESHOLD - VCC_CRITICAL_THRESHOLD);
                    DeviceWrapper::batteryLevel_ = battPct;
                } // fallthrough to default handler and to disabling the I2C comms
                default:
                    // we are done with the I2C transfer
                    break;
            }
        } else {
            ++stats::i2cErrors_;
            LOG("ERR:" << Writer::hex(cause));
            /*
            i2c_deinit(i2c0);
            i2c_init(i2c0, RP_I2C_BAUDRATE); 
            i2c0->hw->intr_mask = 0;
            gpio_set_function(RP_PIN_SDA, GPIO_FUNC_I2C);
            gpio_set_function(RP_PIN_SCL, GPIO_FUNC_I2C);
            */
        }
        // everything else than tx empty bits terminates the i2c transfer for the current tick
        tickInProgress_ = TICK_DONE;
        i2c0->hw->intr_mask = 0;
        i2c0->hw->enable = 0;
        stats::tickUpdateUs_ = uptimeUs() - stats::tickUpdateStart_;
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
        // TODO serial if necessary
        //tud_init(BOARD_TUD_RHPORT);
        //usb_hw->main_ctrl = 0;
        // set the single DMA IRQ 0 handler reserved for the SDK
        irq_set_exclusive_handler(DMA_IRQ_0, irqDMADone_);
        irq_set_exclusive_handler(I2C0_IRQ, irqI2CDone_);
        irq_set_exclusive_handler(TIMER_IRQ_0, irqBSOD_);
        irq_set_enabled(I2C0_IRQ, true);
        // make the I2C IRQ priority larger than that of the DMA (0x80) to ensure that I2C comms do not have to wait for render done if preparing data takes longer than sending them
        irq_set_priority(I2C0_IRQ, 0x40); 
        irq_set_priority(SPI0_IRQ, 0x40);
        irq_set_priority(SPI1_IRQ, 0x40);
        // set TIMER_IRQ_0 used for fatal error BSOD to be the highest
        irq_set_priority(TIMER_IRQ_0, 0x0);
        // initialize the display
        ST7789::initialize();
        setBrightness(128);
        setButtonsRainbow(16);
        // initialize accelerometer & ALS sensor peripherals
        accelerometer_.initialize();
        alsSensor_.initialize();
        alsSensor_.startALS();

        audio::initialize();
        // configure the SD card
        SD::initialize();

        // and run any cartridge-specific initialization
        cartridge::initialize();

        setRumbler(RumblerEffect::OK());
    }

    void yield() {
        tight_loop_contents();
        tud_task();
        cartridge::yield();
    }

   void tick() {
        MEASURE_TIME(stats::tickUs_, 
            while (tickInProgress_ != TICK_DONE)
                yield();
            stats::tickUpdateStart_ = uptimeUs();

            i2cFillAVRTxBlocks();
            tickInProgress_ = TICK_AVR;
#ifdef FOO
            // make sure the I2C is off, then set it up so that it can talk to the accelerometer
            if (stats::ticks_ % 8 == 0) {
                i2c0->hw->enable = 0;
                i2c0->hw->tar = alsSensor_.address;
                i2c0->hw->enable = 1;
                i2c0->hw->data_cmd = platform::LTR390UV::REG_CTRL; 
                i2c0->hw->data_cmd = ((stats::ticks_ % 16) == 0) ? platform::LTR390UV::CTRL_UV_EN : platform::LTR390UV::CTRL_ALS_EN;
                i2c0->hw->data_cmd = (((stats::ticks_ % 16) == 0) ? platform::LTR390UV::REG_DATA_ALS : platform::LTR390UV::REG_DATA_UV) | I2C_IC_DATA_CMD_RESTART_BITS;
                i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_RESTART_BITS; // 1 for read
                i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_STOP_BITS; // 1 for read
                i2c0->hw->rx_tl = 1;
                tickInProgress_ = (stats::ticks_ % 16 == 0) ? TICK_ALS : TICK_UV; 
            } else {
                i2cFillAccelTxBlocks();
                tickInProgress_ = TICK_ACCEL;
            }
#endif
            // make the TX_EMPTY irq fire only when the data is actually processed
            //i2c0->hw->con |= I2C_IC_CON_TX_EMPTY_CTRL_BITS;
            // enable the I2C
            i2c0->hw->intr_mask = I2C_IC_INTR_MASK_M_RX_FULL_BITS | I2C_IC_INTR_MASK_M_TX_ABRT_BITS;
            // set the interrupt 
            ++stats::ticks_;
        );
    }

    void fatalError(uint32_t code, uint32_t line, char const * file) {
        // fill in error metadata
        errorCode_ = code;
        errorLine_ = line;
        errorFile_ = file;
        // set the timer IRQ to fire
        hw_set_bits(&timer_hw->inte, 1);
        irq_set_enabled(TIMER_IRQ_0, true);
        timer_hw->alarm[0] = (timer_hw->timelr + 1000);
        // forever loop so that we don't return
        while(true) {};
    }

    void powerOff() {
        /// TODO: make sure sd and other things are done first, only then poweroff
        DeviceWrapper::sendCommand(cmd::PowerOff{}); 
    }

    Writer writeToSerial() {
        return Writer{[](char x) {
            tud_cdc_write(& x, 1);
            if (x == '\n') {
                tud_cdc_write_flush();            
            }
        }};
    }

    // ============================================================================================
    // Audio
    // ============================================================================================

    void audio::initialize() {
        // initialize audio PWM pins
        audio::dma0_ = dma_claim_unused_channel(true);
        audio::dma1_ = dma_claim_unused_channel(true);

        // initialize the microphone - DAT pin to the PWM & PWM to counting
        //gpio::setAsInputPullDown(RP_PIN_PDM_DATA);
        //pwm_config cfg = pwm_get_default_config();
        //pwm_config_set_clkdiv_mode(&cfg, PWM_DIV_B_HIGH);
        //pwm_config_set_clkdiv(&cfg, 1);
        //pwm_init(RP_MIC_SLICE, &cfg, false);
        //gpio_set_function(RP_PIN_PDM_DATA, GPIO_FUNC_PWM);
        // initailize the CLK pio
        micSm_ = pio_claim_unused_sm(pio0, true);
        //micOffset_ = pio_add_program(pio0, & mic_clk_program);
        micOffset_ = pio_add_program(pio0, & mic_pdm_program);

        mic_pdm_program_init(pio0, micSm_, micOffset_, RP_PIN_PDM_CLK, RP_PIN_PDM_DATA);

    }

    bool audio::headphonesActive() {
        return DeviceWrapper:: state_.state.audioEnabled() && DeviceWrapper::state_.state.headphones();         
    }

    void audio::play(audio::OutStream * stream) {
        if (stream == nullptr) {
            // it's only playback resume
            // TODO check there is active stream
            DeviceWrapper::sendCommand(cmd::AudioEnabled{});
            pwm_set_enabled(RP_PWM_SLICE, true);
        } else {
            // set the left and right pins to be used by the PWM
            gpio_set_function(RP_PIN_PWM_RIGHT, GPIO_FUNC_PWM); 
            gpio_set_function(RP_PIN_PWM_LEFT, GPIO_FUNC_PWM);
            pwm_set_wrap(RP_PWM_SLICE, 4096); // set wrap to 12bit sound levels
            // configure & chain the audio buffer DMAs
            configurePlaybackDMA(dma0_, dma1_, buffer0_, RP_AUDIO_BUFFER_SIZE);
            configurePlaybackDMA(dma1_, dma0_, buffer1_, RP_AUDIO_BUFFER_SIZE);
            // fill the streams
            stream->fillBuffer(buffer0_, RP_AUDIO_BUFFER_SIZE);
            stream->fillBuffer(buffer1_, RP_AUDIO_BUFFER_SIZE);
            // enable audio and start the playback
            playback_ = stream;
            pwm_set_clkdiv(RP_PWM_SLICE, cpu::clockSpeed() / (4096.0 * stream->sampleRate()));
            DeviceWrapper::sendCommand(cmd::AudioEnabled{});
            dma_channel_start(dma0_);
            pwm_set_enabled(RP_PWM_SLICE, true);
        }
    }

    void audio::pause() {
        pwm_set_enabled(RP_PWM_SLICE, false);
        DeviceWrapper::sendCommand(cmd::AudioDisabled{});
    }

    void audio::stop(){
        dma_channel_abort(audio::dma0_);        
        dma_channel_abort(audio::dma1_);
        if (playback_ != nullptr) {
            DeviceWrapper::sendCommand(cmd::AudioDisabled{});
            pwm_set_enabled(RP_PWM_SLICE, false);
//            dma_channel_abort(audio::dma0_);        
//            dma_channel_abort(audio::dma1_);
            playback_ = nullptr;
        } else {
            //pwm_set_enabled(RP_PWM_SLICE, false);
            //pwm_set_enabled(RP_MIC_SLICE, false);
            pio_sm_set_enabled(pio0, micSm_, false);
        }
    }

    void audio::record(std::function<void(uint16_t const *, size_t)> cb) {
        micCallback_ = cb;

        configureRecordDMA(dma0_, dma1_, buffer0_, RP_AUDIO_BUFFER_SIZE);
        configureRecordDMA(dma1_, dma0_, buffer1_, RP_AUDIO_BUFFER_SIZE);


        /*
        // detach the audio left and right pins from the PWM 
        gpio_set_function(RP_PIN_PWM_RIGHT, GPIO_FUNC_NULL);
        gpio_set_function(RP_PIN_PWM_LEFT, GPIO_FUNC_NULL);
        // set the RP_PWM_SLICE to 16kHz
        pwm_set_clkdiv(RP_PWM_SLICE, 1);
        pwm_set_wrap(RP_PWM_SLICE, cpu::clockSpeed() / 16000);

        // reset the counter and start counting
        pwm_set_counter(RP_MIC_SLICE, 0);
        pwm_set_enabled(RP_MIC_SLICE, true);
        // start the clk
        mic_clk_program_init(pio0, micSm_, micOffset_, RP_PIN_PDM_CLK);
        pio_set_clock_speed(pio0, micSm_, 8160000);
        pio_sm_set_enabled(pio0, micSm_, true);
        // reset last mic so that we can calculate the deltas
        micLast_ = 0;
        // start the reader 
        */
        pio_sm_set_enabled(pio0, micSm_, true);
        dma_channel_start(dma0_);
//        pwm_set_enabled(RP_PWM_SLICE, true);

        // TODO stop playback if any 
        // set the mic PWM to the sample rate
        
        //configureRecordDMA(dma0_, dma1_, buffer0_, RP_AUDIO_BUFFER_SIZE);
        //configureRecordDMA(dma1_, dma0_, buffer1_, RP_AUDIO_BUFFER_SIZE);

        // start the CLK PIO
    }

    void audio::irqHandler1() {
        if (playback_ != nullptr) {
            dma_channel_set_read_addr(dma0_, buffer0_, false);
            playback_->fillBuffer(buffer0_, RP_AUDIO_BUFFER_SIZE);
        } else {
            dma_channel_set_write_addr(dma0_, buffer0_, false);
            onBufferRecorded(buffer0_, RP_AUDIO_BUFFER_SIZE);
        }
    }

    void audio::irqHandler2() {
        if (playback_ != nullptr) {
            dma_channel_set_read_addr(dma1_, buffer1_, false);
            playback_->fillBuffer(buffer1_, RP_AUDIO_BUFFER_SIZE);
        } else {
            dma_channel_set_write_addr(dma1_, buffer1_, false);
            onBufferRecorded(buffer1_, RP_AUDIO_BUFFER_SIZE);
        }
    }

    void audio::onBufferRecorded(uint16_t * buffer, size_t size) {
        uint32_t * b32 = reinterpret_cast<uint32_t*>(buffer);
        for (size_t i = 0; i < size / 16; ++i) {
            uint16_t x = 0;
            for (size_t j = 0; j < 8; ++j)            
                x += platform::popCount(b32[i * 8 + j]);
            buffer[i] = x;
        }
        /*
        for (size_t i = 0; i < size; ++i) {
            uint16_t x = buffer[i];
            buffer[i] = x - micLast_;
            micLast_ = x;
        }
        */
        micCallback_(buffer, size / 16);
    }

    void audio::configurePlaybackDMA(int dma, int other, uint16_t * buffer, size_t bufferSize) {
        auto dmaConf = dma_channel_get_default_config(dma);
        channel_config_set_transfer_data_size(& dmaConf, DMA_SIZE_32); // transfer 32 bits (16 per channel, 2 channels)
        channel_config_set_read_increment(& dmaConf, true);  // increment on read
        channel_config_set_dreq(&dmaConf, pwm_get_dreq(RP_PWM_SLICE));// DMA is driven by the PWM slice overflowing
        channel_config_set_chain_to(& dmaConf, other); // chain to the other channel
        dma_channel_configure(dma, & dmaConf, &pwm_hw->slice[RP_PWM_SLICE].cc, buffer, bufferSize / 2, false); // the buffer consists of stereo samples, (32bits), i.e. buffer size / 2
        // enable IRQ0 on the DMA channel (shared with other framework DMA uses such as the display or the SD card)
        dma_channel_set_irq0_enabled(dma, true);
    }

    void audio::configureRecordDMA(int dma, int other, uint16_t * buffer, size_t bufferSize) {
        auto dmaConf = dma_channel_get_default_config(dma);
        channel_config_set_transfer_data_size(&dmaConf, DMA_SIZE_32); // transfer 16 bits (single channel, cummulative)
        channel_config_set_dreq(& dmaConf, pio_get_dreq(pio0, micSm_, false)); // drew is pio rx is ready 
        channel_config_set_read_increment(&dmaConf, false);  // do not increment on read - pio queue
        channel_config_set_write_increment(&dmaConf, true); // increment on write - the buffer
        channel_config_set_chain_to(& dmaConf, other); // chain to the other channel
        dma_channel_configure(dma, & dmaConf, buffer, &pio0->rxf[micSm_], bufferSize / 2, false); // the buffer consists of mono samples, (16bits)
        // enable IRQ0 on the DMA channel (shared with other framework DMA uses such as the display or the SD card)
        dma_channel_set_irq0_enabled(dma, true);


        /*
        auto dmaConf = dma_channel_get_default_config(dma);
        channel_config_set_transfer_data_size(&dmaConf, DMA_SIZE_16); // transfer 16 bits (single channel, cummulative)
        channel_config_set_read_increment(&dmaConf, false);  // do not increment on read - the PWM counter
        channel_config_set_write_increment(&dmaConf, true); // increment on write - the buffer
        channel_config_set_dreq(&dmaConf, pwm_get_dreq(RP_PWM_SLICE));// DMA is driven by the PWM slice overflowing
        channel_config_set_chain_to(& dmaConf, other); // chain to the other channel
        dma_channel_configure(dma, & dmaConf, buffer, &pwm_hw->slice[RP_MIC_SLICE].ctr, bufferSize, false); // the buffer consists of mono samples, (16bits)
        // enable IRQ0 on the DMA channel (shared with other framework DMA uses such as the display or the SD card)
        dma_channel_set_irq0_enabled(dma, true);
        */
    }

    // ============================================================================================
    // DeviceWrapper
    // ============================================================================================

    void DeviceWrapper::waitTickDone() {
        uint32_t timeout = uptimeUs() + 5000;
        while (tickInProgress_ != TICK_DONE) {
            yield();
            if (timeout < uptimeUs()) {
                ++stats::i2cErrors_;
                i2c0->hw->intr_mask = 0;
                i2c0->hw->enable = 0;
                tickInProgress_ = false;
            }
        }
    }

    // ============================================================================================
    // ST7789 Driver
    // ============================================================================================

    void ST7789::initialize() {
        // load and initialize the PIO programs for single and double precission
        pio_ = pio0;
        sm_ = pio_claim_unused_sm(pio_, true);
        offsetSingle_ = pio_add_program(pio_, & ST7789_rgb_program);
        offsetDouble_ = pio_add_program(pio_, & ST7789_rgb_double_program);
        // initialize the DMA channel and set up interrupts
        dma_ = dma_claim_unused_channel(true);
        dmaConf_ = dma_channel_get_default_config(dma_); // create default channel config, write does not increment, read does increment, 32bits size
        channel_config_set_transfer_data_size(& dmaConf_, DMA_SIZE_16); // transfer 16 bytes
        channel_config_set_dreq(& dmaConf_, pio_get_dreq(pio_, sm_, true)); // tell our PIO
        channel_config_set_read_increment(& dmaConf_, true);
        dma_channel_configure(dma_, & dmaConf_, &pio_->txf[sm_], nullptr, 0, false); // start

        // enable IRQ0 on the DMA channel
        dma_channel_set_irq0_enabled(dma_, true);
        //irq_add_shared_handler(DMA_IRQ_0, irqDMADone,  PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
        irq_set_enabled(DMA_IRQ_0, true);
        // reset the display

        reset();

        // now clear the entire display black
#if (defined RCKID_SPLASHSCREEN_OFF)
        setColumnRange(0, 239);
        setRowRange(0, 319);
        beginCommand(RAMWR);
        gpio_put(RP_PIN_DISP_DCX, true);
        for (size_t i = 0, e =320 * 240; i < e; ++i) {
            sendByte(0);
            sendByte(0);
        }
        end();
#else
        configure(DisplayMode::Natural_RGB565);
        resetUpdateRegion();
        beginUpdate();
        PNG png = PNG::fromBuffer(assets::images::logo_black_16);
        png.decode([&](ColorRGB * line, int lineNum, int lineWidth){
            update(line, lineWidth);
        });
        endUpdate();
        configure(DisplayMode::Native_RGB565);
#endif
    }

    void ST7789::reset() {
        dma_channel_abort(dma_);
        pio_sm_set_enabled(pio_, sm_, false);
        updating_ = 0;

        gpio_init(RP_PIN_DISP_TE);
        gpio_set_dir(RP_PIN_DISP_TE, GPIO_IN);
        gpio_init(RP_PIN_DISP_DCX);
        gpio_set_dir(RP_PIN_DISP_DCX, GPIO_OUT);
        gpio_init(RP_PIN_DISP_CSX);
        gpio_set_dir(RP_PIN_DISP_CSX, GPIO_OUT);
        gpio_put(RP_PIN_DISP_CSX, true);

        initializePinsBitBang();

        // TODO check the init sequence
        // we need to use busy waits since the reset is also called from the fatal error handler, which happens inside an IRQ 
        sendCommand(SWRESET);
        busy_wait_ms(150);
        sendCommand(VSCSAD, (uint8_t)0);
        configure(DisplayMode::Native_RGB565);
        sendCommand(TEON, TE_VSYNC);
        sendCommand(SLPOUT);
        busy_wait_ms(150);
        sendCommand(DISPON);
        busy_wait_ms(150);
        //sendCommand(MADCTL, (uint8_t)(MADCTL_MV));
        //sendCommand(MADCTL, (uint8_t)(MADCTL_MY | MADCTL_MV ));
        //sendCommand(MADCTL, 0_u8);
        //setDisplayMode(ST7789::DisplayMode::Native);
        sendCommand(INVON);
    }

    void ST7789::configure(DisplayMode mode) {
        endDMAUpdate(); // make sure we are in bitbang mode when configuring
        switch (mode) {
            case DisplayMode::Native_RGB565:
                sendCommand(COLMOD, COLMOD_565);
                sendCommand(MADCTL, 0_u8);
                break;
            case DisplayMode::Native_2X_RGB565:
                sendCommand(COLMOD, COLMOD_565);
                sendCommand(MADCTL, 0_u8);
                break;
            case DisplayMode::Natural_RGB565:
                sendCommand(COLMOD, COLMOD_565);
                sendCommand(MADCTL, static_cast<uint8_t>(MADCTL_MY | MADCTL_MV));
                break;
            case DisplayMode::Natural_2X_RGB565:
                sendCommand(COLMOD, COLMOD_565);
                sendCommand(MADCTL, static_cast<uint8_t>(MADCTL_MY | MADCTL_MV));
                break;
            /*
            case DisplayMode::Native_RGB666:
                sendCommand(COLMOD, COLMOD_666);
                sendCommand(MADCTRL, 0);
                break;
            case DisplayMode::Native_BGR666:
                sendCommand(COLMOD, COLMOD_66);
                sendCommand(MADCTRL, MADCTL_BGR);
                break;
            */
        }
        displayMode_ = mode;
    }

    void ST7789::fill(ColorRGB color) {
        setColumnRange(0, 239);
        setRowRange(0, 319);
        beginCommand(RAMWR);
        gpio_put(RP_PIN_DISP_DCX, true);
        uint16_t x = color.rawValue16();
        for (size_t i = 0, e = 320 * 240; i < e; ++i) {
            sendByte((x >> 8) & 0xff);
            sendByte(x & 0xff);
        }
        end();
    }

    void ST7789::setUpdateRegion(Rect rect) {
        switch (displayMode_) {
            case DisplayMode::Native_RGB565:
            case DisplayMode::Native_2X_RGB565:
                setColumnRange(rect.top(), rect.bottom() - 1);
                setRowRange(rect.left(), rect.right() - 1);
                break;
            case DisplayMode::Natural_RGB565:
            case DisplayMode::Natural_2X_RGB565:
                setRowRange(rect.top(), rect.bottom() - 1);
                setColumnRange(rect.left(), rect.right() - 1);
                break;
            default:
                UNREACHABLE;
        }
    }

    void ST7789::beginUpdate() {
        beginCommand(RAMWR);
        gpio_put(RP_PIN_DISP_DCX, true);
    }

    void ST7789::endUpdate() {
        end();
    }

    void ST7789::update(ColorRGB const * pixels, uint32_t numPixels) {
        uint8_t const * raw = reinterpret_cast<uint8_t const *>(pixels);
        for (unsigned i = 0; i < numPixels; ++i) {
            sendByte(raw[1]);
            sendByte(raw[0]);
            raw += 2;
        }
    }

    void ST7789::beginDMAUpdate() {
        beginCommand(RAMWR);
        gpio_put(RP_PIN_DISP_DCX, true);
        // initialize the corresponding PIO program
        switch (displayMode_) {
            case DisplayMode::Native_RGB565:
            case DisplayMode::Natural_RGB565:
                ST7789_rgb_program_init(pio_, sm_, offsetSingle_, RP_PIN_DISP_WRX, RP_PIN_DISP_DB8);
                break;
            case DisplayMode::Native_2X_RGB565:
            case DisplayMode::Natural_2X_RGB565:
                ST7789_rgb_double_program_init(pio_, sm_, offsetDouble_, RP_PIN_DISP_WRX, RP_PIN_DISP_DB8);
                break;
            default:
                UNREACHABLE;
        }
        // and start the pio
        pio_sm_set_enabled(pio_, sm_, true);
    }

    void ST7789::endDMAUpdate() {
        if (pio_sm_is_enabled(pio_, sm_)) {
            // let the last display update finish (blocking)
            waitUpdateDone();  
            cb_ = nullptr; // to be sure
            pio_sm_set_enabled(pio_, sm_, false);
            end(); // end the RAMWR command
            initializePinsBitBang();
        }
    }

    void ST7789::initializePinsBitBang() {
        uint32_t outputPinsMask = (1 << RP_PIN_DISP_WRX) | (0xff << RP_PIN_DISP_DB8); // DB8..DB15 are consecutive
        gpio_init_mask(outputPinsMask);
        gpio_set_dir_masked(outputPinsMask, outputPinsMask);
        //gpio_put_masked(outputPinsMask, false);
        gpio_put(RP_PIN_DISP_WRX, false);
    }

    void ST7789::beginCommand(uint8_t cmd) {
        ASSERT(!pio_sm_is_enabled(pio_, sm_) && "Commands are bitbanged so the pins can't belong to the pio");
        gpio_put(RP_PIN_DISP_CSX, false);
        gpio_put(RP_PIN_DISP_DCX, false);
        // RP_PIN_DISP_WRX is expected to be low 
        sendByte(cmd);
    }

    void ST7789::irqHandler() {
        if (cb_) 
            cb_();
        if (updating_ == 0 || (--updating_ == 0))
            stats::displayUpdateUs_ = static_cast<unsigned>(uptimeUs() - stats::displayUpdateStart_);
    }

    // ============================================================================================
    // SD Card Raw Access
    // ============================================================================================

    // status of the SD card
    SD::Status sdStatus_ = SD::Status::NotPresent;
    // number of SD card blocks (each block is always 512bytes)
    uint32_t sdNumBlocks_ = 0;

    /** Tells the card to go to idle state (reset) 
     */
    constexpr uint8_t CMD0[] =   { 0x40, 0x00, 0x00, 0x00, 0x00, 0x95 };
    /** Interface condition (voltage & data byte ping-pong to verify connection - 0xaa)
     */
    constexpr uint8_t CMD8[] =   { 0x48, 0x00, 0x00, 0x01, 0xaa, 0x87 };
    /** Send CSD register, returns 16 bytes
     */
    constexpr uint8_t CMD9[] =   { 0x49, 0x00, 0x00, 0x00, 0x00, 0x01 };
    /** Set block length to 512b (only for non SDHC cards)
     */
    constexpr uint8_t CMD16[] =  { 0x50, 0x00, 0x00, 0x02, 0x00, 0x01 };
    /** Application specific command flag. 
     */
    constexpr uint8_t CMD55[] =  { 0x77, 0x00, 0x00, 0x00, 0x00, 0x65 };
    /** Reads the OCR register. 
     */
    constexpr uint8_t CMD58[] =  { 0x7a, 0x00, 0x00, 0x00, 0x00, 0xfd };
    /** App command - send operation condition 
    */
    constexpr uint8_t ACMD41[] = { 0x69, 0x40, 0x00, 0x00, 0x00, 0x77 };


    // response codes
    constexpr uint8_t SD_NO_ERROR = 0;
    constexpr uint8_t SD_IDLE = 1;
    constexpr uint8_t SD_ERASE_RESET = 2;
    constexpr uint8_t SD_ILLEGAL_COMMAND = 4;
    constexpr uint8_t SD_CRC_ERROR = 8;
    constexpr uint8_t SD_ERASE_SEQUENCE_ERROR = 16;
    constexpr uint8_t SD_ADDRESS_ERROR = 32;
    constexpr uint8_t SD_PARAMETER_ERROR = 64;
    constexpr uint8_t SD_VALID = 128;
    constexpr uint8_t SD_BUSY = 255;

    uint8_t sdSendCommand_(uint8_t const (&cmd)[6], uint8_t * response = nullptr, size_t responseSize = 0, unsigned maxDelay = 128) {
        //gpio::low(RP_PIN_SD_CSN);
        spi_write_blocking(RP_SD_SPI, cmd,  6);
        uint8_t result = SD_BUSY;
        while (result == SD_BUSY && maxDelay-- != 0)
            spi_read_blocking(RP_SD_SPI, 0xff, reinterpret_cast<uint8_t*>(& result), 1);
        if (responseSize != 0 && response != nullptr)
            spi_read_blocking(RP_SD_SPI, 0xff, response, responseSize);
        // after reading each response, it is important to send extra one byte to allow the SD crd to "recover"
        uint8_t tmp = 0xff;
        spi_write_blocking(RP_SD_SPI, & tmp, 1);
        //gpio::high(RP_PIN_SD_CSN);
        return result;
    }

    bool sdReadBlocks_(uint32_t start, uint8_t * buffer, uint32_t numBlocks) {
        while (numBlocks-- != 0) {
            //gpio::low(RP_PIN_SD_CSN);
            uint8_t cmd[] = { 
                0x51, 
                static_cast<uint8_t>((start >> 24) & 0xff), 
                static_cast<uint8_t>((start >> 16) & 0xff), 
                static_cast<uint8_t>((start >> 8) & 0xff), 
                static_cast<uint8_t>(start & 0xff),
                0x01
            };
            //uint8_t cmd[] = { 0x51, 0, 0, 0, 0, 0x01 };
            //spi_write_blocking(RP_SD_SPI, cmd, 6);
            //spi_read_blocking(RP_SD_SPI, 0xff, buffer, 2048);
            //gpio::high(RP_PIN_SD_CSN);
            if (sdSendCommand_(cmd) != 0)
                return false;
            // wait for the block start
            uint8_t res = 0xff;
            while (res != 0xfe)
                spi_read_blocking(RP_SD_SPI, 0xff, &res, 1);
            // read the block
            spi_read_blocking(RP_SD_SPI, 0xff, buffer, 512);
            // and read the CRC
            uint16_t crc;
            spi_read_blocking(RP_SD_SPI, 0xff, reinterpret_cast<uint8_t*>(&crc), 2);
            res = 0xff;
            spi_write_blocking(RP_SD_SPI, & res, 1);
            // verify the CRC
            ++stats::sdReadBlocks_;
            ++start;
            buffer += 512;
        }
        return true;
    }

    bool sdWriteBlocks_(uint32_t start, uint8_t const * buffer, uint32_t numBlocks) {
        while (numBlocks-- != 0) {
            uint8_t cmd[] = { 
                0x58,
                static_cast<uint8_t>((start >> 24) & 0xff), 
                static_cast<uint8_t>((start >> 16) & 0xff), 
                static_cast<uint8_t>((start >> 8) & 0xff),
                static_cast<uint8_t>(start & 0xff),
                0x01
            };
            if (sdSendCommand_(cmd) != 0)
                return false;
            cmd[0] = 0xff;
            cmd[1] = 0xfe; // start of data block
            spi_write_blocking(RP_SD_SPI, cmd, 2); // 1 byte wait + start data block
            spi_write_blocking(RP_SD_SPI, buffer, 512);
            spi_write_blocking(RP_SD_SPI, cmd, 2); // CRC
            spi_read_blocking(RP_SD_SPI, 0xff, cmd, 1); // get the data response
            // wait for busy
            while (cmd[0] != 0xff)
                spi_read_blocking(RP_SD_SPI, 0xff, cmd, 1);
            ++stats::sdWriteBlocks_;
            ++start;
            buffer += 512;
        }
        return true;
    } 

    // ============================================================================================
    // SD Card Wrapper
    // ============================================================================================

    FATFS * fs_;

    /** Getters for the state shared between the SD card class wrapper and the implementation here
     */
    SD::Status SD::status() { return sdStatus_; }
    bool SD::ready() { return sdStatus_ == Status::Ready; }
    uint32_t SD::numBlocks() { return sdNumBlocks_; }

    /** Initializes the SD card into SPI mode. 
     
        The information in the method mostly comes from [1] and associated links, namely [2]. 

        [1] https://electronics.stackexchange.com/questions/602105/how-can-i-initialize-use-sd-cards-with-spi
        [2] http://elm-chan.org/docs/mmc/mmc_e.html#dataxfer
     */
    bool SD::initialize() {
        // to initialize, the SPI baudrate must be between 100-400kHz for the initialization
        spi_init(RP_SD_SPI, 200000);
        gpio_set_function(RP_PIN_SD_SCK, GPIO_FUNC_SPI);
        gpio_set_function(RP_PIN_SD_TX, GPIO_FUNC_SPI);
        gpio_set_function(RP_PIN_SD_RX, GPIO_FUNC_SPI);
        bi_decl(bi_3pins_with_func(RP_PIN_SD_SCK, RP_PIN_SD_TX, RP_PIN_SD_RX, GPIO_FUNC_SPI));
        // initialize the CS for manual control
        gpio::outputHigh(RP_PIN_SD_CSN);
        // create the buffer and fill it with 0xff
        uint8_t buffer[16];
        memset(buffer, 0xff, sizeof(buffer));
        // while CS is high, send at least 74 times 0xff
        spi_write_blocking(RP_SD_SPI, buffer, sizeof(buffer));
        // tell the card to go idle
        gpio::low(RP_PIN_SD_CSN);
        uint8_t status = sdSendCommand_(CMD0);
        if (status != SD_IDLE) {
            // TODO raise error
            return false;
        }
        // send interface condition to verify the communication. this is not supported by v1 cards so if we get illegal command continue with the initialization process
        status = sdSendCommand_(CMD8, buffer, 4);
        if (status != SD_ILLEGAL_COMMAND) {
            if (status != SD_IDLE) {
                // TODO raise error
                return false;
            }
            if (buffer[3] != 0xaa) {
                // TODO raise error
                return false;
            }
        }
        // read the OCR register and check the card supports the 3v3 voltage range. This is likely not necessary as every card should be 3v3 compatible, but just to be sure
        status = sdSendCommand_(CMD58, buffer, 4);
        if (status != SD_IDLE) {
            // TODO raise error
            return false;
        }
        if ((buffer[1] & 0x38) != 0x38) {
            // TODO raise error
            return false;
        }
        // and now get to the power on state, this needs to be done in a loop
        unsigned attempts = 0;
        while (true) {
            if (attempts++ > 100) {
                // TODO raise error
                return false;
            }
            if (sdSendCommand_(CMD55) != SD_IDLE) {
                // TOOD Raise error
                return false;
            }
            if (sdSendCommand_(ACMD41) == SD_NO_ERROR)
                break;
            cpu::delayMs(10);
        }
        // the card is now ready to be operated, send CMD58 again to verify the card power status bit
        status = sdSendCommand_(CMD58, buffer, 4);
        if (status != SD_NO_ERROR) {
            // TODO raise error
            return false;
        }
        if (! (buffer[0] & 0x80)) {
            // TODO raise error - card did not power up properly
            return false;
        }
        // increase speed to 20MHz
        spi_init(RP_SD_SPI, 20000000);
        // if the card is not SDHC, set block length to 512 bytes
        if (((buffer[0] & 64) == 0) && sdSendCommand_(CMD16) != SD_NO_ERROR) {
            // TODO raise error
            return false;
        }
        // determine the card capacity by reading the CSD
        if (sdSendCommand_(CMD9, buffer, 16) != SD_NO_ERROR) {
            // TODO raise error
            return false;
        }
        // get number of blocks from the CS card's capacity. For SDXC and SDHC cards, this is (CSIZE + 1) * 512KB, so we divide the CSIZE + 1 by 1024 to get size in 512 byte blocks
        sdNumBlocks_ = ((buffer[8] << 16) + (buffer[9] << 8) + buffer[10] + 1) * 1024;
        // and we are done, now try mounting the FatFS
        sdStatus_ = Status::Ready;
        fs_ = (FATFS*) malloc(sizeof(FATFS));
        if (f_mount(fs_, "", /* mount immediately */ 1) != FR_OK)
            sdStatus_ = Status::Unrecognized;
        return true;
    }

    void SD::enableUSBMsc(bool value) {
        if (value) {
            // if status is ready, unmount the default drive
            if (sdStatus_ ==  Status::Ready)
                f_unmount("");
            // if there is SD card (even unrecognized), enable the USB 
            if (sdStatus_ != Status::NotPresent) {
                sdStatus_ = Status::USB;
                // initialize the USB
                tud_init(BOARD_TUD_RHPORT);
            }
            
        } else {
            if (sdStatus_ == Status::USB) {
                // disable USB -- reset so that we can again detect DC charge
                memset(reinterpret_cast<uint8_t *>(usb_hw), 0, sizeof(*usb_hw));
                // reinitialize the SD card - the PC might have done things to the SD card that might caus it to be unrecognized
                sdStatus_ = Status::Ready;
                if (f_mount(fs_, "", 0) != FR_OK)
                    sdStatus_ = Status::Unrecognized;
            }
        }
    }
    // ============================================================================================
    // SD filesystem 
    // ============================================================================================

    uint64_t SD::getCapacity() { 
        return static_cast<uint64_t>(fs_->n_fatent - 2) * fs_->csize * 512;
    }

    uint64_t SD::getFreeCapacity() {
        DWORD n;
        FATFS * fs;
        f_getfree("", & n, &fs);
        return static_cast<uint64_t>(n) * fs_->csize * 512;
    }

    SD::Format SD::getFormatKind() {
        switch (fs_->fs_type) {
            case FS_FAT12:
                return Format::FAT12;
            case FS_FAT16:
                return Format::FAT16;
            case FS_FAT32:
                return Format::FAT32;
            case FS_EXFAT:
                return Format::EXFAT;
            default:
                return Format::Unrecognized;
        }
    }

    std::string SD::getLabel() {
        std::string result{' ', 12};
        f_getlabel("",result.data(), 0);
        return result;
    }

    // ============================================================================================
    // SD File
    // ============================================================================================

    SD::File SD::File::openRead(std::string const & path) {
        File result{};
        errorCode_ = f_open(& result.f_, path.c_str(), FA_READ);
        return result;
    }

    SD::File SD::File::openWrite(std::string const & path) {
        UNIMPLEMENTED;
    }

    uint32_t SD::File::size() const {
        return f_size(& f_);
    }

    uint32_t SD::File::read(uint8_t * buffer, uint32_t numBytes) {
        UINT bytesRead;
        errorCode_ = f_read(& f_, buffer, numBytes, & bytesRead);
        return bytesRead;
    }

    uint32_t SD::File::seek(uint32_t position) {
        errorCode_ = f_lseek(& f_, position);
        return (errorCode_ == FR_OK) ? position : 0;
    }

    // ============================================================================================
    // SD Folder
    // ============================================================================================

    SD::Folder SD::Folder::open(std::string const & path) {
        Folder result{};
        errorCode_ = f_opendir(& result.d_, path.c_str());
        return result;
    }

} // namespace rckid

// ================================================================================================
// FatFS device driver (using SD card)
// ================================================================================================

extern "C" {

    DSTATUS disk_status(BYTE pdrv) {
        ASSERT(pdrv == 0);
        return rckid::SD::ready() ? RES_OK : STA_NODISK;
    }

    DSTATUS disk_initialize(BYTE pdrv) {
        ASSERT(pdrv == 0);
        return rckid::SD::ready() ? RES_OK : STA_NODISK;
    }

    DRESULT disk_read(BYTE pdrv, BYTE * buff, LBA_t sector, UINT count) {
        ASSERT(pdrv == 0);
        if (! rckid::SD::ready())
            return RES_NOTRDY;
        return rckid::sdReadBlocks_(sector, buff, count) ? RES_OK : RES_ERROR;
    }

    DRESULT disk_write(BYTE pdrv, BYTE const * buff, LBA_t sector, UINT count) {
        ASSERT(pdrv == 0);
        if (! rckid::SD::ready())
            return RES_NOTRDY;
        return rckid::sdWriteBlocks_(sector, buff, count) ? RES_OK : RES_ERROR;
    }

    DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void * buff) {
        ASSERT(pdrv == 0);
        if (!rckid::SD::ready())
            return RES_NOTRDY;
        switch (cmd) {
            // no need to do anything for CTRL_SYNC as the FatFS exposed API is blocking
            case CTRL_SYNC:
                break;
            // returns the number of SD card blocks
            case GET_SECTOR_COUNT:
                *(LBA_t *)buff = rckid::SD::numBlocks();
                break;
            case GET_BLOCK_SIZE:
                *(DWORD *)buff = 1;
                break;
            // Sector size is fixed to 512 bytes so no need to set anything here
            case GET_SECTOR_SIZE:
            // CTRL_TRIM is not supported, all other ioctls are not supported as well
            case CTRL_TRIM:
            default:
                return RES_PARERR;
        }
        return RES_OK;
    }

    DWORD get_fattime() {
        UNIMPLEMENTED;
    }
}

#endif // ARCH_RP2040