#pragma once

#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/dma.h>
#include <hardware/clocks.h>

#include "platform/platform.h"

#include "st7789.pio.h"

#include "rckid.h"

namespace rckid {

    /** ST7789 display driver

        The display used is the 2.8" IPS panel from aliexpress (https://www.aliexpress.com/item/1005004635814413.html?spm=a2g0o.order_list.order_list_main.5.1d8a1802PNYgMn). Other displays should technically work as well, the 8bit parallel interface is the same between ST7789 and the more common ILI9341 found in non-IPS panels, but the commands can differ. 

        This is the low-level driver that provides basic display control capabilities.

        Pin   | Conn | Details
        ------|------|--------------     
        1-4   | GND  | NC
        5     | GND  | GND
        6     | 3V3  | VDDI
        7     | 2    | CSX
        8     | 3    | DCX
        9     | 4    | WRX
        10    | 3V3  | RDX
        11-15 |      | NC
        16    | 3V3  | LEDA
        17-20 | pwm  | LEDK
        21    | 5    | TE
        22    |      | NC
        23-30 | 6-13 | DB8-DB15
        31    | 3V3  | RESX
        32    | 3V3  | VDD
        33    | 3V3  | VDDI
        34    | GND  | GND
        35-37 | GND  | NC

     */
    class ST7789 {
    public:

        static const platform::gpio::Pin PIN_CSX{2};
        static const platform::gpio::Pin PIN_DCX{3};
        static const platform::gpio::Pin PIN_WRX{4};
        static const platform::gpio::Pin PIN_TE{5};
        static const platform::gpio::Pin PIN_DATA{6}; // 6 - 13 for 8bits

        static void initialize() {
            using namespace platform;
            gpio::output(PIN_CSX);
            gpio::high(PIN_CSX);
            gpio::output(PIN_DCX);
            gpio::high(PIN_DCX);
            gpio::input(PIN_TE);
            // load the pio code for writing data, which also initializes the WR and DATA pins
            pio_ = pio0;
            offset_ = pio_add_program(pio_, &st7789_program);
            sm_ = pio_claim_unused_sm(pio_, true);
            st7789_program_init(pio_, sm_, offset_, PIN_WRX, PIN_DATA);
            // set the base frequency 
            overclock(100);
            // initialize the display
            sendCommand(SWRESET);
            cpu::delayMs(150);
            sendCommand8(VSCSAD, 0);
            sendCommand8(COLMOD, COLMOD_16);
            sendCommand8(TEON, TE_VSYNC);
            sendCommand(SLPOUT);
            cpu::delayMs(150);
            sendCommand(DISPON);
            cpu::delayMs(150);

            rotate(270);
            fillBitBang(0);
            /*
            uint8_t * data = new uint8_t[3200];
            for (size_t i = 0; i < sizeof(data); ++i)
                data[i] = i;

            fillAsync(data, 3200);
            */
            for (int i = 0; i < 5;++i) {
                sendCommand(INVOFF, nullptr, 0);
                cpu::delayMs(500);
                sendCommand(INVON, nullptr, 0);
                cpu::delayMs(500);
            }

        }

        static void rotate(uint angle) {
            uint8_t madctl;
            switch (angle) {
                case 90:
                    madctl = MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR; // left
                    break;
                case 180:
                    madctl = MADCTL_MY | MADCTL_BGR; // top
                    break;
                case 270:
                    madctl = MADCTL_MV | MADCTL_BGR; // right
                    break;
                default:
                    madctl = MADCTL_MX | MADCTL_BGR; // bottom
                    madctl = MADCTL_BGR; // raw
                    break;
            }
            sendCommand8(MADCTL, madctl);
        }

        static void fillBitBang(uint16_t color) {
            // enable the entire display
            setColumnUpdateRange();
            setRowUpdateRange(); 
            // now send the deta
            begin();
            setCommandMode();
            write(RAMWR);
            setDataMode();
            for (int i = 0; i < 320; ++i) {
                for (int j = 0; j < 240; ++j) {
                    write(0b11111000);
                    write(0b00000000);
                }
            }
            end();
        }

        /** Fills the entire screet with given color. 
         */
        /*
        static void fill(Pixel p) {
            setColumnUpdateRange();
            setRowUpdateRange();
            begin();
            setCommandMode();
            write(RAM_WRITE);
            setDataMode();
            for (int i = 0; i < 240; ++i) {
                for (int j = 0; j < 320; ++j) {
                    / *
                    if (j <= 240 - i) 
                        p.r(i % 32).b(j % 32).g(0);
                    else
                        p.r(0).b(0).gFull(j % 64);
                    * /
                    write(static_cast<uint16_t>(p) >> 8);
                    write(static_cast<uint16_t>(p) & 0xff);
                }
            }
            end();
        }
        */

     /** Fills given area of the display with provided pixel data. 
     
        Accepts size of the buffer, which must be a power of two. If the size is smaller than the actual area to be written, provided pixel data will be wrapped. 
     */
    static void fillAsync(uint8_t const * pixelData, size_t pixelDataSize = 0) {
        /*
        if (FMARK != gpio::UNUSED) {
            while (!gpio_get(FMARK));
            //while (gpio_get(FMARK));
        }
        */
        setColumnUpdateRange(0, 319);
        setRowUpdateRange(0, 10);
        begin();
        setCommandMode();
        write(RAMWR);
        setDataMode();
        /*
        if (FMARK != gpio::UNUSED) {
            while (!gpio_get(FMARK));
            //while (gpio_get(FMARK));
        }
        */
        if (writeAsync(pixelData, 320 * 10 * 2, pixelDataSize))
            writeAsyncWait();
        end();    
    }        

        static void setColumnUpdateRange(unsigned start = 0, unsigned end = 319) {
            sendCommand16(CASET, start, end);
        }

        static void setRowUpdateRange(unsigned start = 0, unsigned end = 239) {
            sendCommand16(RASET, start, end);
        }


    private:

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

        static void sendCommand(uint8_t cmd, uint8_t const * params, uint8_t size) {
            begin();
            setCommandMode();
            write(cmd);
            if (size > 0) {
                setDataMode();
                while (size-- != 0)
                    write(*params++);
            }
            end();
        }

        static void sendCommand(uint8_t cmd) {
            sendCommand(cmd, nullptr, 0);
        }

        static void sendCommand8(uint8_t cmd, uint8_t p) {
            sendCommand(cmd, & p, 1);
        }

        static void sendCommand16(uint8_t cmd, uint16_t p) {
            uint16_t params = swapBytes(p);
            sendCommand(cmd, reinterpret_cast<uint8_t *>( & params), 2);
        }

        static void sendCommand16(uint8_t cmd, uint16_t p1, uint16_t p2) {
            uint16_t params[] = { swapBytes(p1), swapBytes(p2) };
            sendCommand(cmd, reinterpret_cast<uint8_t *>(params), 4);
        }

        static void begin() {
            using namespace platform;
            st7789_wait_idle(pio_, sm_);
            gpio::low(PIN_CSX);
        }

        static void end() {
            using namespace platform;
            st7789_wait_idle(pio_, sm_);
            gpio::high(PIN_CSX);
        }

        static void setCommandMode() {
            using namespace platform;
            st7789_wait_idle(pio_, sm_);
            gpio::low(PIN_DCX);
        }

        static void setDataMode() {
            using namespace platform;
            st7789_wait_idle(pio_, sm_);
            gpio::high(PIN_DCX);
        }

        static void write(uint8_t data) {
            st7789_put(pio_, sm_, data);
        }

        /** By default the write cycle is 66ns, i.e. 30 MHz. 
         
            This method sets the display driver speed in percentage of the base speed above. 

            TODO this may be individual display & wiring, but on the test unit, the display so far was happy with 2 cycles per byte, which is more than a 400% overclock.
        */
        static void overclock(uint multiplier_pct = 100) {
            uint hz = 303030 * multiplier_pct; // 30Mhz already divided by 100 for the multiplier to work
            uint clk = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS) * 1000; // [Hz]
            uint clkdiv = (clk / hz);
            uint clkfrac = (clk - (clkdiv * hz)) * 256 / hz;
            pio_sm_set_clkdiv_int_frac(pio_, sm_, clkdiv & 0xffff, clkfrac & 0xff);
        }

        /** Async (where supported) write of the given buffer. 

            Always transmits the required bytes. If the transfer is asynchronous, returns true, false is returned when asynrhonous transfer is not supported (and therefore the transfer has already happened).     
        */
        static bool writeAsync(uint8_t const * data, size_t size, size_t actualSize = 0) {
            dma_ = dma_claim_unused_channel(true);
            dma_channel_config c = dma_channel_get_default_config(dma_); // create default channel config, write does not increment, read does increment, 32bits size
            channel_config_set_transfer_data_size(& c, DMA_SIZE_8); // transfer 8 bytes
            channel_config_set_dreq(& c, pio_get_dreq(pio_, sm_, true)); // tell our PIO
            // determine the wrapping
            if (actualSize != 0) {
                if (actualSize == 1)
                    channel_config_set_read_increment(& c, false);
                else
                    channel_config_set_ring(& c, false, 31 - __builtin_clz(actualSize)); 
            }        
            dma_channel_configure(dma_, & c, &pio_->txf[sm_], data, size, true); // start
            return true;
        }

        /** Returns true if there is an asynchronous data write in process. 
         
            This means that either the DMA channel is still transferring, or if DMA is done we must make sure that the PIO has stalled on an empty queue. 
        */
        static bool writeAsyncBusy() {
            return dma_channel_is_busy(dma_) || st7789_busy(pio_, sm_);
        }

        /** Waits for the asynchronous data transfer to finish, then returns. 
         
            If there is no such transfer, returns immediately. 
        */
        static void writeAsyncWait() {
            dma_channel_wait_for_finish_blocking(dma_);
            dma_channel_unclaim(dma_);
            dma_ = -1;
            st7789_wait_idle(pio_, sm_);
        }        

        static inline PIO pio_;
        static inline uint sm_;
        static inline uint offset_;
        static inline uint dma_ = -1;

    }; // rckid::ST7789


} // namespace rckid