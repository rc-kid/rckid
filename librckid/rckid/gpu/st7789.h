#pragma once

#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/dma.h>


#include "rckid/rckid.h"
#include "common/config.h"

#include "graphics.h"

namespace rckid {

    /** Low level driver for the ST7789 display driver. 
     
        The driver takes care of display initialization and provides basic function for display updates. 
     */
    class ST7789 {
    public:

        /** Initializes the display. 
         
            Can also be used to reset the display. 
         */
        static void initialize();

        /** Fills given rectange with the specified color. 
         */
        static void fillRect(Rect r, Color c);

        /** Fills the entire display with the specified color. 
         */
        static void fill(Color c) { fillRect(Rect::WH(320, 240), c); }

        /** Copies given pixel data in row-wise format to the specified rectange. 
         
            The third argument specifies the size of the color aray (in pixels), and if smaller than the pixels in the rectange, the color data will be wrapped around. 
         */
        static void blitRect(Rect r, Color * data, size_t numPixels);

        /** Copies given pixel data in row-wise format to the specified rectange. 
         
            The color data provided must contain at least as many pixels as are in the rectange. 
         */
        static void blitRect(Rect r, Color * data) { blitRect(r, data, r.width() * r.height()); }

        /** Busy waits for the rising edge on the TE display pin, signalling the beginning of the V-blank period. 
         */
        static void waitVSync() { while (gpio_get(RP_PIN_DISP_TE)); while (! gpio_get(RP_PIN_DISP_TE)); }

        /** \name Continuous mode
         
            The continous mode does not send any commands to the display and only updates the entire display area. When data for entire display are sent, new frame will begin. The continuous mode uses 32bit PIO tuned to high speed fast updates with minimal CPU intervention. 

            The continuous mode is not intended to be used directly by the users, but rather should be utilized by various gpu modes, such as framebuffer or tiling engine. 
         */
        //@{
        static void enterContinuousMode(unsigned width = 320, unsigned height = 240);

        static void leaveContinuousMode();

        static void updateContinuous(Color const * data, size_t numPixels);

        static void waitUpdateDone() { while (dma_channel_is_busy(dma_)); }
        //@}


    private:

        static void initializePinsBitBang();

        static void beginCommand(uint8_t cmd) {
            gpio_put(RP_PIN_DISP_CSX, false);
            gpio_put(RP_PIN_DISP_DCX, false);
            // RP_PIN_DISP_WRX is expected to be low 
            sendByte(cmd);
        }

        static void end() {
            gpio_put(RP_PIN_DISP_CSX, true);
        }

        static void sendCommand(uint8_t cmd) {
            beginCommand(cmd);
            end();
        }

        static void sendCommand(uint8_t cmd, uint8_t const * params, size_t size) {
            beginCommand(cmd);
            gpio_put(RP_PIN_DISP_DCX, true);
            while (size-- > 0)
                sendByte(*params++);
            end();
        }

        static void sendCommand(uint8_t cmd, uint8_t p) {
            beginCommand(cmd);
            gpio_put(RP_PIN_DISP_DCX, true);
            sendByte(p);
            end();
        }

        static void sendCommand(uint8_t cmd, uint16_t p) {
            uint16_t d = swapBytes(p);
            sendCommand(cmd, reinterpret_cast<uint8_t *>(&d), 2);
        }

        static void sendCommand(uint8_t cmd, uint16_t p1, uint16_t p2) {
            uint16_t params[] = { swapBytes(p1), swapBytes(p2) };
            sendCommand(cmd, reinterpret_cast<uint8_t *>(params), 4);
        }

        static void sendByte(uint32_t b) {
            gpio_put_masked(0xff << RP_PIN_DISP_DB8, b << RP_PIN_DISP_DB8);
            sleep_ns(40);
            gpio_put(RP_PIN_DISP_WRX, true);
            sleep_ns(40);
            gpio_put(RP_PIN_DISP_WRX, false);
        }

        static inline PIO pio_;
        static inline uint sm_;
        static inline uint offset_;
        static inline uint dma_ = -1;
        static inline dma_channel_config dmaConf_;

        /** Current size of the display. 
         */
        static inline uint16_t rows_;
        static inline uint16_t cols_; 


        static constexpr uint8_t SWRESET = 0x01;

        static constexpr uint8_t SLPIN = 0x10; // enters sleep mode
        static constexpr uint8_t SLPOUT = 0x11; // leaves sleep mode, wait 120ms afterwards
        static constexpr uint8_t PTLON = 0x12; // enters partial mode (described by 0x30h)
        static constexpr uint8_t NORON = 0x13; // enters normal mode 
        static constexpr uint8_t INVOFF = 0x20; // turns off the inverse mode
        static constexpr uint8_t INVON = 0x21; // turns on the inverse mode
        static constexpr uint8_t GAMSET = 0x26; 
        static constexpr uint8_t DISPOFF = 0x28; // displays blank page (white), no change to memory
        static constexpr uint8_t DISPON = 0x29; // displays the memory contents

        static constexpr uint8_t CASET = 0x2a; // column address set
        static constexpr uint8_t RASET = 0x2b; // row address set
        static constexpr uint8_t RAMWR = 0x2c; // ram data write

        static constexpr uint8_t PTLAR = 0x30; // partial area specification
        static constexpr uint8_t VSCRDEF = 0x33; // vertical scrolling definition


        static constexpr uint8_t TEOFF = 0x34; // tearing effect line off
        static constexpr uint8_t TEON = 0x35; // tearing effect line off
        static constexpr uint8_t TE_VSYNC = 0; // TE on vsync only
        static constexpr uint8_t TE_V_AND_H_SYNC = 1; // TE on both V and H sync

        static constexpr uint8_t MADCTL = 0x36; // Memory Area Data Access Control
        static constexpr uint8_t MADCTL_MY = 0x80;  // Bottom to top
        static constexpr uint8_t MADCTL_MX = 0x40;  // Right to left
        static constexpr uint8_t MADCTL_MV = 0x20;  // Reverse Mode
        static constexpr uint8_t MADCTL_ML = 0x10;  // LCD refresh Bottom to top
        static constexpr uint8_t MADCTL_RGB = 0x00; // Red-Green-Blue pixel order
        static constexpr uint8_t MADCTL_BGR = 0x08; // Blue-Green-Red pixel order
        static constexpr uint8_t MADCTL_MH = 0x04;  // LCD refresh right to left    

        static constexpr uint8_t VSCSAD = 0x37; // vertical scroll start address of ram

        static constexpr uint8_t IDMOFF = 0x38; // idle mode off
        static constexpr uint8_t IDMON = 0x39; // idle mode on (fewer colors, etc.)

        static constexpr uint8_t COLMOD = 0x3a; // Color mode
        static constexpr uint8_t COLMOD_16 = 0x55; // 16 bit pixel format for both control and RGB interfaces to be sure

        static constexpr uint8_t WRMEMC = 0x3c;
        static constexpr uint8_t STE = 0x44; // set tear scanline


    }; // rckid::ST7789

} // namespace rckid