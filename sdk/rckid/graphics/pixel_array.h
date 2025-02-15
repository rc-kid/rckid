#pragma once

#include "../rckid.h"
#include "geometry.h"
#include "color.h"
#include "font.h"

namespace rckid {

    /** Pixel Array 
     
        Drawing operations on pixel buffers. The pixel array provides basic operations on pixel buffers in the native display orientation so that the routines can be reused by different graphic classes. 
     */
    template<uint32_t BPP>
    class PixelArray {
        static_assert(BPP == 16 || BPP == 8 || BPP == 4 || BPP == 2, "Only 16, 8, 4, or 2 bpp supported");
    public:

        using Pixel = std::conditional_t<BPP == 16, uint16_t, uint8_t>;

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
        static constexpr Pixel get(Coord x, Coord y, Coord width, Coord height, Pixel const * array) {
            switch (BPP) {
                case 16:
                case 8:
                    return array[offset(x, y, width, height)];
                case 4: {
                    uint32_t o = offset(x, y, width, height);
                    return (array[o >> 1] >> ((o & 1) * 4)) & 0x0f;
                }
                case 2: {
                    uint32_t o = offset(x, y, width, height);
                    return (array[o >> 2] >> ((o & 3) * 2)) & 0x03;
                }
                default:
                    UNREACHABLE;
            }
        }

        /** Sets the value of given pixel. If the coordinates are outside of the pixel array bounds, does nothing. 
         */
        static constexpr void set(Coord x, Coord y, Coord width, Coord height, Pixel value, Pixel * array) {
            // don't do anything if outside of bounds
            if (x < 0 || y < 0 || x >= width || y >= height)
                return;
            setNoCheck(x, y, width, height, value, array);
        }

        /** Sets the value of given pixel assuming the coordinates are within the pixel array bounds.
         */
        static constexpr void setNoCheck(Coord x, Coord y, Coord width, Coord height, Pixel value, Pixel * array) {
            switch (BPP) {
                case 16:
                case 8:
                    array[offset(x, y, width, height)] = value;
                    return;
                case 4: {
                    uint32_t o = offset(x, y, width, height);
                    Pixel & xx = array[o >> 1];
                    xx &= ~(0x0f << ((o & 1) * 4));
                    xx |= value << ((o & 1) * 4);
                    return;
                }
                case 2: {
                    uint32_t o = offset(x, y, width, height);
                    Pixel & xx = array[o >> 2];
                    xx &= ~(0x03 << ((o & 3) * 2));
                    xx |= value << ((o & 3) * 2);
                    return;
                }
                default:
                    UNREACHABLE;
            }
        }

        static constexpr Coord putChar(Coord x, Coord y, Coord width, Coord height, Font const & font, char c, Pixel const * colors, Pixel * pixels) {
            GlyphInfo const & g = font.glyphs[static_cast<uint8_t>((c - 32 >= 0) ? (c - 32) : 0)];
            // if the start is after, or the end is before the current bitamp, rsimply advance
            if (x > width || x + g.advanceX < 0)
                return g.advanceX;
            uint8_t const * gPixels = font.pixels + g.index;
            int ys = y + g.y;
            int ye = ys + g.height;
            for (int xx = x + g.x,xe = x + g.x + g.width; xx < xe; ++xx) {
                uint32_t col;
                uint32_t bits = 0;
                for (int yy = ys; yy != ye; ++yy) {
                    if (bits == 0) {
                        bits = 8;
                        col = *gPixels++;
                    }
                    unsigned a = (col >> 6) & 0x3;
                    if (a != 0)
                        set(xx, yy, width, height, colors[a], pixels);
                    col = col << 2;
                    bits -= 2;
                }
            }
            return g.advanceX;
        }
    }; // PixelArray

} // namespace rckid