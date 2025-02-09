#include "ST7789.h"
#include "ST7789_rgb.pio.h"
#include "ST7789_rgb_double.pio.h"

#include <rckid/graphics/png.h>
#include <rckid/assets/images.h>

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
        setDisplayMode(DisplayResolution::Full, DisplayRefreshDirection::Native);
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
        // and now do the png file
        // now clear the entire display black
#if (RCKID_SPLASHSCREEN_OFF)
        beginCommand(RAMWR);
        gpio_put(RP_PIN_DISP_DCX, true);
        for (size_t i = 0, e =320 * 240; i < e; ++i) {
            sendByte(0);
            sendByte(0);
        }
        gpio_put(RP_PIN_DISP_DCX, false);
        end();
#else
        setDisplayMode(DisplayResolution::Full, DisplayRefreshDirection::Natural);
        setUpdateRegion(updateRegion_);
        beginCommand(RAMWR);
        gpio_put(RP_PIN_DISP_DCX, true);
        PNG png = PNG::fromBuffer(assets::logo16);
        png.decode16([&](uint16_t * line, [[maybe_unused]] int lineNum, int lineWidth){
            for (int i = 0; i < lineWidth; ++i)
                sendWord(line[i]);
        });
        gpio_put(RP_PIN_DISP_DCX, false);
        end();
        setDisplayMode(DisplayResolution::Full, DisplayRefreshDirection::Native);
        setUpdateRegion(updateRegion_);
#endif
    }

    void ST7789::setDisplayMode(DisplayResolution res, DisplayRefreshDirection dir) {
        mode_ = static_cast<uint8_t>(res) | (static_cast<uint8_t>(dir) << 4); 
        enterCommandMode();
        switch (mode_) {
            case MODE_FULL_NATIVE:
            case MODE_HALF_NATIVE:
                sendCommand(COLMOD, COLMOD_565);
                sendCommand(MADCTL, 0_u8);
                break;
            case MODE_FULL_NATURAL:
            case MODE_HALF_NATURAL:
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
    }

    void ST7789::setUpdateRegion(Rect rect) {
        enterCommandMode();
        switch (mode_) {
            case MODE_FULL_NATIVE:
                setColumnRange(rect.top(), rect.bottom() - 1);
                setRowRange(rect.left(), rect.right() - 1);
                break;
            case MODE_HALF_NATIVE:
                UNIMPLEMENTED;
                break;
            case MODE_FULL_NATURAL:
                setRowRange(rect.top(), rect.bottom() - 1);
                setColumnRange(rect.left(), rect.right() - 1);
                break;
            case MODE_HALF_NATURAL:
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
                case MODE_FULL_NATIVE:
                case MODE_FULL_NATURAL:
                    ST7789_rgb_program_init(pio_, sm_, offsetSingle_, RP_PIN_DISP_WRX, RP_PIN_DISP_DB8);
                    break;
                case MODE_HALF_NATIVE:
                case MODE_HALF_NATURAL:
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
        //gpio::outputHigh(gpio::Pin{21});
        if (cb_) 
            cb_();
        if (updating_ > 0)
            --updating_;
        //gpio::outputLow(gpio::Pin{21});
    }

} // namespace rckid