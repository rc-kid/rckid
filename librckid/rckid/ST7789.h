#pragma once

#include "rckid.h"
#include "graphics/color.h"
#include "graphics/primitives.h"

//#include "gpu/graphics.h"

namespace rckid {

    /** Low level driver for the ST7789 display driver. 
     
        The driver takes care of display initialization and provides basic functions for display updates in either direct, or continuous mode. The direct provides blocking interface for sending either commands, or data updates to the system, while the continous mode only supoorts sending data updates via DMA. 

        RCKid uses a 37pin hand-solderable version of 2.8" 320x240 TFT display with the following pinout:

        1 - DBO
        2 - DB1
        3 - DP2
        4 - DB3
        5 - GND
        6 - VDDI
        7 - CSX
        8 - DCX
        9 - WRX
        10 - RDX
        11 - NC
        12 - NC
        13 - NC
        14 - NC
        15 - NC
        16 - LED-A
        17 - LED-K
        18 - LED-K
        19 - LED-K
        20 - LED-K
        21 - TE
        22 - DB4
        23 - DB8
        24 - DB9
        25 - DB10
        26 - DB11
        27 - DB12
        28 - DB13
        29 - DB14
        30 - DB15
        31 - RESX
        32 - VDD
        33 - VDDI
        35 - DB5
        36 - DB6
        37 - DB7

     */
    class ST7789 {
    public:

        using UpdatePixelsCallback = std::function<void()>;

        enum class ColorMode : uint8_t {
            RGB565 = 0x55,
            RGB666 = 0x66,
        }; // ST7789::PixelMode

        enum class DisplayMode: uint8_t {
            Native = 0, 
            NativeBGR = 0x08, // MADCTL_BGR
            Natural = 0x80 + 0x20, // MADCTL_MY | MADCTL_MV 
            NaturalBGR = 0x80 + 0x20 + 0x08, // MADCTL_MY | MADCTL_MV | MADCTL_BGR
        }; // ST7789::DisplayMode

        enum class Mode {
            Single, 
            Double,
        }; // ST7789::Mode

        /** Initializes the display. 
         
            Performs a full reset and initializes the display to 320x240 format with 565 RGB colors and clears the entire display black. 
         */
        static void initialize();

        /** Performs reeset of the display into bitbanging mode. 
         
            Called by the initialize function and might be useful in cases the display is in an unknown state, such as when the blue screen of death might occur. 
         */
        static void reset();

        /** Clears the entire display with given color. 
         */
        static void fill(ColorRGB color);

        /** \name Continuous mode
         
            The continous mode does not send any commands to the display and only updates the entire display area. When data for entire display are sent, new frame will begin. The continuous mode uses 32bit PIO tuned to high speed fast updates with minimal CPU intervention. 

            The continuous mode is not intended to be used directly by the users, but rather should be utilized by various gpu modes, such as framebuffer or tiling engine. 
         */
        static void enterContinuousMode(Mode mode = Mode::Single) { enterContinuousMode(Rect::WH(320, 240), mode); }

        static void enterContinuousMode(Rect rect, Mode mode = Mode::Single); 

        static void leaveContinuousMode();

        static void updatePixels(uint16_t const * pixels, size_t numPixels) {
            cb_ = [](){ 
                ST7789::updating_ = false; 
                Stats::displayUpdateUs_ = static_cast<unsigned>(uptime_us() - Stats::updateStart_);
            };
            updatePixelsPartial(pixels, numPixels);
        }

        static void updatePixelsPartial(uint16_t const * pixels, size_t numPixels, UpdatePixelsCallback cb) {
            Stats::updateStart_ = uptime_us();
            cb_ = cb;
            updatePixelsPartial(pixels, numPixels);
        }

        static void updatePixelsPartial(uint16_t const * pixels, size_t numPixels) {
            if (!updating_)
                Stats::updateStart_ = uptime_us();
            updating_ = true;
#if (! defined LIBRCKID_MOCK)
            dma_channel_transfer_from_buffer_now(dma_, pixels, numPixels);
#else       
            sendMockPixels(pixels, numPixels);
#endif
        }

        /** Updates the entire display area using given profile information. 
         */
        //template<typename PROFILE>
        //static void update(typename PROFILE::Color const * pixels, int width, int height);

        static bool updateDone() { return !updating_; }

        static void waitUpdateDone() { 
            uint64_t t = uptime_us();
            while (updating_)
                yield();
            Stats::updateWaitUs_ = static_cast<unsigned>(uptime_us() - t);
        }

        /** Sets the color mode used by the driver. By default RGB565 is used, but RGB666 can be selected instead, in which case 3 bytes are sent per pixel, each containing 6bit color information in the MSBs. 
         */
        static void setColorMode(ColorMode pm) {
            sendCommand(COLMOD, static_cast<uint8_t>(pm));
        }

        /** Sets the corresponding display mode (orientation and color order). 
         */
        static void setDisplayMode(DisplayMode dm) {
            sendCommand(MADCTL, static_cast<uint8_t>(dm));
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
        static void waitVSync() { 
#if (! defined LIBRCKID_MOCK)
            uint64_t t = uptime_us();
            while (gpio_get(RP_PIN_DISP_TE)); 
            while (! gpio_get(RP_PIN_DISP_TE))
                yield();
            Stats::vsyncWaitUs_ = static_cast<unsigned>(uptime_us() - t);
#endif
        }

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
#if (! defined LIBRCKID_MOCK)
            gpio_put_masked(0xff << RP_PIN_DISP_DB8, b << RP_PIN_DISP_DB8);
            sleep_ns(40);
            gpio_put(RP_PIN_DISP_WRX, true);
            sleep_ns(40);
            gpio_put(RP_PIN_DISP_WRX, false);
#endif
        }

#if (defined LIBRCKID_MOCK)
        static void sendMockPixels(uint16_t const * pixels, size_t numPixels);
#endif

        static void irqDMADone();

        static inline PIO pio_;
        static inline uint sm_;
        static inline uint offsetSingle_;
        static inline uint offsetDouble_;
        static inline uint dma_ = -1;
        static inline dma_channel_config dmaConf_;

        static inline UpdatePixelsCallback cb_;
        static inline volatile bool updating_ = false;

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

} // namespace rckid
