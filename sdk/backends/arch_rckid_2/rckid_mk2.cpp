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

#include "avr/src/state.h"
#include "avr/src/commands.h"

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
        State state_; 
        State lastState_;
        uint8_t ticks_ = 0;


        namespace audio {

            uint8_t volume_ = 128;
            bool playback_ = false;
            uint dma0_ = 0;
            uint dma1_ = 0;
            DoubleBuffer * playbackBuffer_;

        }

    }

    /** Sends given I2C command to the AVR. 
     */
    template<typename T>
    static void sendCommand(T const & cmd) {
        i2c_write_blocking(i2c0, I2C_AVR_ADDRESS, (uint8_t const *) & cmd, sizeof(T), false);
    }    

    void __not_in_flash_func(i2cFillAVRTxBlocks)() {
        i2c0->hw->enable = 0;
        i2c0->hw->tar = I2C_AVR_ADDRESS;
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
                    lastState_ = state_;
                    uint8_t * raw = reinterpret_cast<uint8_t*>(&state_);
                    for (int i = 0; i < 8; ++i)
                        *(raw++) = i2c0->hw->data_cmd;
                    // update battery level gauge
                    /*
                    unsigned battPct = vBatt();
                    if (battPct <= VCC_CRITICAL_THRESHOLD)
                        battPct = 0;
                    else if (battPct >= VBATT_FULL_THRESHOLD)
                        battPct = 100;
                    else 
                        battPct = (battPct - VCC_CRITICAL_THRESHOLD) * 100 / (VBATT_FULL_THRESHOLD - VCC_CRITICAL_THRESHOLD);
                    DeviceWrapper::batteryLevel_ = battPct;
                    */
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
        i2c0->hw->enable = 0;
    }


    /** Audio DMA playback handler.
        
     */
    void audioPlaybackDMA(uint finished, uint other) {
        // reconfigure the currently finished buffer to start from the current front buffer (will be back after the swap) - note the other dma has already been started by the finished one
        dma_channel_set_read_addr(finished, audio::playbackBuffer_->getFrontBuffer(), false);
        // swap the buffers (this also calls the callback that fills the front part of the buffer with data)
        audio::playbackBuffer_->swap();
        // finally, while in the IRQ we must ensure that the data filled by the app in the callback conforms to what we expect. This means adjusting the volume and converting from int16_t to uint16_t centered at 32768
        // also for the basic PWM out w/o sigma delta, this also means loweing the resolution to 12 bits only
        
        // TODO
        //int16_t * buf = audioPlaybackBuffer_->getFrontBuffer();
        //for (uint32_t i = 0; i < audioPlaybackBuffer_.size(); ++i) {

        //}

    }



    void __not_in_flash_func(irqDMADone_)() {
        //gpio::outputHigh(GPIO21);
        unsigned irqs = dma_hw->ints0;
        dma_hw->ints0 = irqs;
        if (audio::playback_) {
            if (irqs & (1u << audio::dma0_))
                audioPlaybackDMA(audio::dma0_, audio::dma1_);
            if (irqs & (1u << audio::dma1_))
                audioPlaybackDMA(audio::dma1_, audio::dma0_);
        }
        // for audio, reset the DMA start address to the beginning of the buffer and tell the stream to refill
//        if (irqs & (1u << audio::dma0_))
//            audio::irqHandler1();
//        if (irqs & (1u << audio::dma1_))
//            audio::irqHandler2();
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

    bool buttonState(Btn b, State & state) {
        switch (b) {
            case Btn::Up: return state.btnUp();
            case Btn::Down: return state.btnDown();
            case Btn::Left: return state.btnLeft();
            case Btn::Right: return state.btnRight();
            case Btn::A: return state.btnA();
            case Btn::B: return state.btnB();
            case Btn::Select: return state.btnSel();
            case Btn::Start: return state.btnStart();
            case Btn::VolumeUp: return state.btnVolUp();
            case Btn::VolumeDown: return state.btnVolDown();
            case Btn::Home: return state.btnHome();
            default:
                UNREACHABLE;
        }
    }


    bool btnDown(Btn b) {
        return buttonState(b, state_);

    }

    bool btnPressed(Btn b) {
        return buttonState(b, state_) && ! buttonState(b, lastState_);

    }

    bool btnReleased(Btn b) {
        return ! buttonState(b, state_) && buttonState(b, lastState_);
    }

    int16_t accelX() { return 0; }
    int16_t accelY() { return 0; }
    int16_t accelZ() { return 0; }

    int16_t gyroX() { return 0; }
    int16_t gyroY() { return 0; }
    int16_t gyroZ() { return 0; }

    // power management

    void sleep() {
        UNIMPLEMENTED;
    }

    bool charging() { 
        return state_.charging();
    }

    bool dcPower() {
        return state_.dcPower();
    }

    unsigned vBatt() {
        return state_.vBatt();
    }

    unsigned batteryLevel() {
        // TODO change this to sth meaningful
        return 67;
    }

    // display

    DisplayMode displayMode() { 
        return ST7789::displayMode();
     }

    void displaySetMode(DisplayMode mode) {
        ST7789::setDisplayMode(mode);
    }

    uint8_t displayBrightness() { 
        return state_.brightness();
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

    // audio

    void audioConfigurePlaybackDMA(int dma, int other, uint8_t * buffer, size_t bufferSize) {
        auto dmaConf = dma_channel_get_default_config(dma);
        channel_config_set_transfer_data_size(& dmaConf, DMA_SIZE_32); // transfer 32 bits (16 per channel, 2 channels)
        channel_config_set_read_increment(& dmaConf, true);  // increment on read
        channel_config_set_dreq(&dmaConf, pwm_get_dreq(RP_AUDIO_PWM_SLICE));// DMA is driven by the PWM slice overflowing
        channel_config_set_chain_to(& dmaConf, other); // chain to the other channel
        dma_channel_configure(dma, & dmaConf, &pwm_hw->slice[RP_AUDIO_PWM_SLICE].cc, buffer, bufferSize / 2, false); // the buffer consists of stereo samples, (32bits), i.e. buffer size / 2
        // enable IRQ0 on the DMA channel (shared with other framework DMA uses such as the display or the SD card)
        dma_channel_set_irq0_enabled(dma, true);
    }


    void audioEnable() {
        sendCommand(cmd::AudioEnabled{});

    }

    void audioDisable() {
        sendCommand(cmd::AudioDisabled{});
    }

    bool audioHeadphones() {
        return state_.headphones();
    }

    uint8_t audioVolume() {
        return audio::volume_;
    }

    void audioSetVolume(uint8_t value) {
        audio::volume_ = value;

    }

    void audioPlay(DoubleBuffer & data, uint32_t bitrate) {
        // stop the previous playback if any
        if (audio::playback_)
            audioStop();
        // set the audio playback buffer
        audio::playbackBuffer_ = & data;
        // set the left and right pins to be used by the PWM
        gpio_set_function(RP_PIN_PWM_RIGHT, GPIO_FUNC_PWM); 
        gpio_set_function(RP_PIN_PWM_LEFT, GPIO_FUNC_PWM);
        // initialize the DMA channels 
        audioConfigurePlaybackDMA(audio::dma0_, audio::dma1_, data.getBackBuffer(), RP_AUDIO_BUFFER_SIZE);
        audioConfigurePlaybackDMA(audio::dma1_, audio::dma0_, data.getFrontBuffer(), RP_AUDIO_BUFFER_SIZE);
        dma_channel_start(audio::dma0_);
        // and finally the timers
        pwm_set_clkdiv(RP_AUDIO_PWM_SLICE, cpu::clockSpeed() / (4096.0 * bitrate));
        pwm_set_wrap(RP_AUDIO_PWM_SLICE, 4096); // set wrap to 12bit sound levels
        pwm_set_enabled(RP_AUDIO_PWM_SLICE, true);
    }

    void audioRecord(DoubleBuffer & data, uint32_t bitrate) {
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

    }

    // RGB LEDs

    void ledsOff() {
        sendCommand(cmd::RGBOff());
    }

    void ledSetEffect(Btn b, LEDEffect const & effect) {

    }

    void ledSetEffects(LEDEffect const & dpad, LEDEffect const & a, LEDEffect const & b, LEDEffect const & select, LEDEffect const & start) {

    }

    // Rumbler

    void rumble(uint8_t intensity, uint16_t duration, unsigned repetitions, uint16_t offDuration) {
        sendCommand(cmd::Rumbler(RumblerEffect(intensity, duration, offDuration, repetitions)));
    }

    // accelerated functions
    #include "rckid/accelerated.inc.h"
    MEM_FILL_8
    MEM_FILL_16
    MEM_FILL_32

}