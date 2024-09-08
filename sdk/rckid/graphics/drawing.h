#pragma once

#include "../rckid.h"
#include "geometry.h"
#include "color.h"

namespace rckid {

    template<typename T>
    class Renderer;

    /** \defgroup drawing Drawing 
     
        Drawing is done on pixel buffers, with varying sizes and bit depth. Due to the physical construction of the device (the display is rotated), the pixel data is stored in column-wise format starting from top-right to bottom-left (DisplayMode::Native) This means that an 16bpp pixel buffer can be transferred to the display in a single DMA command without tearing effect. 

        Other 

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
    constexpr __force_inline uint32_t pixelBufferOffset(Coord x, Coord y, Coord width, Coord height) {
        return (width - x - 1) * height + y; 
    }

    /** Returns the value of given pixel.
     */
    template<typename COLOR>
    constexpr inline COLOR pixelBufferAt(uint8_t const * buffer, Coord x, Coord y, Coord width, Coord height) {
        uint32_t offset = pixelBufferOffset(x, y, width, height);
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
    constexpr inline void setPixelBufferAt(uint8_t * buffer, Coord x, Coord y, COLOR value, Coord width, Coord height) {
        uint32_t offset = pixelBufferOffset(x, y, width, height);
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
                ASSERT((numPixels & 1) == 0); // we need even number of pixels for 4 bpp
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

    /** Returns the address of the n-th column from given pixel buffer. 
     */
    template<typename COLOR>
    constexpr inline uint32_t pixelBufferColumnOffset(unsigned width, unsigned height, unsigned x) {
        return (width - 1 - x) * height * COLOR::BPP / 8;
    }
 
    /** Converts consecutive pixels from their internal format to the RGB 565 representation. 
     
        If the pixel buffer is already in the RGB format, this is a simple memcopy, otherwise each source pixel's palette color is adjusted by given offset and the color from palette is used.  

        TODO these functions are likely candidates for assembly fast methods
     */
    template<typename COLOR>
    constexpr inline ColorRGB * pixelBufferToRGB(uint8_t const * buffer, ColorRGB * out, uint32_t numPixels, ColorRGB const * palette = nullptr, uint8_t paletteOffset = 0) {
        switch (COLOR::BPP) {
            case 16:
                ASSERT(palette == nullptr);
                // ignore the paletteOffset and simply copy the appropriate number of bytes
                memcpy(out, buffer, numPixels * 2);
                return out + numPixels;
            case 8: {
                ASSERT(palette != nullptr);
                uint8_t const * pixels = reinterpret_cast<uint8_t const *>(buffer);
                while (numPixels-- != 0)
                    *(out++) = palette[(*pixels++ + paletteOffset) & 0xff];
                return out;
            }
            case 4: {
                ASSERT(palette != nullptr);
                uint8_t const * pixels = reinterpret_cast<uint8_t const *>(buffer);
                while (numPixels != 0) {
                    uint8_t p = *pixels++;
                    *(out++) = palette[((p & 0xf) + paletteOffset) & 0xff];
                    *(out++) = palette[((p >> 4) + paletteOffset) & 0xff];
                    numPixels -= 2;
                }
                return out;
            }
            default:
                UNREACHABLE;
        }
    }

    /** Converts the consecutive pixels from their internal format to the RGB 565 representation, making the last color in each format transparent. 
     
        TODO figure out how to do this properly - maybe set color value, etc
     */
    template<typename COLOR>
    inline ColorRGB * pixelBufferToRGBTransparent(uint8_t const * buffer, ColorRGB * out, unsigned numPixels, ColorRGB const * palette, uint8_t paletteOffset) {
        switch (COLOR::BPP) {
            //case 16: -- not here, specialized below
            case 8: {
                ASSERT(palette != nullptr);
                while (numPixels-- != 0) {
                    uint8_t c = *buffer++;
                    if (c == 0)
                        ++out;
                    else 
                        *(out++) = palette[(c + paletteOffset) & 0xff];
                }
                return out;
            }
            case 4: {
                ASSERT(palette != nullptr);
                uint8_t const * pixels = reinterpret_cast<uint8_t const *>(buffer);
                while (numPixels != 0) {
                    uint8_t p = *pixels++;
                    if ((p & 0xf) == 0)
                        ++out;
                    else 
                        *(out++) = palette[((p & 0xf) + paletteOffset) & 0xff];
                    if ((p >> 4) == 0)
                        ++out;
                    else
                        *(out++) = palette[((p >> 4) + paletteOffset) & 0xff];
                    numPixels -= 2;
                }
                return out;
            }
            default:
                UNREACHABLE;
        }
    }

    inline ColorRGB * pixelBufferToRGBTransparent(uint8_t const * buffer, ColorRGB * out, unsigned numPixels, ColorRGB transparent) {
        // ignore the paletteOffset and simply copy the appropriate number of bytes
        ColorRGB const * pixels = reinterpret_cast<ColorRGB const *>(buffer);
        while (numPixels-- != 0) {
            ColorRGB c = *pixels++;
            if (c != transparent)
                *(out++) = c;
            else
                ++out;
        }
        return out + numPixels;
    }

} // namespace rckid