#include "ST7789.h"

#include <pico/time.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/dma.h>


#include "rckid.h"

namespace rckid {

    void ST7789::reset() {
        gpio_init(RP_PIN_DISP_TE);
        gpio_set_dir(RP_PIN_DISP_TE, GPIO_IN);
        gpio_init(RP_PIN_DISP_DCX);
        gpio_set_dir(RP_PIN_DISP_DCX, GPIO_OUT);
        gpio_init(RP_PIN_DISP_CSX);
        gpio_set_dir(RP_PIN_DISP_CSX, GPIO_OUT);
        gpio_put(RP_PIN_DISP_CSX, true);

        initializePinsBitBang();

        // TODO check the init sequence
        //while (true)
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
        sendCommand(MADCTL, 0_u8);
        sendCommand(INVON);

        // now clear the entire display black
        setColumnRange(0, 239);
        setRowRange(0, 319);
        beginCommand(RAMWR);
        gpio_put(RP_PIN_DISP_DCX, true);
        for (size_t i = 0, e =320 * 240; i < e; ++i) {
            sendByte(0);
            sendByte(0);
        }
        end();
    }

    void ST7789::loadPIODriver(pio_program_t const & driver, DriverInitializer initializer) {
        // load the pio program (we only do this once)
        pio_ = pio0;
        offset_ = pio_add_program(pio_, & driver);
        sm_ = pio_claim_unused_sm(pio_, true);
        dma_ = dma_claim_unused_channel(true);
        dmaConf_ = dma_channel_get_default_config(dma_); // create default channel config, write does not increment, read does increment, 32bits size
        channel_config_set_transfer_data_size(& dmaConf_, DMA_SIZE_16); // transfer 16 bytes
        channel_config_set_dreq(& dmaConf_, pio_get_dreq(pio_, sm_, true)); // tell our PIO
        channel_config_set_read_increment(& dmaConf_, true);
        dma_channel_configure(dma_, & dmaConf_, &pio_->txf[sm_], nullptr, 0, false); // start

        // enable IRQ0 on the DMA channel
        dma_channel_set_irq0_enabled(dma_, true);
        irq_set_exclusive_handler(DMA_IRQ_0, irqDMADone);
        irq_set_enabled(DMA_IRQ_0, true);
        // and now start the driver
        initializer(pio_, sm_, offset_, RP_PIN_DISP_WRX, RP_PIN_DISP_DB8);
    }

    void ST7789::startPIODriver() {
        pio_set_clock_speed(pio_, sm_, RP_ST7789_BAUDRATE);
        pio_sm_set_enabled(pio_, sm_, true);
    }

    void ST7789::enterContinuousMode() {
        //uint16_t left = (320 - width) / 2;
        //uint16_t top = (240 - height) / 2;
        // initiate a full screen rectangle update 
        //sendCommand(RASET, left, (uint16_t) left + width - 1);
        //sendCommand(CASET, top, (uint16_t) top + height - 1);
        beginCommand(RAMWR);
        gpio_put(RP_PIN_DISP_DCX, true);
        /*
        // claim the resources required for the continuous mode
        pio_gpio_init(pio_, RP_PIN_DISP_WRX);
        pio_sm_set_consecutive_pindirs(pio_, sm_, RP_PIN_DISP_WRX, 1, true);
        pio_gpio_init(pio_, RP_PIN_DISP_DB8);
        pio_gpio_init(pio_, RP_PIN_DISP_DB9);
        pio_gpio_init(pio_, RP_PIN_DISP_DB10);
        pio_gpio_init(pio_, RP_PIN_DISP_DB11);
        pio_gpio_init(pio_, RP_PIN_DISP_DB12);
        pio_gpio_init(pio_, RP_PIN_DISP_DB13);
        pio_gpio_init(pio_, RP_PIN_DISP_DB14);
        pio_gpio_init(pio_, RP_PIN_DISP_DB15);
        pio_sm_set_consecutive_pindirs(pio_, sm_, RP_PIN_DISP_DB8, 8, true);
        pio_sm_config c = ST7789_rgb_program_get_default_config(offset_);
        sm_config_set_sideset_pins(&c, RP_PIN_DISP_WRX);
        sm_config_set_out_pins(&c, RP_PIN_DISP_DB8, 8);
        sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
        // disable autopull
        sm_config_set_out_shift(&c, false, true, 16);
        pio_sm_init(pio_, sm_, offset_, &c);
        // start the pio
        pio_set_clock_speed(pio_, sm_, 30000000);
        pio_sm_set_enabled(pio_, sm_, true);
        */
    }

    void ST7789::leaveContinuousMode() {
        pio_sm_set_enabled(pio_, sm_, false);
        end(); // end the RAMWR command
        initializePinsBitBang();
    }

    void ST7789::updateContinuous(void const * data, size_t numPixels) {
        dma_channel_configure(dma_, & dmaConf_, &pio_->txf[sm_], data, numPixels, true); // start
    }

    void ST7789::initializePinsBitBang() {
        uint32_t outputPinsMask = (1 << RP_PIN_DISP_WRX) | (0xff << RP_PIN_DISP_DB8); // DB8..DB15 are consecutive
        gpio_init_mask(outputPinsMask);
        gpio_set_dir_masked(outputPinsMask, outputPinsMask);
        //gpio_put_masked(outputPinsMask, false);
        gpio_put(RP_PIN_DISP_WRX, false);
    }

    void ST7789::irqDMADone() {
        if(dma_channel_get_irq0_status(dma_)) {
            dma_channel_acknowledge_irq0(dma_); // clear the flag
            if (transferStart_ != nullptr) {
                size_t cnt = transferEnd_ - transferStart_;
                if (cnt > 2 * lineSizeInBytes_)
                    cnt = 2 * lineSizeInBytes_;
                if (cnt > 0) {
                    dma_channel_transfer_from_buffer_now(dma_, transferStart_, cnt / 2);
                    transferStart_ += lineSizeInBytes_;
                }
                if (transferStart_ >= transferEnd_)
                    transferStart_ = nullptr;
            }
        }
    }


} // namespace rckid