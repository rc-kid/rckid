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

#include "sensors.h"
#include "ST7789_rgb.pio.h"
#include "ST7789_rgb_double.pio.h"
#include "rckid/graphics/framebuffer.h"
#include "fonts/Iosevka_Mono6pt7b.h"



extern uint8_t __StackLimit, __bss_end__;

namespace rckid {

    void start() {
        // FIXME for reasons I do not completely understand, the board init must be before the other calls, or the device hangs? 
        board_init();
        Device::initialize();
        // initialize the display
        ST7789::initialize();

        int errorCode = setjmp(rckid::Device::fatalError_);
        if (errorCode != 0) 
            rckid::Device::BSOD(errorCode);
        rckid_main();
    }

    void yield() {
        tud_task();
        Audio::processEvents();
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

    // 

    void cpuOverclock(unsigned hz, bool overvolt) {
        if (overvolt) {
            vreg_set_voltage(VREG_VOLTAGE_1_20);
            sleep_ms(10);
        } else {
            // TODO non-overvolt                
        }
        Device::clockSpeed_ = hz;
        set_sys_clock_khz(hz / 1000, true);
    }


    // TODO super dumb nanosecond-like delay. Should be changed to take into account the actual cpu clock speed etc
    void sleep_ns(uint32_t ns) {
        //ns = ns * 4;
        while (ns >= 8) 
            ns -= 8;
    }

    // 4.4ms for system currently

    void Device::initialize() {
        // initialize the I2C bus
        i2c_init(i2c0, RP_I2C_BAUDRATE); 
        gpio_set_function(RP_PIN_SDA, GPIO_FUNC_I2C);
        gpio_set_function(RP_PIN_SCL, GPIO_FUNC_I2C);
        // Make the I2C pins available to picotool
        bi_decl(bi_2pins_with_func(RP_PIN_SDA, RP_PIN_SCL, GPIO_FUNC_I2C));  
        // TODO serial if necessary
        tud_init(BOARD_TUD_RHPORT);
        LOG("RCKid initialized");
        resetVRAM();
        BMI160::initialize();
        LTR390UV::initialize();
        LTR390UV::startALS();
    }

    void Device::tick() {
        ++ticks_;
        lastState_ = state_;
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

        BMI160::measure(accelState_);
        if (ticks_ % 8 == 0) {
            if ((ticks_ & 16) == 0) {
                lightALS_ = LTR390UV::measureALS();
                LTR390UV::startUV();
            } else {
                lightUV_ = LTR390UV::measureUV();
                LTR390UV::startALS();
            }
        }
    }

    void Device::BSOD(int code) {
        resetVRAM();
        FrameBuffer<ColorRGB> fb{Bitmap<ColorRGB>::inVRAM(320,240)};
        fb.setFg(ColorRGB::White());
        fb.setFont(Iosevka_Mono6pt7b);
        fb.setBg(ColorRGB::Blue());
        fb.fill();
        fb.textMultiline(0,0) << ":(\n\n"
            << "FATAL ERROR " << code << "\n\n"
            << "File: " << fatalErrorFile_ << "\n"
            << "Line: " << fatalErrorLine_; 
        ST7789::reset();
        ST7789::enterContinuousMode(ST7789::Mode::Single);
        fb.render();
        while(true) {
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
        //irq_set_exclusive_handler(DMA_IRQ_0, irqDMADone);
        irq_add_shared_handler(DMA_IRQ_0, irqDMADone,  PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
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
        setDisplayMode(DisplayMode::Natural);
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
        setDisplayMode(ST7789::DisplayMode::Native);
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
        setColorMode(ColorMode::RGB565);
        sendCommand(TEON, TE_VSYNC);
        sendCommand(SLPOUT);
        sleep_ms(150);
        sendCommand(DISPON);
        sleep_ms(150);
        //sendCommand(MADCTL, (uint8_t)(MADCTL_MV));
        //sendCommand(MADCTL, (uint8_t)(MADCTL_MY | MADCTL_MV ));
        //sendCommand(MADCTL, 0_u8);
        setDisplayMode(ST7789::DisplayMode::Native);
        sendCommand(INVON);
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

    void ST7789::enterContinuousMode(Rect rect, Mode mode) {
        leaveContinuousMode();
        setColumnRange(rect.top(), rect.height() - 1);
        setRowRange(rect.left(), rect.width() - 1);
        // start the continuous RAM write command
        beginCommand(RAMWR);
        gpio_put(RP_PIN_DISP_DCX, true);
        // initialize the corresponding PIO program
        switch (mode) {
            case Mode::Single:
                ST7789_rgb_program_init(pio_, sm_, offsetSingle_, RP_PIN_DISP_WRX, RP_PIN_DISP_DB8);
                break;
            case Mode::Double:
                ST7789_rgb_double_program_init(pio_, sm_, offsetDouble_, RP_PIN_DISP_WRX, RP_PIN_DISP_DB8);
                break;
        }
        // and start the pio
        pio_sm_set_enabled(pio_, sm_, true);
    }

    void ST7789::leaveContinuousMode() {
        pio_sm_set_enabled(pio_, sm_, false);
        end(); // end the RAMWR command
        initializePinsBitBang();
    }

    void ST7789::initializePinsBitBang() {
        uint32_t outputPinsMask = (1 << RP_PIN_DISP_WRX) | (0xff << RP_PIN_DISP_DB8); // DB8..DB15 are consecutive
        gpio_init_mask(outputPinsMask);
        gpio_set_dir_masked(outputPinsMask, outputPinsMask);
        //gpio_put_masked(outputPinsMask, false);
        gpio_put(RP_PIN_DISP_WRX, false);
    }

    void __not_in_flash_func(ST7789::irqDMADone)() {
        if(dma_channel_get_irq0_status(dma_)) {
            dma_channel_acknowledge_irq0(dma_); // clear the flag
            if (cb_)
                cb_();
            else 
                updating_ = false;
        }
    }

    // ============================================================================================
    // Audio Driver
    // ============================================================================================

    void Audio::initialize() {
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

    void Audio::startPlayback(SampleRate rate, uint16_t * buffer, size_t bufferSize, CallbackPlay cb) {
        configureDMA(dma0_, dma1_, buffer, bufferSize / 2); 
        configureDMA(dma1_, dma0_, buffer + bufferSize / 2, bufferSize / 2);
        cbPlay_ = cb;
        buffer_ = buffer;
        bufferSize_ = bufferSize;
        setSampleRate(rate);
        // add shared IRQ handler on the DMA done
        irq_add_shared_handler(DMA_IRQ_0, irqDMADone,  PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
        status_ |= PLAYBACK;
        status_ &= ~ BUFFER_INDEX; // transferring the first half of the buffer
        dma_channel_start(dma0_);
        //dma_channel_transfer_from_buffer_now(dma0_, buffer, bufferSize_ / 2);
        pwm_set_enabled(PWM_SLICE, true);
        //pwm_set_enabled(TIMER_SLICE, true);
    }

    void Audio::stopPlayback() {
        pwm_set_enabled(PWM_SLICE, false);
        pwm_set_enabled(TIMER_SLICE, false);
        irq_remove_handler(DMA_IRQ_0, irqDMADone);
        dma_channel_abort(dma0_);
        dma_channel_abort(dma1_);
        status_ &= ~PLAYBACK;
    }

    void Audio::startRecording(SampleRate rate) {

    }

    void Audio::stopRecording() {

    }

    void Audio::processEvents() {
        if (status_ & CALLBACK) {
            status_ &= ~CALLBACK;
            if (status_ & PLAYBACK) {
                cbPlay_(buffer_ + ((status_ & BUFFER_INDEX) ? 0 : bufferSize_ / 2), bufferSize_ / 2);
            } else if (status_ & RECORDING) {
                UNIMPLEMENTED; 
            }
        }
    }

    void Audio::configureDMA(int dma, int other, uint16_t const * buffer, size_t bufferSize) {
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

    void Audio::setSampleRate(uint16_t rate) {
        // since even the lower frequency (8kHz) can be obtained with a 250MHz (max) sys clock and 16bit wrap, we keep clkdiv at 1 and only change wrap here
        pwm_set_wrap(TIMER_SLICE, (cpuClockSpeed() * 10 / rate + 5) / 10); 
    }

    void __not_in_flash_func(Audio::irqDMADone)() {
        if (dma_channel_get_irq0_status(dma0_)) {
            dma_channel_acknowledge_irq0(dma0_); // clear the flag
            // update dma0 address 
            dma_channel_set_read_addr(dma0_, buffer_, false);
            // we finished sending first part of buffer and are now sending the second part
            status_ &= ~BUFFER_INDEX;
            status_ |= CALLBACK;
        } else if (dma_channel_get_irq0_status(dma1_)) {
            dma_channel_acknowledge_irq0(dma1_); // clear the flag
            // update dma1 address 
            dma_channel_set_read_addr(dma1_, buffer_ + bufferSize_ / 2, false);
            // and set the callback appropriately
            status_ |= BUFFER_INDEX | CALLBACK;
        }
        /*
        if(dma_channel_get_irq0_status(dma_)) {
            dma_channel_acknowledge_irq0(dma_); // clear the flag
            if (status_ & PLAYBACK) {
                status_ |= CALLBACK;
                if (status_ & BUFFER_INDEX) {
                    // transfer first half of the buffer
                    status_ &= ~ BUFFER_INDEX;
                    dma_channel_transfer_from_buffer_now(dma_, buffer_, bufferSize_ / 2);
                } else {
                    // transfer the second half of the buffer
                    status_ |= BUFFER_INDEX;
                    // we add 16bit integers, but copy 32bit numbers (stereo)
                    dma_channel_transfer_from_buffer_now(dma_, buffer_ + bufferSize_, bufferSize_ / 2);
                }
            } else if (status_ & RECORDING) {
                status_ |= CALLBACK;
            }
        } */
    }

    // ============================================================================================
    // BMI160 Accelerometer & Gyro + BMI150 Magnetometer driver
    // ============================================================================================

    bool BMI160::isPresent() {
        if (!i2cDevicePresent(I2C_ADDRESS))
            return false;
        return i2cRegisterRead8(I2C_ADDRESS, REG_CHIP_ID) == CHIP_ID;
    }

    void BMI160::initialize() {
        // TODO the initialization does not work atm for magnetometer, maybe it needs to be initialized first using the manual interface?
        i2cRegisterWrite8(I2C_ADDRESS, REG_CMD, CMD_ACCEL_ON);
        delay_ms(5);
        i2cRegisterWrite8(I2C_ADDRESS, REG_CMD, CMD_GYRO_ON);
        delay_ms(90);
    }

    void BMI160::measure(State & state) {
        // TODO make this non-blocking
        i2c_write_blocking(i2c0, I2C_ADDRESS, & REG_DATA, 1, true);
        i2c_read_blocking(i2c0, I2C_ADDRESS, reinterpret_cast<uint8_t *>(& state), sizeof(State), false);
    }

    // ============================================================================================
    // LTR390UV Ambient & UV light Sensor driver
    // ============================================================================================

    bool LTR390UV::isPresent() {
        if (!i2cDevicePresent(I2C_ADDRESS))
            return false;
        return (i2cRegisterRead8(I2C_ADDRESS, REG_PART_ID) & 0xf0) == PART_ID;
    }

    void LTR390UV::initialize() {
        i2cRegisterWrite8(I2C_ADDRESS, REG_MEAS_RATE, MEAS_RATE_16bit_25ms);
        i2cRegisterWrite8(I2C_ADDRESS, REG_GAIN, GAIN_18);
    }

    void LTR390UV::startALS() {
        i2cRegisterWrite8(I2C_ADDRESS, REG_CTRL, CTRL_ALS_EN);
    }

    uint16_t LTR390UV::measureALS() {
        uint8_t data[3];
        i2c_write_blocking(i2c0, I2C_ADDRESS, & REG_DATA_ALS, 1, true);
        i2c_read_blocking(i2c0, I2C_ADDRESS, data, 3, false);
        return data[0] + data[1] * 256;
    }

    void LTR390UV::startUV() {
        i2cRegisterWrite8(I2C_ADDRESS, REG_CTRL, CTRL_UV_EN);
    }

    uint16_t LTR390UV::measureUV() {
        uint8_t data[3];
        i2c_write_blocking(i2c0, I2C_ADDRESS, & REG_DATA_UV, 1, true);
        i2c_read_blocking(i2c0, I2C_ADDRESS, data, 3, false);
        return data[0] + data[1] * 256;
    }

} // namespace rckid

void pio_set_clock_speed(PIO pio, unsigned sm, unsigned hz) {
    uint kHz = hz / 1000;
    uint clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS); // [kHz]
    uint clkdiv = (clk / kHz);
    uint clkfrac = (clk - (clkdiv * kHz)) * 256 / kHz;
    pio_sm_set_clkdiv_int_frac(pio, sm, clkdiv & 0xffff, clkfrac & 0xff);
} 

int main() {
    rckid::start();
    UNREACHABLE;
}


#endif // !LIBRCKID_MOCK