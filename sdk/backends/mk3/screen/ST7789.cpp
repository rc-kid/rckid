#include "ST7789.h"
#include "ST7789_rgb16.pio.h"

#include <rckid/graphics/png.h>
#include <rckid/assets/images.h>
#include "../backend_internals.h"

namespace rckid {

    void ST7789::initialize() {
        // load and initialize the PIO programs for single and double precission
        pio_set_gpio_base(RCKID_ST7789_PIO, 16);
        sm_ = pio_claim_unused_sm(RCKID_ST7789_PIO, true);
        LOG(LL_INFO, "SM: " << (uint32_t)sm_);
        offsetSingle_ = pio_add_program(RCKID_ST7789_PIO, & ST7789_rgb16_program);
        LOG(LL_INFO, "Offset single: " << (uint32_t)offsetSingle_);
        //offsetDouble_ = pio_add_programRCKID_ST7789_PIO, & ST7789_rgb_double_program);
        // initialize the DMA channel and set up interrupts
        dma_ = dma_claim_unused_channel(true);
        LOG(LL_INFO, "DMA: " << (uint32_t)dma_);
        dmaConf_ = dma_channel_get_default_config(dma_); // create default channel config, write does not increment, read does increment, 32bits size
        channel_config_set_transfer_data_size(& dmaConf_, DMA_SIZE_16); // transfer 16 bytes
        channel_config_set_dreq(& dmaConf_, pio_get_dreq(RCKID_ST7789_PIO, sm_, true)); // tell our PIO
        channel_config_set_read_increment(& dmaConf_, true);
        dma_channel_configure(dma_, & dmaConf_, &RCKID_ST7789_PIO->txf[sm_], nullptr, 0, false); // start

        // enable IRQ0 on the DMA channel
        dma_channel_set_irq0_enabled(dma_, true);
        //irq_add_shared_handler(DMA_IRQ_0, irqDMADone,  PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);

        // reset the display
        reset();
    }

    void ST7789::reset() {
        // reset updating - forcefully since we are resetting the display anyways
        dma_channel_abort(dma_);
        pio_sm_set_enabled(RCKID_ST7789_PIO, sm_, false);
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
        setDisplayMode(Resolution::Full, DisplayRefreshDirection::ColumnFirst);
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
        updateRegion_ = Rect::WH(RCKID_DISPLAY_WIDTH, RCKID_DISPLAY_HEIGHT);
        setUpdateRegion(updateRegion_);
        // and now do the png file
        // now clear the entire display black
#if (RCKID_SPLASHSCREEN_OFF == 1)
        clear(0x0000);
#else
        setDisplayMode(Resolution::Full, DisplayRefreshDirection::RowFirst);
        setUpdateRegion(updateRegion_);
        beginCommand(RAMWR);
        gpio_put(RP_PIN_DISP_DCX, true);
        PNG png = PNG::fromBuffer(assets::logo16);
        png.decodeRGB([&](uint16_t * line, [[maybe_unused]] int lineNum, int lineWidth){
            for (int i = 0; i < lineWidth; ++i)
                sendWord(line[i]);
        });
        gpio_put(RP_PIN_DISP_DCX, false);
        end();
        setDisplayMode(Resolution::Full, DisplayRefreshDirection::ColumnFirst);
        setUpdateRegion(updateRegion_);
#endif
    }

    void ST7789::setDisplayMode(Resolution res, DisplayRefreshDirection dir) {
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

    void ST7789::clear(uint16_t color) {
        setDisplayMode(Resolution::Full, DisplayRefreshDirection::RowFirst);
        setUpdateRegion(Rect::WH(RCKID_DISPLAY_WIDTH, RCKID_DISPLAY_HEIGHT));
        enterCommandMode();
        beginCommand(RAMWR);
        gpio_put(RP_PIN_DISP_DCX, true);
        for (size_t i = 0, e = RCKID_DISPLAY_WIDTH * RCKID_DISPLAY_HEIGHT; i < e; ++i)
            sendWord(color);

        gpio_put(RP_PIN_DISP_DCX, false);
        end();
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
        if (pio_sm_is_enabled(RCKID_ST7789_PIO, sm_)) {
            // let the last display update finish (blocking)
            waitUpdateDone();  
            cb_ = nullptr; // to be sure
            pio_sm_set_enabled(RCKID_ST7789_PIO, sm_, false);
            end(); // end the RAMWR command
            // mark pins at bitbanged
            initializePinsBitBang();
        }
    }

    void ST7789::enterUpdateMode() {
        if (!pio_sm_is_enabled(RCKID_ST7789_PIO, sm_)) {
            beginCommand(RAMWR);
            gpio_put(RP_PIN_DISP_DCX, true);
            // initialize the corresponding PIO program
            switch (mode_) {
                case MODE_FULL_NATIVE:
                case MODE_FULL_NATURAL:
                    ST7789_rgb16_program_init(RCKID_ST7789_PIO, sm_, offsetSingle_, RP_PIN_DISP_WRX, RP_PIN_DISP_DB15);
                    break;
                /*
                case MODE_HALF_NATIVE:
                case MODE_HALF_NATURAL:
                    ST7789_rgb_double_program_initRCKID_ST7789_PIO, sm_, offsetDouble_, RP_PIN_DISP_WRX, RP_PIN_DISP_DB8);
                    break;
                */
                default:
                    UNREACHABLE;
            }
            // and start the pio
            pio_sm_set_enabled(RCKID_ST7789_PIO, sm_, true);
        }
    }


    //void ST7789::beginDMAUpdate() {

    //}

    //void ST7789::endDMAUpdate() {

    //}

    void ST7789::initializePinsBitBang() {

        constexpr uint64_t outputPinsMask = 0xffff_u64 << RP_PIN_DISP_DB15; // DB0..DB15 are consecutive
        gpio_set_dir_masked64(outputPinsMask, 0xffffffffffffffffll); // set all pins to output
        gpio_put_masked64(outputPinsMask, 0);
        gpio_set_function_masked64(outputPinsMask, GPIO_FUNC_SIO);
        //gpio_init_mask(outputPinsMask);
        //gpio_set_dir_masked64(outputPinsMask, outputPinsMask);
        //gpio_put_masked(outputPinsMask, false);

        /*
        gpio_init(RP_PIN_DISP_DB0);
        gpio_init(RP_PIN_DISP_DB1);
        gpio_init(RP_PIN_DISP_DB2);
        gpio_init(RP_PIN_DISP_DB3);
        gpio_init(RP_PIN_DISP_DB4);
        gpio_init(RP_PIN_DISP_DB5);
        gpio_set_dir(RP_PIN_DISP_DB0, GPIO_OUT);
        gpio_set_dir(RP_PIN_DISP_DB1, GPIO_OUT);
        gpio_set_dir(RP_PIN_DISP_DB2, GPIO_OUT);
        gpio_set_dir(RP_PIN_DISP_DB3, GPIO_OUT);
        gpio_set_dir(RP_PIN_DISP_DB4, GPIO_OUT);
        gpio_set_dir(RP_PIN_DISP_DB5, GPIO_OUT);
        */


        gpio_init(RP_PIN_DISP_WRX);
        gpio_set_dir(RP_PIN_DISP_WRX, GPIO_OUT);
        gpio_put(RP_PIN_DISP_WRX, true);

        gpio_init(RP_PIN_DISP_RDX);
        gpio_set_dir(RP_PIN_DISP_RDX, GPIO_OUT);
        gpio_put(RP_PIN_DISP_RDX, true);

    }

    void ST7789::irqHandler() {
        //gpio::outputHigh(gpio::Pin{21});
        if (cb_) 
            cb_();
        if (updating_ > 0)
            updating_ = updating_ - 1;
        //gpio::outputLow(gpio::Pin{21});
    }

} // namespace rckid