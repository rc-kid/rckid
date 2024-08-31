#include "ST7789.h"
#include "ST7789_rgb.pio.h"
#include "ST7789_rgb_double.pio.h"

namespace rckid {

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
    }

    void ST7789::reset() {
        // reset updating - forcefully since we are resetting the display anyways
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
        setDisplayMode(DisplayMode::Native);
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
        updateRegion_ = Rect::WH(320, 240);
    }

    void ST7789::setDisplayMode(DisplayMode mode) {
        enterCommandMode();
        switch (mode) {
            case DisplayMode::Native:
            case DisplayMode::NativeDouble:
                sendCommand(COLMOD, COLMOD_565);
                sendCommand(MADCTL, 0_u8);
                break;
            case DisplayMode::Natural:
            case DisplayMode::NaturalDouble:
                sendCommand(COLMOD, COLMOD_565);
                sendCommand(MADCTL, static_cast<uint8_t>(MADCTL_MY | MADCTL_MV));
                break;
            case DisplayMode::Off:
                UNIMPLEMENTED;
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
        mode_ = mode;
    }

    void ST7789::setUpdateRegion(Rect rect) {
        enterCommandMode();
        switch (mode_) {
            case DisplayMode::Native:
                setColumnRange(rect.top(), rect.bottom() - 1);
                setRowRange(rect.left(), rect.right() - 1);
                break;
            case DisplayMode::NativeDouble:
                UNIMPLEMENTED;
                break;
            case DisplayMode::Natural:
                setRowRange(rect.top(), rect.bottom() - 1);
                setColumnRange(rect.left(), rect.right() - 1);
                break;
            case DisplayMode::NaturalDouble:
                UNIMPLEMENTED;
                break;
            default:
                UNREACHABLE;
        }
    }

    void ST7789::enterCommandMode() {
        if (pio_sm_is_enabled(pio_, sm_)) {
            // let the last display update finish (blocking)
            waitUpdateDone();  
            cb_ = nullptr; // to be sure
            pio_sm_set_enabled(pio_, sm_, false);
            end(); // end the RAMWR command
            // mark pins at bitbanged
            initializePinsBitBang();
        }
    }

    void ST7789::enterUpdateMode() {
        if (!pio_sm_is_enabled(pio_, sm_)) {
            beginCommand(RAMWR);
            gpio_put(RP_PIN_DISP_DCX, true);
            // initialize the corresponding PIO program
            switch (mode_) {
                case DisplayMode::Native:
                case DisplayMode::Natural:
                    ST7789_rgb_program_init(pio_, sm_, offsetSingle_, RP_PIN_DISP_WRX, RP_PIN_DISP_DB8);
                    break;
                case DisplayMode::NativeDouble:
                case DisplayMode::NaturalDouble:
                    ST7789_rgb_double_program_init(pio_, sm_, offsetDouble_, RP_PIN_DISP_WRX, RP_PIN_DISP_DB8);
                    break;
                default:
                    UNREACHABLE;
            }
            // and start the pio
            pio_sm_set_enabled(pio_, sm_, true);
        }
    }


    //void ST7789::beginDMAUpdate() {

    //}

    //void ST7789::endDMAUpdate() {

    //}

    void ST7789::initializePinsBitBang() {
        uint32_t outputPinsMask = (1 << RP_PIN_DISP_WRX) | (0xff << RP_PIN_DISP_DB8); // DB8..DB15 are consecutive
        gpio_init_mask(outputPinsMask);
        gpio_set_dir_masked(outputPinsMask, outputPinsMask);
        //gpio_put_masked(outputPinsMask, false);
        gpio_put(RP_PIN_DISP_WRX, false);
    }

    void ST7789::irqHandler() {
        if (cb_) 
            cb_();
        if (updating_ > 0)
            --updating_;
       //if (updating_ == 0 || (--updating_ == 0))
        //    stats::displayUpdateUs_ = static_cast<unsigned>(uptimeUs() - stats::displayUpdateStart_);
    }

} // namespace rckid