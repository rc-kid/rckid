#pragma once

#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/dma.h>



#include "common/config.h"
#include "utils.h"
#include "ST7789_rgb.pio.h"
#include "ST7789_rgb_double.pio.h"
#include "ST7789_rgba.pio.h"
#include "color.h"

//#include "gpu/graphics.h"

namespace rckid {

    namespace display_profile {

        struct RGB {
            using Color = ColorRGB;
            static constexpr int Width = 320;
            static constexpr int Height = 240;
            static constexpr bool NativeMode = true;
            static constexpr bool Double = false;
        }; 

        struct RGBDouble {
            using Color = ColorRGB;
            static constexpr int Width = 160;
            static constexpr int Height = 120;
            static constexpr bool NativeMode = true;
            static constexpr bool Double = false;
        }; 

    } // namespace rckid::display_config

    /** Low level driver for the ST7789 display driver. 
     
        The driver takes care of display initialization and provides basic functions for display updates in either direct, or continuous mode. The direct provides blocking interface for sending either commands, or data updates to the system, while the continous mode only supoorts sending data updates via DMA. 

     */
    class ST7789 {
    public:
        enum class ColorMode : uint8_t {
            RGB565 = 0x55,
            RGB666 = 0x66,
        }; // ST7789::PixelMode

        typedef void (*DriverInitializer)(PIO, uint, uint, uint, uint);

        /** Initializes the display with given profile. 
         */
        template<typename PROFILE>
        static void initialize();

        /** Updates the entire display area using given profile information. 
         */
        template<typename PROFILE>
        static void update(typename PROFILE::Color const * pixels, int width, int height);

        /** Initializes the display. 
         
            Performs a full reset and initializes the display to 320x240 format with 565 RGB colors and clears the entire display black. 
         */
        static void reset();

        /** Loads the specified pio driver. 
         */
        static void loadPIODriver(pio_program_t const & driver, DriverInitializer initializer); 

        /** Starts the previously loaded pio driver. 
         */
        static void startPIODriver();

        // TODO pio stop


        /** Sets the color mode used by the driver. By default RGB565 is used, but RGB666 can be selected instead, in which case 3 bytes are sent per pixel, each containing 6bit color information in the MSBs. 
         */
        static void setColorMode(ColorMode pm) {
            sendCommand(COLMOD, static_cast<uint8_t>(pm));
        }

        /** Enables the native rotation of the display, in which the display will be rendered from top right to bottom left in its native rotation. This mode is ideal for properly working vsync. RCKid's graphic primitives compensate for this by different mapping functions and the native mode is thus enabled by default. 
         */
        static void nativeRotation() {
            sendCommand(MADCTL, 0_u8);
        }

        /** Enables the natural display orientation in which the display is updated from left top to bottom right in the way it is oriented inside RCKid. However, in this mode the vsync information is much less useful. 
         */
        static void naturalRotation() {
            sendCommand(MADCTL, (uint8_t)(MADCTL_MY | MADCTL_MV ));
        }

        /** Sets the columns range for RAM updates. Columns are referenced in the native mode and can be from 0 to 239 inclusive. 
        */
        static void setColumnRange(uint16_t start, uint16_t end) {
            sendCommand(CASET, start, end);
        }

        /** Sets the rows range for RAM updates. Rows are references in the native mode and can be from 0 to 319 inclusive. 
         */
        static void setRowRange(uint16_t start, uint16_t end) {
            sendCommand(RASET, start, end);
        }

        /** Busy waits for the rising edge on the TE display pin, signalling the beginning of the V-blank period. 
         */
        static void waitVSync() { while (gpio_get(RP_PIN_DISP_TE)); while (! gpio_get(RP_PIN_DISP_TE)); }

        /** \name Continuous mode
         
            The continous mode does not send any commands to the display and only updates the entire display area. When data for entire display are sent, new frame will begin. The continuous mode uses 32bit PIO tuned to high speed fast updates with minimal CPU intervention. 

            The continuous mode is not intended to be used directly by the users, but rather should be utilized by various gpu modes, such as framebuffer or tiling engine. 
         */
        static void enterContinuousMode();

        static void leaveContinuousMode();

        static void updateContinuous(void const * data, size_t numPixels);

        static void waitUpdateDone() { while (transferStart_ != nullptr); }

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

        static void irqDMADone();

        static inline PIO pio_;
        static inline uint sm_;
        static inline uint offset_;
        static inline uint dma_ = -1;
        static inline dma_channel_config dmaConf_;

        static inline volatile uint8_t const * transferStart_ = nullptr;
        static inline uint8_t const * transferEnd_;
        static inline size_t lineSizeInPixels_;



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

        static constexpr uint8_t WRMEMC = 0x3c;
        static constexpr uint8_t STE = 0x44; // set tear scanline


    }; // rckid::ST7789

    /** Initializes the display for the native RGB 16bit pixels. 
     */
    template<>
    inline void ST7789::initialize<display_profile::RGB>() {
        reset();
        setColumnRange(0, 239);
        setRowRange(0, 319);
        enterContinuousMode();
        loadPIODriver(ST7789_rgb_program, ST7789_rgb_program_init);
        startPIODriver();
    }

    template<>
    inline void ST7789::update<display_profile::RGB>(ColorRGB const * pixels, int width, int height) {
        transferEnd_ = (uint8_t const *)pixels;
        transferStart_ = (uint8_t const *)pixels;
        dma_channel_transfer_from_buffer_now(dma_, pixels, width * height);
        //dma_channel_configure(dma_, & dmaConf_, &pio_->txf[sm_], pixels, width * height, true); // start
    }

    template<>
    inline void ST7789::initialize<display_profile::RGBDouble>() {
        reset();
        setColumnRange(0, 239);
        setRowRange(0, 319);
        enterContinuousMode();
        loadPIODriver(ST7789_rgb_double_program, ST7789_rgb_double_program_init);
        startPIODriver();
    }

    template<>
    inline void ST7789::update<display_profile::RGBDouble>(ColorRGB const * pixels, int width, int height) {
        transferEnd_ = (uint8_t const *)(pixels + width * height);
        transferStart_ = (uint8_t const *)(pixels);
        lineSizeInPixels_ = height;
        dma_channel_transfer_from_buffer_now(dma_, pixels, lineSizeInPixels_);
        //dma_channel_configure(dma_, & dmaConf_, &pio_->txf[sm_], pixels, width * height, true); // start
    }


} // namespace rckid