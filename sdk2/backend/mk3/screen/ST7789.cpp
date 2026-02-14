#include <hardware/dma.h>

#include "ST7789.h"
#include "ST7789_rgb16.pio.h"

#include <rckid/hal.h>
#include <rckid/graphics/png.h>
#include <assets/images.h>

#include "../config.h"

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
        // we need to use busy waits since the reset is also called from the fatal error handler, which happens inside an IRQ 
        sendCommand(SWRESET);
        busy_wait_ms(150);
        sendCommand(VSCSAD, (uint8_t)0);
        setRefreshDirection(hal::display::RefreshDirection::ColumnFirst);
        sendCommand(TEON, TE_VSYNC);
        sendCommand(SLPOUT);
        busy_wait_ms(150);
        sendCommand(DISPON);
        busy_wait_ms(150);
        //sendCommand(MADCTL, (uint8_t)(MADCTL_MV));
        //sendCommand(MADCTL, (uint8_t)(MADCTL_MY | MADCTL_MV ));
        //sendCommand(MADCTL, 0_u8);
        //setRefreshDirection(ST7789::DisplayMode::Native);
        sendCommand(INVON);
        updateRegion_ = Rect::WH(hal::display::WIDTH, hal::display::HEIGHT);
        setUpdateRegion(updateRegion_);
        // and now do the png file
        // now clear the entire display black
#if (RCKID_SPLASHSCREEN_OFF == 1)
        clear(0x0000);
#else
        setRefreshDirection(hal::display::RefreshDirection::RowFirst);
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
        setRefreshDirection(hal::display::RefreshDirection::ColumnFirst);
        setUpdateRegion(updateRegion_);
#endif
    }

    void ST7789::setRefreshDirection(hal::display::RefreshDirection dir) {
        refreshDir_ = dir;
        switch (dir) {
            case hal::display::RefreshDirection::ColumnFirst:
                sendCommand(COLMOD, COLMOD_565);
                sendCommand(MADCTL, 0_u8);
                break;
            case hal::display::RefreshDirection::RowFirst:
                 sendCommand(COLMOD, COLMOD_565);
                sendCommand(MADCTL, static_cast<uint8_t>(MADCTL_MY | MADCTL_MV));
                break;
            default:
                UNREACHABLE;
        }
    }

    void ST7789::clear(uint16_t color) {
        setRefreshDirection(hal::display::RefreshDirection::ColumnFirst);
        setUpdateRegion(Rect::WH(hal::display::WIDTH, hal::display::HEIGHT));
        beginCommand(RAMWR);
        gpio_put(RP_PIN_DISP_DCX, true);
        for (size_t i = 0, e = hal::display::WIDTH * hal::display::HEIGHT; i < e; ++i)
            sendWord(color);

        gpio_put(RP_PIN_DISP_DCX, false);
        end();
    }

    void ST7789::setUpdateRegion(Rect rect) {
        updateRegion_ = rect;
        switch (refreshDir_) {
            case hal::display::RefreshDirection::ColumnFirst: {
                Coord offset = 320 - rect.right();
                setColumnRange(rect.top(), rect.bottom() - 1);
                setRowRange(rect.left() + offset, rect.right() + offset - 1);
                break;
            }
            case hal::display::RefreshDirection::RowFirst: {
                setRowRange(rect.top(), rect.bottom() - 1);
                setColumnRange(rect.left(), rect.right() - 1);
                break;
            }
            default:
                UNREACHABLE;
        }
    }

    void ST7789::initializePinsBitBang() {

        constexpr uint64_t outputPinsMask = 0xffff_u64 << RP_PIN_DISP_DB15; // DB0..DB15 are consecutive
        gpio_set_dir_masked64(outputPinsMask, 0xffffffffffffffffll); // set all pins to output
        gpio_put_masked64(outputPinsMask, 0);
        gpio_set_function_masked64(outputPinsMask, GPIO_FUNC_SIO);

        gpio_init(RP_PIN_DISP_WRX);
        gpio_set_dir(RP_PIN_DISP_WRX, GPIO_OUT);
        gpio_put(RP_PIN_DISP_WRX, true);

        gpio_init(RP_PIN_DISP_RDX);
        gpio_set_dir(RP_PIN_DISP_RDX, GPIO_OUT);
        gpio_put(RP_PIN_DISP_RDX, true);

    }

} // namespace rckid