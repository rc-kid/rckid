#include "ST7789.h"

#include <pico/time.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/dma.h>

#include "images/logo-16.h"
#include "graphics/png.h"

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
        //irq_set_exclusive_handler(DMA_IRQ_0, irqDMADone);
        irq_add_shared_handler(DMA_IRQ_0, irqDMADone,  PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
        irq_set_enabled(DMA_IRQ_0, true);
        // reset the display

        reset();

        // now clear the entire display black
        setColumnRange(0, 319);
        setRowRange(0, 239);
        beginCommand(RAMWR);
        gpio_put(RP_PIN_DISP_DCX, true);
#if (defined RCKID_SPLASHSCREEN_OFF)
        for (size_t i = 0, e =320 * 240; i < e; ++i) {
            sendByte(0);
            sendByte(0);
        }
#else
        PNG png = PNG::fromBuffer(Logo16, sizeof(Logo16));
        png.decode([&](Color * line, int lineNum, int lineWidth){
            uint8_t const * raw = reinterpret_cast<uint8_t *>(line);
            for (int i = 0; i < lineWidth; ++i) {
                sendByte(raw[1]);
                sendByte(raw[0]);
                raw += 2;
            }
        });
#endif
        end();


    }

    void ST7789::reset() {
        dma_channel_abort(dma_);
        pio_sm_set_enabled(pio_, sm_, false);

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
        setDisplayMode(DisplayMode::Natural);
        sendCommand(INVON);
    }

    void ST7789::fill(Color color) {
        setColumnRange(0, 319);
        setRowRange(0, 239);
        beginCommand(RAMWR);
        gpio_put(RP_PIN_DISP_DCX, true);
        uint16_t x = color.rawValue16();
        for (size_t i = 0, e =320 * 240; i < e; ++i) {
            sendByte((x >> 8) & 0xff);
            sendByte(x & 0xff);
        }
        end();
    }

    void ST7789::enterContinuousMode(Mode mode) {
        leaveContinuousMode();
        switch (mode) {
            case Mode::Single:
                setColumnRange(0, 239);
                setRowRange(0, 319);
                setColorMode(ST7789::ColorMode::RGB565);
                setDisplayMode(ST7789::DisplayMode::Native);
                // start the continuous RAM write command
                beginCommand(RAMWR);
                gpio_put(RP_PIN_DISP_DCX, true);
                // initialize the PIO program 
                ST7789_rgb_program_init(pio_, sm_, offsetSingle_, RP_PIN_DISP_WRX, RP_PIN_DISP_DB8);
                break;
            case Mode::Double:
                setColumnRange(0, 239);
                setRowRange(0, 319);
                setColorMode(ST7789::ColorMode::RGB565);
                setDisplayMode(ST7789::DisplayMode::Native);
                // start the continuous RAM write command
                beginCommand(RAMWR);
                gpio_put(RP_PIN_DISP_DCX, true);
                // initialize the PIO program 
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

    void ST7789::irqDMADone() {
        if(dma_channel_get_irq0_status(dma_)) {
            dma_channel_acknowledge_irq0(dma_); // clear the flag
            if (cb_)
                cb_();
            else 
                updating_ = false;
        }
    }


} // namespace rckid