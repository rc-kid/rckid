#pragma once

#include <functional>

#include "rckid/rckid.h"
#include "rckid/stats.h"
#include "rckid/graphics/color.h"
#include "rckid/graphics/primitives.h"

namespace rckid {

    /** Supported FPS values for the display. 
     */
    enum class FPS : uint8_t {
        FPS_60 = 0x0f,
        FPS_50 = 0x15, 
        FPS_40 = 0x1e,  
    };

    /** Low level driver for the ST7789 display driver. 
     
        The driver takes care of display initialization and provides basic functions for display updates via DMA.

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

        /** Initializes the display. 
         
            Performs a full reset and initializes the display to 320x240 format with 565 RGB colors and clears the entire display black. 
         */
        static void initialize();

        /** Performs reeset of the display into bitbanging mode. 
         
            Called by the initialize function and might be useful in cases the display is in an unknown state, such as when the blue screen of death might occur. 
         */
        static void reset();

        /** Returns the current display mode.
         */
        static DisplayMode mode() { return mode_; }

        /** Configures the display for the given mode. 
         */
        static void setMode(DisplayMode mode);

        static Rect updateRegion() { return updateRegion_; }

        /** Sets the update region of the screen. 
         
            The rectangle is always assumed to be in the natural coordinates, i.e. 320 columns, 240 rows. 
         */
        static void setUpdateRegion(Rect rect);

        /** Sets the framerate of the display.
         
            At startup, 60 fps is selected, but this can be lowered by the apps based on the preset value. 
         */
        static void setFPS(FPS fps) {
            sendCommand(FRCTRL2, static_cast<uint8_t>(fps));
        }

        /** Clears the entire display with given color. 
    
            Mostly useful for barebones clearing the screen in debug mode as the fill rate is rather slow. A much better approach is to enter the continous mode and fill the screen using the pio & dma.  
         */
        static void fill(ColorRGB color);


        static void setUpdateRegion(int width, int height) { 
            setUpdateRegion(Rect::XYWH((320 - width) / 2, (240 - height) / 2, width, height));
        }

        /** Resets the update region to entire screen. 
         */
        //static void resetUpdateRegion() { setUpdateRegion(Rect::WH(320, 240)); }

        /** \name Update mode
         * 
         */
        //@{

        //static void beginUpdate();
        //static void endUpdate();

        //static void update(ColorRGB const * pixels, uint32_t numPixels);
        //@}

        /** \name DMA update mode 

            The DMA mode does not send any commands to the display and only updates the selected display area. When data for the display area are sent, new frame will begin. The DMA mode uses 32bit PIO tuned to high speed for fast updates with minimal CPU intervention. 

            To specify the portion of the screen to be updated, use the setUpdateRegion() or resetUpdateRegion() methods. 
         */
        //@{

        /** Enters the DMA update mode, takes the display pins for the pio. 
         */
        static void beginDMAUpdate();

        /** Leaves the DMA update mode and returns the display pins back to GPIO. 
         */
        static void endDMAUpdate();

        /** Returns true if there is a screen update in progress. This is whenever the DMA is active, but can also be between DMA transfers if the callback function indicated more data to come. 
         */
        static bool dmaUpdateInProgress() { return updating_ != 0; }

        /** Writes given pixels to the srceeen using the DMA. 
         
            If the DMA update is active, it reuses the callback function set with the first call, otherwise the update is treated as a single update. 
         */
        static void dmaUpdateAsync(ColorRGB const * pixels, size_t numPixels) {
            if (updating_ == 0)
                cb_ = nullptr; // be done with the update
            dmaUpdateAsync(pixels, numPixels, cb_);
        }

        /** Writes given pixels and provides a callback function to be called when the DMA transfer finishes. 
         */
        static void dmaUpdateAsync(ColorRGB const * pixels, uint32_t numPixels, DMAUpdateCallback cb) {
            cb_ = cb;
            if (updating_ == 0)
                stats::displayUpdateStart_ = uptimeUs();
            // updating_ is volatile, but this is ok - it is only main app code (here), or from an IRQ
            updating_ = updating_ + 1;
#if (! defined ARCH_MOCK)
            dma_channel_transfer_from_buffer_now(dma_, pixels, numPixels);
#else       
            sendMockPixels(pixels, numPixels);
#endif
        }

        //static void dmaUpdateBlocking(ColorRGB const * pixels, uint32_t numPixels) {
        //    ASSERT(!dmaUpdateInProgress() && "Blocking DMA update must be the first in frame");
        //    dmaUpdateAsync(pixels, numPixels);
        //    while (dmaUpdateInProgress()) 
        //        yield();
        //}
        //@}

        /** Busy waits until the display finishes updating. Only useful in DMA update async mode, otherwise returns immediately.
         */
        static void waitUpdateDone() { 
            while (updating_ != 0)
                yield();
        }

        /** Busy waits for the rising edge on the TE display pin, signalling the beginning of the V-blank period. 
         */
        static void waitVSync() { 
#if (! defined ARCH_MOCK)
            MEASURE_TIME(stats::waitVSyncUs_, 
                while (gpio_get(RP_PIN_DISP_TE)); 
                while (! gpio_get(RP_PIN_DISP_TE))
                    yield();
            );
#endif
        }

    private:

        static void irqHandler();

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

        static void initializePinsBitBang();

        static void beginCommand(uint8_t cmd);

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
            uint16_t d = platform::swapBytes(p);
            sendCommand(cmd, reinterpret_cast<uint8_t *>(&d), 2);
        }

        static void sendCommand(uint8_t cmd, uint16_t p1, uint16_t p2) {
            uint16_t params[] = { platform::swapBytes(p1), platform::swapBytes(p2) };
            sendCommand(cmd, reinterpret_cast<uint8_t *>(params), 4);
        }

        /** Sends single byte in the bit-banging mode. 
         
            According to the datasheet (page 41), the read byte cycle is at least 66ns with both high and low pulses being at least 15ns. To work in the worst supported case at 250MHz, we insert 24 NOP instructions in total for 96ns, which adds about 30% safety margin. This will double in the 125MHz default speed, since the driver should mostly utilize the PIO driven DMA access, it should not be a problem. 
         */
        static void sendByte(uint32_t b) {
#if (! defined ARCH_MOCK)
            gpio_put_masked(0xff << RP_PIN_DISP_DB8, b << RP_PIN_DISP_DB8);
            cpu::nop();
            cpu::nop();
            cpu::nop();
            cpu::nop();
            cpu::nop();
            cpu::nop();
            cpu::nop();
            cpu::nop();
            cpu::nop();
            cpu::nop();
            cpu::nop();
            cpu::nop();
            gpio_put(RP_PIN_DISP_WRX, true);
            cpu::nop();
            cpu::nop();
            cpu::nop();
            cpu::nop();
            cpu::nop();
            cpu::nop();
            cpu::nop();
            cpu::nop();
            cpu::nop();
            cpu::nop();
            cpu::nop();
            cpu::nop();
            gpio_put(RP_PIN_DISP_WRX, false);
#endif
        }

        // PIO settings including the DMA used for the display and the addresses for the pio drivers for normal and double modes.
        static inline PIO pio_;
        static inline uint sm_;
        static inline uint offsetSingle_;
        static inline uint offsetDouble_;
        static inline uint dma_ = -1;
        static inline dma_channel_config dmaConf_;

        // Update bookkeeping - the callback, whether update is active or not, the current display mode and the update region
        static inline DMAUpdateCallback cb_;
        static inline volatile uint32_t updating_ = 0;
        static inline DisplayMode mode_ = DisplayMode::Native;
        static inline Rect updateRegion_ = Rect::WH(320, 240);


        // Low level driver constants
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
        static constexpr uint8_t COLMOD_565 = 0x55; // Color Mode 5-6-5
        static constexpr uint8_t COLMOD_666 = 0x66; // Color Mode 6-6-6

        static constexpr uint8_t WRMEMC = 0x3c;
        static constexpr uint8_t STE = 0x44; // set tear scanline

        static constexpr uint8_t PORCTRL = 0xb2; // porch control
        static constexpr uint8_t FRCTRL2 = 0xc6; // framerate control

        // the global IRQ handler
        friend void irqDMADone_();
        friend void yield();

    }; // rckid::ST7789

} // namespace rckid
