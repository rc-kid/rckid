#pragma once

#include "../rckid.h"
#include "geometry.h"
#include "color.h"

namespace rckid {

    /** \defgroup drawing Drawing 
     
        Drawing is done on pixel buffers, with varying sizes and bit depth. Due to the physical construction of the device (the display is rotated), the pixel data is stored in column-wise format starting from top-right to bottom-left (DisplayMode::Native) This means that an 16bpp pixel buffer can be transferred to the display in a single DMA command without tearing effect. 

        Note that all color buffers are expected to be aligned as uint32_t. This is the case if they were obtained by new or malloc. 
     */
    
    /** Returns number of bytes a pixel buffer of given dimension needs for the specified color. 
     
        Single column must fit exactly into an uint8_t array, which is not a problem for RGB and 256 colors, but forces 16 color buffers to have even heights.  
    */
    template<typename COLOR>
    constexpr inline uint32_t pixelBufferSize(Coord width, Coord height) {
        ASSERT(height * COLOR::BPP % 8 == 0);
        return width * height * COLOR::BPP / 8;
    }

    /** Returns the pixel offset for pixel at coordinates (x,y) in a pixel buffer of specified width and height. Assumes the native display orientation, i.e. right-top corner is index 0, column-first format. 
      */
    constexpr __force_inline uint32_t pixelOffset(Coord x, Coord y, Coord width, Coord height) {
        return (width - x - 1) * height + y; 
    }

    /** Returns the value of given pixel.
     */
    template<typename COLOR>
    constexpr inline COLOR pixelAt(uint8_t const * buffer, Coord x, Coord y, Coord width, Coord height) {
        uint32_t offset = pixelOffset(x, y, width, height);
        switch (COLOR::BPP) {
            case 16:
            case 8:
                return reinterpret_cast<COLOR const*>(buffer)[offset];
            case 4: {
                return COLOR::fromRaw((buffer[offset >> 1] >> ((offset & 1) * 4)) & 0x0f);
            }
            default:
                UNREACHABLE;
        }
    }

    /** Sets the value of given pixel. 
     */
    template<typename COLOR>
    constexpr inline void setPixelAt(uint8_t * buffer, Coord x, Coord y, COLOR value, Coord width, Coord height) {
        uint32_t offset = pixelOffset(x, y, width, height);
        switch (COLOR::BPP) {
            case 16:
            case 8:
                reinterpret_cast<COLOR*>(buffer)[offset] = value;;
                break;
            case 4: {
                uint8_t & xx = buffer[offset >> 1];
                xx &= ~(0x0f << ((offset & 1) * 4));
                xx |= value.toRaw() << ((offset & 1) * 4);
                break;
            }
            default:
                UNREACHABLE;
        }
    }

    /** Fills the entire buffer with given color. 
     */
    template<typename COLOR>
    constexpr void pixelBufferFill(uint8_t * buffer, uint32_t numPixels, COLOR color) {
        uint32_t value = color.toRaw();
        switch (COLOR::BPP) {
            case 4:
                ASSERT(numPixels & 1 == 0); // we need even number of pixels for 4 bpp
                value = (value << 4) | value;
                numPixels /= 2;
                [[fallthrough]];
            case 8:
                if (numPixels % 1)
                    return memFill(buffer, numPixels, static_cast<uint8_t>(value));
                value = (value << 8) | value;
                numPixels /= 2;
                [[fallthrough]];
            case 16:
                if (numPixels % 1)
                    return memFill(reinterpret_cast<uint16_t*>(buffer), numPixels, static_cast<uint16_t>(value));
                value = (value << 16) | value;
                numPixels /= 2;
                [[fallthrough]];
            case 32:
                ASSERT(COLOR::BPP != 32); // we actually don't support 32 bpp, this is here only for faster memop
                return memFill(reinterpret_cast<uint32_t*>(buffer), numPixels, value);
            default:
                UNREACHABLE;
        }
    }




} // namespace rckid