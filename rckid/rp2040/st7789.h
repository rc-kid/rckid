#pragma once

#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/dma.h>
#include <hardware/clocks.h>

#include "platform/platform.h"

#include "st7789.pio.h"

#include "rckid.h"

namespace rckid {

    /** A single pixel. 
     
    */
    class Pixel {
    public:

        Pixel():
            raw_{0} {
        }

        static constexpr Pixel RGB(uint8_t r, uint8_t g, uint8_t b) {
            g = (g >= 16) ? (g << 1) : ((g << 1) + 1);
            return RGBFull(r, g, b);
        }
        static constexpr Pixel RGBFull(uint8_t r, uint8_t g, uint8_t b) {
            return Pixel{
                ((r & 31) << 3) |
                ((g >> 3) & 7) | ((g & 7) << 13) |
                (b & 31) << 8
            };
        }

        static constexpr Pixel Black() { return Pixel::RGB(0,0,0); }
        static constexpr Pixel Red() { return Pixel::RGB(31, 0, 0); };
        static constexpr Pixel Green() { return Pixel::RGB(0, 31, 0); };
        static constexpr Pixel Blue() { return Pixel::RGB(0, 0, 31); };
        static constexpr Pixel White() { return Pixel::RGB(31, 31, 31); };

        uint8_t r() const { return (raw_ >> 8) & 31; }
        uint8_t g() const { return ((raw_ & 7) << 2) | (( raw_ >> 14) & 3); }
        uint8_t b() const { return (raw_ >> 3) & 31; }

        Pixel & r(uint8_t value) {
            raw_ &= ~RED_MASK;
            raw_ |= (value << 8) & RED_MASK;
            return *this;
        }

        Pixel & g(uint8_t value) {
            value = (value >= 16) ? (value << 1) : ((value << 1) + 1);
            return gFull(value);
        }

        Pixel & b(uint8_t value) {
            raw_ &= ~BLUE_MASK;
            raw_ |= (value << 3) & BLUE_MASK;
            return * this;
        }

        uint8_t gFull() const { return ((raw_ & 7) << 3) | (( raw_ >> 13) & 7); }

        Pixel & gFull(uint8_t value) {
            raw_ &= ~GREEN_MASK;
            raw_ |= ((value >> 3) & 7) | ((value & 7) << 13);
            return *this;
        }

        constexpr operator uint16_t () {
            return raw_;
        }

    private:

        constexpr Pixel(int raw):
            raw_{static_cast<uint16_t>(raw)} {
        }

        // GGGRRRRRBBBBBGGG
        static constexpr uint16_t RED_MASK = 0b0001111100000000;
        static constexpr uint16_t GREEN_MASK = 0b1110000000000111;
        static constexpr uint16_t BLUE_MASK = 0b0000000011111000;
        uint16_t raw_;
    };

    static_assert(sizeof(Pixel) == 2, "Wrong pixel size!");

    /** ST7789 display driver

        The display used is the 2.8" IPS panel from aliexpress (https://www.aliexpress.com/item/1005004635814413.html?spm=a2g0o.order_list.order_list_main.5.1d8a1802PNYgMn). Other displays should technically work as well, the 8bit parallel interface is the same between ST7789 and the more common ILI9341 found in non-IPS panels, but the commands can differ. 

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
            overclock(100); // set base frequency
            // initialize the display
            sendCommand8(VERTICAL_SCROLL_START, 0);
            sendCommand8(PIXEL_FORMAT, PIXEL_FORMAT_16);
            sendCommand8(TEARING_LINE_ON, TEARING_LINE_MODE_VSYNC);
            sendCommand(SLEEP_OUT, nullptr, 0);
            cpu::delayMs(150);
            sendCommand(DISPLAY_ON, nullptr, 0);
            cpu::delayMs(150);

            rotate(90);

            uint8_t * data = new uint8_t[3200];
            for (size_t i = 0; i < sizeof(data); ++i)
                data[i] = i;

            fillAsync(data, 3200);
            for (int i = 0; i < 5;++i) {
                sendCommand(INVERSE_OFF, nullptr, 0);
                cpu::delayMs(500);
                sendCommand(INVERSE_ON, nullptr, 0);
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

        /** Fills the entire screet with given color. 
         */
        static void fill(Pixel p) {
            setColumnUpdateRange();
            setRowUpdateRange();
            begin();
            setCommandMode();
            write(RAM_WRITE);
            setDataMode();
            for (int i = 0; i < 240; ++i) {
                for (int j = 0; j < 320; ++j) {
                    /*
                    if (j <= 240 - i) 
                        p.r(i % 32).b(j % 32).g(0);
                    else
                        p.r(0).b(0).gFull(j % 64);
                    */
                    write(static_cast<uint16_t>(p) >> 8);
                    write(static_cast<uint16_t>(p) & 0xff);
                }
            }
            end();
        }

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
        write(RAM_WRITE);
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
            sendCommand16(COLUMN_ADDRESS_SET, start, end);
        }

        static void setRowUpdateRange(unsigned start = 0, unsigned end = 239) {
            sendCommand16(PAGE_ADDRESS_SET, start, end);
        }


    private:


        static constexpr uint8_t SLEEP_IN = 0x10;
        static constexpr uint8_t SLEEP_OUT = 0x11;
        static constexpr uint8_t NORMAL_MODE = 0x13;
        static constexpr uint8_t INVERSE_OFF = 0x20;
        static constexpr uint8_t INVERSE_ON = 0x21;
        static constexpr uint8_t DISPLAY_OFF = 0x28;
        static constexpr uint8_t DISPLAY_ON = 0x29;

        static constexpr uint8_t COLUMN_ADDRESS_SET = 0x2a;
        static constexpr uint8_t PAGE_ADDRESS_SET = 0x2b;
        static constexpr uint8_t RAM_WRITE = 0x2c;


        static constexpr uint8_t TEARING_LINE_OFF = 0x34;
        static constexpr uint8_t TEARING_LINE_ON = 0x35;
        static constexpr uint8_t TEARING_LINE_MODE_VSYNC = 0;
        static constexpr uint8_t TEARING_LINE_MODE_V_AND_H_SYNC = 1;

        static constexpr uint8_t MADCTL = 0x36;
        static constexpr uint8_t MADCTL_MY = 0x80;  // Bottom to top
        static constexpr uint8_t MADCTL_MX = 0x40;  // Right to left
        static constexpr uint8_t MADCTL_MV = 0x20;  // Reverse Mode
        static constexpr uint8_t MADCTL_ML = 0x10;  // LCD refresh Bottom to top
        static constexpr uint8_t MADCTL_RGB = 0x00; // Red-Green-Blue pixel order
        static constexpr uint8_t MADCTL_BGR = 0x08; // Blue-Green-Red pixel order
        static constexpr uint8_t MADCTL_MH = 0x04;  // LCD refresh right to left    

        static constexpr uint8_t VERTICAL_SCROLL_START = 0x37;


        static constexpr uint8_t PIXEL_FORMAT = 0x3a;
        static constexpr uint8_t PIXEL_FORMAT_16 = 0x05; // 16 bit pixel format



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