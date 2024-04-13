#if (! defined LIBRCKID_MOCK)

#include "bsp/board.h"
#include "tusb_config.h"
#include "tusb.h"

#include <pico/time.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/pwm.h>
#include <hardware/dma.h>

#include "common/config.h"
#include "rckid.h"
#include "ST7789.h"
#include "audio.h"
#include "app.h"

#include "images/logo-16.h"
#include "graphics/png.h"

#include "ST7789_rgb.pio.h"
#include "ST7789_rgb_double.pio.h"
#include "rckid/graphics/framebuffer.h"
#include "fonts/Iosevka_Mono6pt7b.h"

#include "ltr390uv.h"
#include "bmi160.h"

extern uint8_t __StackLimit, __bss_end__;

namespace rckid {

    void irqDMADone_();

    // fake allocation for the VRAM of properly set size
    uint8_t __attribute__((section (".vram"))) __vram__[RCKID_VRAM_SIZE];

    platform::LTR390UV alsSensor_{};
    platform::BMI160 accelerometer_{};

    void start() {
        // FIXME for reasons I do not completely understand, the board init must be before the other calls, or the device hangs? 
        board_init();
        Device::initialize();
        // initialize the display
        ST7789::initialize();
        setBrightness(128);

        int errorCode = setjmp(rckid::Device::fatalError_);
        if (errorCode != 0) 
            rckid::Device::BSOD(errorCode);
        rckid_main();
    }

    void yield() {
        tud_task();
        audio::processEvents();
    }

    Writer writeToUSBSerial() {
        return Writer{[](char x) {
            tud_cdc_write(& x, 1);
            if (x == '\n')
                tud_cdc_write_flush();            
        }};
    }

    void enableSerialPort() {
        stdio_uart_init_full(
            RP_DEBUG_UART, 
            RP_DEBUG_UART_BAUDRATE, 
            RP_DEBUG_UART_TX_PIN, 
            RP_DEBUG_UART_RX_PIN
        );
    }

    // power management ---------------------------------------------------------------------------

    void powerOff() {
        /// TODO: make sure sd and other things are done first, only then poweroff
        Device::sendCommand(cmd::PowerOff{}); 
    }

    // memory -------------------------------------------------------------------------------------

    size_t freeHeap() {
        size_t heapSize = &__StackLimit  - &__bss_end__;    
        return heapSize - mallinfo().uordblks;
    }

    void Device::initialize() {
        // initialize the I2C bus
        i2c_init(i2c0, RP_I2C_BAUDRATE); 
        gpio_set_function(RP_PIN_SDA, GPIO_FUNC_I2C);
        gpio_set_function(RP_PIN_SCL, GPIO_FUNC_I2C);
        // Make the I2C pins available to picotool
        bi_decl(bi_2pins_with_func(RP_PIN_SDA, RP_PIN_SCL, GPIO_FUNC_I2C));  
        // TODO serial if necessary
        tud_init(BOARD_TUD_RHPORT);
        // set the single DMA IRQ 0 handler reserved for the SDK
        irq_set_exclusive_handler(DMA_IRQ_0, irqDMADone_);
        LOG("RCKid initialized");
        resetVRAM();
        accelerometer_.initialize();
        alsSensor_.initialize();
        alsSensor_.startALS();
    }

    void Device::tick() {
        ++ticks_;
        lastState_ = state_.state;
        // query the AVR for the status bytes, first set the address
        i2c0->hw->enable = 0;
        i2c0->hw->tar = AVR_I2C_ADDRESS;
        i2c0->hw->enable = 1;
        // add commands for getting the blocks
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS; // 1 for read
        i2c0->hw->data_cmd = I2C_IC_DATA_CMD_CMD_BITS | I2C_IC_DATA_CMD_STOP_BITS; // 1 for read, stop
        while (i2c0->hw->rxflr != 6) 
            yield();
        uint8_t * raw = reinterpret_cast<uint8_t*>(&state_);
        for (int i = 0; i < 6; ++i)
            *(raw++) = i2c0->hw->data_cmd;
        // i2c_read_blocking(i2c0, AVR_I2C_ADDRESS, (uint8_t *)& state_, sizeof(State), false);

        platform::BMI160::State aState;
        accelerometer_.measure(aState);
        accelX_ = aState.accelX;
        accelY_ = aState.accelY;
        accelZ_ = aState.accelZ;
        gyroX_ = aState.gyroX;
        gyroY_ = aState.gyroY;
        gyroZ_ = aState.gyroZ;

        if (ticks_ % 8 == 0) {
            if ((ticks_ & 16) == 0) {
                lightALS_ = alsSensor_.measureALS();
                alsSensor_.startUV();
            } else {
                lightUV_ = alsSensor_.measureUV();
                alsSensor_.startALS();
            }
        }
    }

    void Device::BSOD(int code) {
        resetVRAM();
        FrameBuffer<ColorRGB> fb{Bitmap<ColorRGB>{320,240, MemArea::VRAM}};
        fb.setFg(ColorRGB::White());
        fb.setFont(Iosevka_Mono6pt7b);
        fb.setBg(ColorRGB::Blue());
        fb.fill();
        fb.textMultiline(0,0) << ":(\n\n"
            << "FATAL ERROR " << code << "\n\n"
            << "File: " << fatalErrorFile_ << "\n"
            << "Line: " << fatalErrorLine_; 
        ST7789::reset();
        ST7789::enterContinuousUpdate();
        fb.render();
        while(true) {
        };
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
        //setDisplayMode(DisplayMode::Natural);
        setColumnRange(0, 319);
        setRowRange(0, 239);
        beginCommand(RAMWR);
        gpio_put(RP_PIN_DISP_DCX, true);
        PNG png = PNG::fromBuffer(Logo16, sizeof(Logo16));
        png.decode([&](ColorRGB * line, int lineNum, int lineWidth){
            uint8_t const * raw = reinterpret_cast<uint8_t *>(line);
            for (int i = 0; i < lineWidth; ++i) {
                sendByte(raw[1]);
                sendByte(raw[0]);
                raw += 2;
            }
        });
        end();
        configure(DisplayMode::Native_RGB565);
#endif
    }

    void ST7789::reset() {
        dma_channel_abort(dma_);
        pio_sm_set_enabled(pio_, sm_, false);
        updating_ = false;

        gpio_init(RP_PIN_DISP_TE);
        gpio_set_dir(RP_PIN_DISP_TE, GPIO_IN);
        gpio_init(RP_PIN_DISP_DCX);
        gpio_set_dir(RP_PIN_DISP_DCX, GPIO_OUT);
        gpio_init(RP_PIN_DISP_CSX);
        gpio_set_dir(RP_PIN_DISP_CSX, GPIO_OUT);
        gpio_put(RP_PIN_DISP_CSX, true);

        initializePinsBitBang();

        // TODO check the init sequence
        sendCommand(SWRESET);
        sleep_ms(150);
        sendCommand(VSCSAD, (uint8_t)0);
        configure(DisplayMode::Native_RGB565);
        sendCommand(TEON, TE_VSYNC);
        sendCommand(SLPOUT);
        sleep_ms(150);
        sendCommand(DISPON);
        sleep_ms(150);
        //sendCommand(MADCTL, (uint8_t)(MADCTL_MV));
        //sendCommand(MADCTL, (uint8_t)(MADCTL_MY | MADCTL_MV ));
        //sendCommand(MADCTL, 0_u8);
        //setDisplayMode(ST7789::DisplayMode::Native);
        sendCommand(INVON);
    }

    void ST7789::configure(DisplayMode mode) {
        if (continuousUpdateActive())
            leaveContinuousUpdate();
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
        for (size_t i = 0, e =320 * 240; i < e; ++i) {
            sendByte((x >> 8) & 0xff);
            sendByte(x & 0xff);
        }
        end();
    }

    bool ST7789::continuousUpdateActive() {
        return pio_sm_is_enabled(pio_, sm_);
    }

    void ST7789::enterContinuousUpdate(Rect rect) {
        leaveContinuousUpdate();
        setColumnRange(rect.top(), rect.bottom() - 1);
        setRowRange(rect.left(), rect.right() - 1);
        // start the continuous RAM write command
        beginCommand(RAMWR);
        gpio_put(RP_PIN_DISP_DCX, true);
        // initialize the corresponding PIO program
        switch (displayMode_) {
            case DisplayMode::Native_RGB565:
                ST7789_rgb_program_init(pio_, sm_, offsetSingle_, RP_PIN_DISP_WRX, RP_PIN_DISP_DB8);
                break;
            case DisplayMode::Native_2X_RGB565:
                ST7789_rgb_double_program_init(pio_, sm_, offsetDouble_, RP_PIN_DISP_WRX, RP_PIN_DISP_DB8);
                break;
            default:
                UNREACHABLE;
        }
        // and start the pio
        pio_sm_set_enabled(pio_, sm_, true);
    }

    void ST7789::leaveContinuousUpdate() {
        if (continuousUpdateActive()) {
            // temporarily disable the IRQ on the DMA to ignore spurious complete events when aborting
            dma_channel_set_irq0_enabled(dma_, false);
            dma_channel_abort(dma_);
            dma_channel_set_irq0_enabled(dma_, true);
            updating_ = false;
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

    // ============================================================================================
    // Audio Driver
    // ============================================================================================

    void audio::initialize() {
        // configure the PWM pins
        gpio_set_function(RP_PIN_PWM_RIGHT, GPIO_FUNC_PWM);
        gpio_set_function(RP_PIN_PWM_LEFT, GPIO_FUNC_PWM);
        // configure the audio out PWM
        //pwm_set_wrap(PWM_SLICE, 254); // set wrap to 8bit sound levels
        pwm_set_wrap(PWM_SLICE, 4096); // set wrap to 8bit sound levels
        
        pwm_set_clkdiv(PWM_SLICE, 1.38); // 12bit depth @ 44.1kHz and 250MHz
        // set the PWM output to count to 256 441000 times per second
        //pwm_set_clkdiv(PWM_SLICE, 11.07);
        //pwm_set_clkdiv(PWM_SLICE, 61.03);
        // acquire and configure the DMA
        dma0_ = dma_claim_unused_channel(true);
        dma1_ = dma_claim_unused_channel(true);
        /*
        auto dmaConf = dma_channel_get_default_config(dma_);
        channel_config_set_transfer_data_size(& dmaConf, DMA_SIZE_32); // transfer 32 bytes (16 per channel, 2 channels)
        channel_config_set_read_increment(& dmaConf, true);
        channel_config_set_dreq(& dmaConf, pwm_get_dreq(TIMER_SLICE)); // DMA is driver by the sample rate PWM
        //channel_config_set_dreq(&dmaConf, pwm_get_dreq(PWM_SLICE));
        dma_channel_configure(dma_, & dmaConf, &pwm_hw->slice[PWM_SLICE].cc, nullptr, 0, false);
        // enable IRQ0 on the DMA channel (shared with SD card and display)
        dma_channel_set_irq0_enabled(dma_, true);
        */
    }

    void audio::startPlayback(SampleRate rate, uint16_t * buffer, size_t bufferSize, CallbackPlay cb) {
        configureDMA(dma0_, dma1_, buffer, bufferSize / 2); 
        configureDMA(dma1_, dma0_, buffer + bufferSize / 2, bufferSize / 2);
        cbPlay_ = cb;
        buffer_ = buffer;
        bufferSize_ = bufferSize;
        setSampleRate(rate);
        status_ |= PLAYBACK;
        status_ &= ~ BUFFER_INDEX; // transferring the first half of the buffer
        dma_channel_start(dma0_);
        //dma_channel_transfer_from_buffer_now(dma0_, buffer, bufferSize_ / 2);
        pwm_set_enabled(PWM_SLICE, true);
        //pwm_set_enabled(TIMER_SLICE, true);
    }

    void audio::stopPlayback() {
        pwm_set_enabled(PWM_SLICE, false);
        pwm_set_enabled(TIMER_SLICE, false);
        //irq_remove_handler(DMA_IRQ_0, irqDMADone);
        dma_channel_abort(dma0_);
        dma_channel_abort(dma1_);
        status_ &= ~PLAYBACK;
    }

    void audio::startRecording(SampleRate rate) {

    }

    void audio::stopRecording() {

    }

    void audio::processEvents() {
        if (status_ & CALLBACK) {
            status_ &= ~CALLBACK;
            if (status_ & PLAYBACK) {
                cbPlay_(buffer_ + ((status_ & BUFFER_INDEX) ? 0 : bufferSize_ / 2), bufferSize_ / 2);
            } else if (status_ & RECORDING) {
                UNIMPLEMENTED; 
            }
        }
    }

    void audio::configureDMA(int dma, int other, uint16_t const * buffer, size_t bufferSize) {
        auto dmaConf = dma_channel_get_default_config(dma);
        channel_config_set_transfer_data_size(& dmaConf, DMA_SIZE_32); // transfer 32 bytes (16 per channel, 2 channels)
        channel_config_set_read_increment(& dmaConf, true);
        //channel_config_set_dreq(& dmaConf, pwm_get_dreq(TIMER_SLICE)); // DMA is driver by the sample rate PWM
        channel_config_set_dreq(&dmaConf, pwm_get_dreq(PWM_SLICE));
        channel_config_set_chain_to(& dmaConf, other); // chain to the other channel
        dma_channel_configure(dma, & dmaConf, &pwm_hw->slice[PWM_SLICE].cc, buffer, bufferSize / 2, false); // the buffer consists of stereo samples, (32bits), i.e. buffer size / 2
        // enable IRQ0 on the DMA channel (shared with SD card and display)
        dma_channel_set_irq0_enabled(dma, true);
    }

    void audio::setSampleRate(uint16_t rate) {
        // since even the lower frequency (8kHz) can be obtained with a 250MHz (max) sys clock and 16bit wrap, we keep clkdiv at 1 and only change wrap here
        pwm_set_wrap(TIMER_SLICE, (cpu::clockSpeed() * 10 / rate + 5) / 10); 
    }

    // ============================================================================================
    // DMA handler
    // ============================================================================================


     /** Shared DMA interrupt handler for all SDK processes. 
     
        Sharing the handler makes the dispatch a bit faster. 
     */
    void __not_in_flash_func(irqDMADone_)() {
        // display
        if (dma_channel_get_irq0_status(ST7789::dma_)) {
            dma_channel_acknowledge_irq0(ST7789::dma_); // clear the flag
            if (ST7789::cb_()) {
                ST7789::updating_ = false;
                stats::displayUpdateUs_ = static_cast<unsigned>(uptimeUs() - stats::updateStart_);
            }
        }
        // audio
        if (dma_channel_get_irq0_status(audio::dma0_)) {
            dma_channel_acknowledge_irq0(audio::dma0_); // clear the flag
            // update dma0 address 
            dma_channel_set_read_addr(audio::dma0_, audio::buffer_, false);
            // we finished sending first part of buffer and are now sending the second part
            audio::status_ &= ~audio::BUFFER_INDEX;
            audio::status_ |= audio::CALLBACK;
        } else if (dma_channel_get_irq0_status(audio::dma1_)) {
            dma_channel_acknowledge_irq0(audio::dma1_); // clear the flag
            // update dma1 address 
            dma_channel_set_read_addr(audio::dma1_, audio::buffer_ + audio::bufferSize_ / 2, false);
            // and set the callback appropriately
            audio::status_ |= audio::BUFFER_INDEX | audio::CALLBACK;
        }

    }

} // namespace rckid

int main() {
    rckid::start();
    UNREACHABLE;
}


#endif // !LIBRCKID_MOCK