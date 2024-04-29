#include "rckid.h"

#if (defined ARCH_RP2040)

#include "bsp/board.h"
#include "tusb_config.h"
#include "tusb.h"

#include <pico/time.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/pwm.h>
#include <hardware/dma.h>

#include "rckid.h"
#include "assets.h"
#include "graphics/ST7789.h"
#include "graphics/png.h"

#include "ST7789_rgb.pio.h"
#include "ST7789_rgb_double.pio.h"

#include "ltr390uv.h"
#include "bmi160.h"


platform::LTR390UV alsSensor_{};
platform::BMI160 accelerometer_{};

static constexpr unsigned TICK_DONE = 0;
static constexpr unsigned TICK_ALS = 1;
static constexpr unsigned TICK_UV = 2;
static constexpr unsigned TICK_ACCEL = 3;
static constexpr unsigned TICK_AVR = 4;

volatile unsigned tickInProgress_ = TICK_DONE;

namespace rckid {

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
        unsigned irqs = dma_hw->ints0;
        dma_hw->ints0 = irqs;
        // display
        if (irqs & ( 1u << ST7789::dma_))
            ST7789::irqHandler();

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
                } // fallthrough to default handler and to disabling the I2C comms
                default:
                    // we are done with the I2C transfer
                    break;
            }
        } else {
            ++stats::i2cErrors_;
        }
        // everything else than tx empty bits terminates the i2c transfer for the current tick
        tickInProgress_ = TICK_DONE;
        i2c0->hw->intr_mask = 0;
        i2c0->hw->enable = 0;
        stats::tickUpdateUs_ = uptimeUs() - stats::tickUpdateStart_;
    }

    void initialize() {
        board_init();
        // initialize the I2C bus
        i2c_init(i2c0, RP_I2C_BAUDRATE); 
        i2c0->hw->intr_mask = 0;
        gpio_set_function(RP_PIN_SDA, GPIO_FUNC_I2C);
        gpio_set_function(RP_PIN_SCL, GPIO_FUNC_I2C);
        // Make the I2C pins available to picotool
        bi_decl(bi_2pins_with_func(RP_PIN_SDA, RP_PIN_SCL, GPIO_FUNC_I2C));  
        // TODO serial if necessary
        tud_init(BOARD_TUD_RHPORT);
        // set the single DMA IRQ 0 handler reserved for the SDK
        irq_set_exclusive_handler(DMA_IRQ_0, irqDMADone_);
        irq_set_exclusive_handler(I2C0_IRQ, irqI2CDone_);
        irq_set_enabled(I2C0_IRQ, true);
        // make the I2C IRQ priority larger than that of the DMA (0x80) to ensure that I2C comms do not have to wait for render done if preparing data takes longer than sending them
        irq_set_priority(I2C0_IRQ, 0x40); 
        // initialize the display
        ST7789::initialize();
        setBrightness(128);
        setButtonsRainbow(16);
        // initialize accelerometer & ALS sensor peripherals
        accelerometer_.initialize();
        alsSensor_.initialize();
        alsSensor_.startALS();
    }

    void yield() {
        tight_loop_contents();
        tud_task();
    }

   void tick() {
        MEASURE_TIME(stats::tickUs_, 
            while (tickInProgress_ != TICK_DONE)
                yield();
            stats::tickUpdateStart_ = uptimeUs();
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
            // make the TX_EMPTY irq fire only when the data is actually processed
            //i2c0->hw->con |= I2C_IC_CON_TX_EMPTY_CTRL_BITS;
            // enable the I2C
            i2c0->hw->intr_mask = I2C_IC_INTR_MASK_M_RX_FULL_BITS | I2C_IC_INTR_MASK_M_TX_ABRT_BITS;
            // set the interrupt 
            ++stats::ticks_;
        );
    }

    void powerOff() {
        /// TODO: make sure sd and other things are done first, only then poweroff
        DeviceWrapper::sendCommand(cmd::PowerOff{}); 
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
        setColumnRange(0, 319);
        setRowRange(0, 239);
        beginCommand(RAMWR);
        gpio_put(RP_PIN_DISP_DCX, true);
        PNG png = PNG::fromBuffer(assets::Logo16);
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
            irqReady_ = false;
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

    void ST7789::irqHandler() {
        if (cb_()) {
            updating_ = false;
            stats::displayUpdateUs_ = static_cast<unsigned>(uptimeUs() - stats::displayUpdateStart_);
        }
    }
}

#endif // ARCH_RP2040