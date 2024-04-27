#pragma once

#include <functional>

#include "rckid/rckid.h"
#include "rckid/stats.h"
#include "rckid/graphics/color.h"
#include "rckid/graphics/primitives.h"

namespace rckid {

    enum class DisplayMode {
        Native_RGB565,
        Native_2X_RGB565, 
        Natural_RGB565,
        Natural_2X_RGB565,
        // TODO other modes might have to be supported as well such as RGB666 or BGR alternatives, but these should be conditional on the 
        //Native_RGB666,
        //Native_BGR666,
    }; 

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

        TODO reading from the display is a bit complicated, a single byte must be at least 450ns and the read time must be precisely timed, or the data is gone. We can read in chunks and repurpose the SDA as sync mechanism between AVR and RP (AVR will tell RP to read the data by pulling SDA low). This will be interpreted by all other I2C devices as reapeated start & stop conditions from the AVR and should therefore be ignored and not interfere with the correctness of the I2C bus. 

     */
    class ST7789 {
    public:

        /** Callback function for continuous pixel update. 
         
            The function takes no arguments and returns true if the application is done upating the pixels for the current frame, or false if more pixel updates will be scheduled for current frame. 
         */
        using UpdatePixelsCallback = std::function<bool()>;

        /** Initializes the display. 
         
            Performs a full reset and initializes the display to 320x240 format with 565 RGB colors and clears the entire display black. 
         */
        static void initialize();

        /** Performs reeset of the display into bitbanging mode. 
         
            Called by the initialize function and might be useful in cases the display is in an unknown state, such as when the blue screen of death might occur. 
         */
        static void reset();

        /** Configures the display for the given mode. 
         */
        static void configure(DisplayMode mode);

        /** Clears the entire display with given color. 
    
            Mostly useful for barebones clearing the screen in debug mode as the fill rate is rather slow. A much better approach is to enter the continous mode and fill the screen using the pio & dma.  
         */
        static void fill(ColorRGB color);

        /** \name Continuous mode
         
            The continous mode does not send any commands to the display and only updates the entire display area. When data for entire display are sent, new frame will begin. The continuous mode uses 32bit PIO tuned to high speed for fast updates with minimal CPU intervention. 

            When entering the continuous mode, a rectangle of the screen that will be updated can be specified (defaulting to full screen). This can speed rendering even more in cases where only partial screen updates are necessary. 
            
         */

        /** Returns true if the display is in the continuous update mode. 
         */
        static bool continuousUpdateActive(); 

        /** Starts the continuous update of the entire screen. 
         */
        static void enterContinuousUpdate() { enterContinuousUpdate(Rect::WH(320, 240)); }

        /** Starts the continous update for part of the screen of given width and height that will be centered both vertically and horizotally on the screen. 
         */
        static void enterContinuousUpdate(int width, int height) { 
            enterContinuousUpdate(Rect::XYWH((320 - width) / 2, (240 - height) / 2, width, height));
        };

        /** Enters continous update of the specified rectangle of the screen. The rectangle must be specified in the full resolution of the display (i.e. 320x240 max). 
         */
        static void enterContinuousUpdate(Rect rect); 

        static void leaveContinuousUpdate();

        static void writePixels(uint16_t const * pixels, size_t numPixels) {
            if (!updating_)
                cb_ = [](){ return true; }; // be done with the update
            writePixels(pixels, numPixels, cb_);
        }

        static void writePixels(uint16_t const * pixels, size_t numPixels, UpdatePixelsCallback cb) {
            cb_ = cb;
            if (!updating_) {
                stats::updateStart_ = uptimeUs();
                updating_ = true;
            }
#if (! defined ARCH_MOCK)
            dma_channel_transfer_from_buffer_now(dma_, pixels, numPixels);
#else       
            sendMockPixels(pixels, numPixels);
#endif
        }

        //@}

        /** Returns true if the display is currently being updated. 
         */
        static bool updating() { return updating_; }

        /** Busy waits until the display finishes updating. 
         */
        static void waitUpdateDone() { 
            uint64_t t = uptimeUs();
            while (updating_)
                yield();
            stats::updateWaitUs_ = static_cast<unsigned>(uptimeUs() - t);
        }

        /** Busy waits for the rising edge on the TE display pin, signalling the beginning of the V-blank period. 
         */
        static void waitVSync() { 
#if (! defined ARCH_MOCK)
            uint64_t t = uptimeUs();
            while (gpio_get(RP_PIN_DISP_TE)); 
            while (! gpio_get(RP_PIN_DISP_TE))
                yield();
            stats::vsyncWaitUs_ = static_cast<unsigned>(uptimeUs() - t);
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

#if (defined ARCH_MOCK)
        static void sendMockPixels(uint16_t const * pixels, size_t numPixels);
#endif

        //static void irqDMADone();

        static inline PIO pio_;
        static inline uint sm_;
        static inline uint offsetSingle_;
        static inline uint offsetDouble_;
        static inline uint dma_ = -1;
        static inline dma_channel_config dmaConf_;

        static inline UpdatePixelsCallback cb_;
        static inline volatile bool updating_ = false;
        static inline volatile bool irqReady_ = false;

        static inline DisplayMode displayMode_ = DisplayMode::Native_RGB565;

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

        // the global IRQ handler
        friend void irqDMADone_();
        friend void yield();

    }; // rckid::ST7789

} // namespace rckid
