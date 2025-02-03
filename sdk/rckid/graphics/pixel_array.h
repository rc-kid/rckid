#pragma once

#include "../rckid.h"
#include "geometry.h"
#include "color.h"
#include "font.h"

namespace rckid {

    /** Pixel Array 
     
        Drawing operations on pixel  buffers. 
     */
    template<uint32_t BPP>
    class PixelArray {
    public:

        /** Returns number of bytes a pixel array of given dimension needs for the specified color. 
         
            Single column must fit exactly into an uint8_t array, which is not a problem for RGB and 256 colors, but forces 16 color buffers to have even heights.  
        */
        static constexpr uint32_t numBytes(Coord width, Coord height) {
            ASSERT(height * BPP % 8 == 0);
            return width * height * BPP / 8;
        }

        /** Returns the pixel offset for pixel at coordinates (x,y) in a pixel buffer of specified width and height. Assumes the native display orientation, i.e. right-top corner is index 0, column-first format. 
         */
        static constexpr uint32_t offset(Coord x, Coord y, Coord width, Coord height) {
            return (width - x - 1) * height + y; 
        }

        /** Returns the value of given pixel.
         */
        static constexpr uint16_t get(Coord x, Coord y, Coord width, Coord height, uint8_t const * array) {
            uint32_t i = offset(x, y, width, height); 
            switch (BPP) {
                case 16:
                    return reinterpret_cast<uint16_t const *>(array)[i];
                case 8:
                    return array[i];
                case 4:
                    return (array[i >> 1] >> ((i & 1) * 4)) & 0x0f;
                default:
                    UNREACHABLE;
            }
        }

        /** Sets the value of given pixel. If the coordinates are outside of the pixel array bounds, does nothing. 
         */
        static constexpr void set(Coord x, Coord y, Coord width, Coord height, uint16_t value, uint8_t * array) {
            // don't do anything if outside of bounds
            if (x < 0 || y < 0 || x >= width || y >= height)
                return;
            setNoCheck(x, y, width, height, value, array);
        }

        /** Sets the value of given pixel assuming the coordinates are within the pixel array bounds.
         */
        static constexpr void setNoCheck(Coord x, Coord y, Coord width, Coord height, uint16_t value, uint8_t * array) {
            uint32_t i = offset(x, y, width, height); 
            switch (BPP) {
                case 16:
                    reinterpret_cast<uint16_t *>(array)[i] = value & 0xffff;
                    break;
                case 8:
                    array[i] = value & 0xff;
                    break;
                case 4: {
                    uint8_t & xx = array[i >> 1];
                    xx &= ~(0x0f << ((i & 1) * 4));
                    xx |= (value & 0xf) << ((i & 1) * 4);
                    break;
                }
                default:
                    UNREACHABLE;
            }
        }

        static constexpr Coord putChar(Coord x, Coord y, Coord width, Coord height, Font const & font, char c, uint16_t const * colors, uint8_t * array) {
            GlyphInfo const & g = font.glyphs[static_cast<uint8_t>((c - 32 >= 0) ? (c - 32) : 0)];
            // if the start is after, or the end is before the current bitamp, rsimply advance
            if (x > width || x + g.advanceX < 0)
                return g.advanceX;
            uint8_t const * pixels = font.pixels + g.index;
            int ys = y + g.y;
            int ye = ys + g.height;
            for (int xx = x + g.x,xe = x + g.x + g.width; xx < xe; ++xx) {
                uint32_t col;
                uint32_t bits = 0;
                for (int yy = ys; yy != ye; ++yy) {
                    if (bits == 0) {
                        bits = 8;
                        col = *pixels++;
                    }
                    unsigned a = (col >> 6) & 0x3;
                    if (a != 0)
                        set(xx, yy, width, height, colors[a], array);
                    col = col << 2;
                    bits -= 2;
                }
            }
            return g.advanceX;
        }
    }; // PixelArray

} // namespace rckid