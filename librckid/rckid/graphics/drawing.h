#pragma once

#include "rckid/rckid.h"
#include "color.h"

namespace rckid {

    /** \page Drawing 
     
        Drawing is done on pixel buffers, with varying sizes and bit depth. Due to the physical construction of the device (the display is rotated), the pixel data is stored in column-wise format starting from top-right to bottom-left. This means that an 16bpp pixel buffer can be transferred to the display in a single DMA command without tearing effect. 

        Pixel buffers support different bit depth, ranging from 16 to 2. 16 bits per pixel corresponds to the RGB 565 color being stored for each pixel, while the lower values indicate palette indices.

        Pixel buffers are always stored as uint32_t arrays, requiring their height to be divisible by 32/bpp. For 16 and 8 bpp, the pixel format within the uint32_t array corresponds to their uint16_t and uint8_t counterparts. For bpp less than 8, the pixels are ordered from first to last in the LSB bits. See details of corresponding pixel formats. 

     */

    /** Pixel format (and bits per pixel)
     */
    enum class PixelFormat {
        RGB,
        Color256, 
        Color16,
        //Color4, 
    }; 

    /** Returns the bits per pixel used by the specified pixel format. 
     */
    constexpr inline unsigned bpp(PixelFormat f) {
        switch (f) {
            case PixelFormat::RGB:
                return 16;
            case PixelFormat::Color256:
                return 8;
            case PixelFormat::Color16:
                return 4;
            //case PixelFormat::Color4:
            //    return 2;
            default:
                UNREACHABLE;
        }
    }

    /** Returns the length of a pixel buffer for given width, height and format. 
     
        NOTE The length is the number of uint32_t elements.  
     */
    constexpr inline size_t pixelBufferLength(unsigned width, unsigned height, PixelFormat fmt) { return width * height * bpp(fmt) / 32; }


    /** Packs 4 8bpp colors into a single uint32_t value 
     */
    constexpr inline uint32_t pack8(uint8_t x0, uint8_t x1, uint8_t x2, uint8_t x3) {
        return (x3 << 24) | (x2 << 16) | (x1 << 8) | x0;
    }

    /** Packs 8 4bpp colors into a single uint32_t value
     */
    constexpr inline uint32_t pack4(uint8_t x0, uint8_t x1, uint8_t x2, uint8_t x3, uint8_t x4, uint8_t x5, uint8_t x6, uint8_t x7) {
        #define P(x, y) ((y << 4) | x)
        return pack8(P(x0, x1), P(x2, x3), P(x4, x5), P(x6, x7));
        #undef P
    }

    /** Packs 16 2bpp colors into a single uin32_t value 
     */
    //constexpr inline uint32_t pack2(uint8_t x0, uint8_t x1, uint8_t x2, uint8_t x3, uint8_t x4, uint8_t x5, uint8_t x6, uint8_t x7, uint8_t x8, uint8_t x9, uint8_t x10, uint8_t x11, uint8_t x12, uint8_t x13, uint8_t x14, uint8_t x15) {
    //    #define P(x1, x2, x3, x4) ((x4 << 6) | (x3 << 4) | (x2 << 2) | x1)
    //    return pack8(P(x0, x1, x2, x3), P(x4, x5, x6, x7), P(x8, x9, x10, x11), P(x12, x13, x14, x15));
    //    #undef P
    //}

    /** Returns the value of given pixel.
     */
    template<PixelFormat FMT>
    constexpr inline unsigned pixelAt(uint32_t const * buffer, unsigned width, unsigned height, unsigned x, unsigned y) {
        switch (bpp(FMT)) {
            case 16:
                return reinterpret_cast<uint16_t const*>(buffer)[y + (width - 1 - x) * height];
            case 8:
                return reinterpret_cast<uint8_t const*>(buffer)[y + (width -1 -x) * height];
            case 4:
                return (reinterpret_cast<uint8_t const*>(buffer)[(y / 2) + (width -1 -x) * height / 2] >> ((y & 1) * 4)) & 0xf;
            //case 2:
            //    return (reinterpret_cast<uint8_t const*>(buffer)[(y / 4) + (width -1 -x) * height / 4] >> ((y & 3) * 2)) & 0x3;
            default:
                UNREACHABLE;
        }
    }

    /** Sets the value of given pixel. 
     */
    template<PixelFormat FMT>
    constexpr inline void setPixelAt(uint32_t * buffer, unsigned width, unsigned height, unsigned x, unsigned y, unsigned value) {
        switch (bpp(FMT)) {
            case 16:
                reinterpret_cast<uint16_t const*>(buffer)[y + (width - 1 - x) * height] = static_cast<uint16_t>(value);
                break;
            case 8:
                reinterpret_cast<uint8_t const*>(buffer)[y + (width -1 -x) * height] = static_cast<uint8_t>(value);
                break;
            case 4: {
                uint8_t & x = reinterpret_cast<uint8_t const*>(buffer)[y / 2 + (width -1 -x) * height / 2];
                x = x & ~(0x0f << ((y & 1) * 4));
                x |= static_cast<uint8_t>(value) << ((y & 1) * 4);
                break;
            }
            //case 2: {
            //    uint8_t & x = reinterpret_cast<uint8_t const*>(buffer)[y / 4 + (width -1 -x) * height / 4];
            //    x = x & ~(0x03 << ((y & 3) * 2));
            //    x |= static_cast<uint8_t>(value) << ((y & 3) * 2);
            //    break;
            //}
            default:
                UNREACHABLE;
        }
    }

    /** Returns the address of the n-th column from given pixel buffer. 
     */
    template<PixelFormat FMT>
    constexpr inline uint32_t const * getColumn(uint32_t const * buffer, unsigned width, unsigned height, unsigned x) {
        return buffer + (width - 1 - x) * height * bpp(FMT) / 32;
    }

    template<PixelFormat FMT>
    constexpr inline uint32_t * getColumn(uint32_t * buffer, unsigned width, unsigned height, unsigned x) {
        return const_cast<uint32_t*>(getColumn<FMT>(const_cast<uint32_t const *>(buffer), width, height, x));
    }

    /** Converts consecutive pixels from their internal format to the RGB 565 representation. 
     
        If the pixel buffer is already in the RGB format, this is a simple memcopy, otherwise each source pixel's palette color is adjusted by given offset and the color from palette is used.  

        TODO these functions are likely candidates for assembly fast methods
     */
    template<PixelFormat FMT>
    constexpr inline uint16_t * convertToRGB(uint32_t const * buffer, uint16_t * out, unsigned numPixels, ColorRGB const * palette, uint8_t paletteOffset) {
        switch (bpp(FMT)) {
            case 16:
                ASSERT(palette == nullptr);
                // ignore the paletteOffset and simply copy the appropriate number of bytes
                memcpy(out, buffer, numPixels * 2);
                return out + numPixels;
            case 8: {
                ASSERT(palette != nullptr);
                uint8_t const * pixels = reinterpret_cast<uint8_t const *>(buffer);
                while (numPixels-- != 0)
                    *(out++) = palette[(*pixels++ + paletteOffset) & 0xff].rawValue16();
                return out;
            }
            case 4: {
                ASSERT(palette != nullptr);
                uint8_t const * pixels = reinterpret_cast<uint8_t const *>(buffer);
                while (numPixels != 0) {
                    uint8_t p = *pixels++;
                    *(out++) = palette[((p & 0xf) + paletteOffset) & 0xff].rawValue16();
                    *(out++) = palette[((p >> 4) + paletteOffset) & 0xff].rawValue16();
                    numPixels -= 2;
                }
                return out;
            }
            default:
                UNREACHABLE;
        }
    }


} // namespace rckid