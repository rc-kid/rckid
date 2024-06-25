#pragma once

#include "platform/platform.h"
#include "platform/fonts.h"

namespace platform { 

    /** PCD8544 aka the nokia 5110 display. 
     
        RST connection is not absolutely necessary. When not used it should be held high. 
    */

    template<gpio::Pin RST, gpio::Pin SCE, gpio::Pin DC>
    class PCD8544 {
    public:
        PCD8544() {
            gpio::output(RST);
            gpio::output(SCE);
            gpio::output(DC);
            gpio::high(RST);
            gpio::high(SCE);
            gpio::high(DC);
        }
        
        void reset() {
            gpio::low(RST);
            cpu::delayMs(10);
            gpio::high(RST);
        }
        
        void enable() {
            reset();
            spi::begin(SCE);
            writeCommand(0x21); // extended commands, power on
            writeCommand(0xbf); // VOP (contrast)
            writeCommand(0x04); // temp coef 
            writeCommand(0x14); // bias mode 1:48
            writeCommand(0x20); // basic commands, power on
            normalMode();
            gotoXY(0,0);
            clear();
            spi::end(SCE);
        }
        
        void disable() {
            spi::begin(SCE);
            writeCommand(0b00100100);
            spi::end(SCE);
        }
        
        void normalMode() {
            spi::begin(SCE);
            writeCommand(0x0c); // normal mode
            spi::end(SCE);
        }
        
        void inverseMode() {
            spi::begin(SCE);
            writeCommand(0x0d); // inverse mode
            spi::end(SCE);
        }
        
        void clear() {
            spi::begin(SCE);
            for (int i = 0; i < 84 * 48 / 8; ++i)
                writeData(0);	
            spi::end(SCE);
        }
        
        void gotoXY(uint8_t col, uint8_t row) {
            spi::begin(SCE);
            writeCommand(0x80 | col);
            writeCommand(0x40 | row);	
            spi::end(SCE);
        }
        
        /** Displays a single character.
         */
        void write(char x) {
            spi::begin(SCE);
            writeChar(x);
            spi::end(SCE);
        }
        
        /** Writes a null-terminated string. 
         */
        void write(char const * x) {
            spi::begin(SCE);
            while (*x != 0) {
                writeChar(*x);
                ++x;
            }
            spi::end(SCE);
        }

        /** Writes a null-terminated string to a given position. 
         */
        void write(uint8_t col, uint8_t row, char const * x) {
            spi::begin(SCE);
            writeCommand(0x80 | col);
            writeCommand(0x40 | row);	
            while (*x != 0) {
                writeChar(*x);
                ++x;
            }
            spi::end(SCE);
        }

        /** Writes a null-terminated string to a given position, using scaled font (2x larger) to a given position. 
         
            The large text uses the same font, but doubles each pixel, i.e. a cell size will become 10x16.
        */
        void writeLarge(uint8_t col, uint8_t row, char const * x) {
            spi::begin(SCE);
            writeCommand(0x80 | col);
            writeCommand(0x40 | row);	
            char const * xx = x;
            while (*xx != 0) {
                uint8_t * d = Font::basic + (*xx - 0x20) * 5;
                for (uint8_t i = 0; i < 5; ++i) {
                    uint8_t v = expandNibble(d[i]);
                    writeData(v);
                    writeData(v);
                }
                writeData(0);
                writeData(0);
                ++xx;
            }
            writeCommand(0x80 | col);
            writeCommand(0x40 | row + 1);
            while (*x != 0) {
                uint8_t * d = Font::basic + (*x - 0x20) * 5;
                for (uint8_t i = 0; i < 5; ++i) {
                    uint8_t v = expandNibble(d[i] >> 4);
                    writeData(v);
                    writeData(v);
                }
                writeData(0);
                writeData(0);
                ++x;
            }
            spi::end(SCE);
        }
        
        /** Writes an unsigned 16bit integer. 
         
            If the fill character is '\0' (default), the number will be left-justified, if ' ' it will be right justified, '0' will print all leading zeros as well. 
        */
        void write(uint16_t x, char fill = '\0') {
            spi::begin(SCE);
            for (uint16_t i = 10000; i > 0; i = i / 10) {
                if (x > i || i == 1) {
                    writeChar((x / i) + '0');
                    fill = '0';
                    x = x % i;
                } else if (fill != '\0') {
                    writeChar(fill);
                }
            }
            spi::end(SCE);
        }

        
    private:

        void writeCommand(uint8_t cmd) {
            gpio::low(DC);
            spi::transfer(cmd);
            gpio::high(DC);
        }
        
        void writeData(uint8_t data) {
            spi::transfer(data);
        }

        void writeData(uint8_t * data, uint16_t size) {
            spi::send(data, size);
        }

        void writeChar(char c) {
            writeData(Font::basic + (c - 0x20) * 5, 5);
            //writeData(0);
        }
    } ;

} // namespace platform
