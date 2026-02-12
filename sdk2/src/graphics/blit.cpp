#include <rckid/graphics/blit.h>

namespace rckid {

    // simple blitting

    __attribute__((weak))
    uint32_t blit_rgb565(uint8_t const * src, Color::RGB565 * dst, uint32_t numPixels) {
        uint16_t const * source = reinterpret_cast<uint16_t const *>(src);
        uint16_t * destination = reinterpret_cast<uint16_t *>(dst);
        for (uint32_t i = 0; i < numPixels; ++i)
            destination[i] = source[i];
        return numPixels * 2; // bpp is 16
    }

    __attribute__((weak))
    uint32_t blit_rgb332(uint8_t const * src, Color::RGB565 * dst, uint32_t numPixels) {
        Color::RGB332 const * source = reinterpret_cast<Color::RGB332 const *>(src);
        uint16_t * destination = reinterpret_cast<uint16_t *>(dst);
        for (uint32_t i = 0; i < numPixels; ++i)
            destination[i] = source[i];
        return numPixels; // bpp is 8
    }

    __attribute__((weak))
    uint32_t blit_index256(uint8_t const * src, Color::RGB565 * dst, uint32_t numPixels, Color::RGB565 const * palette) {
        Color::Index256 const * source = reinterpret_cast<Color::Index256 const *>(src);
        uint16_t * destination = reinterpret_cast<uint16_t *>(dst);
        for (uint32_t i = 0; i < numPixels; ++i) {
            destination[i] = palette[static_cast<uint8_t>(source[i])];
        }
        return numPixels; // bpp is 8
    }

    __attribute__((weak))
    uint32_t blit_index16(uint8_t const * src, Color::RGB565 * dst, uint32_t numPixels, Color::RGB565 const * palette) {
        uint16_t * destination = reinterpret_cast<uint16_t *>(dst);
        uint32_t pixelsToDraw = numPixels;
        while (pixelsToDraw >= 2) {
            uint8_t byte = *src++;
            *(destination++) = palette[byte & 0x0f];
            *(destination++) = palette[byte >> 4];
            pixelsToDraw -= 2;
        }
        if (pixelsToDraw == 1) {
            uint8_t byte = *src;
            *destination = palette[byte & 0x0f];
        }
        return numPixels / 2;
    }

    // transparent blitting

    __attribute__((weak))
    uint32_t blit_rgb565(uint8_t const * src, Color::RGB565 * dst, uint32_t numPixels, uint32_t transparentColor) {
        uint16_t const * source = reinterpret_cast<uint16_t const *>(src);
        uint16_t * destination = reinterpret_cast<uint16_t *>(dst);
        for (uint32_t i = 0; i < numPixels; ++i)
            if (source[i] != transparentColor)
                destination[i] = source[i];
        return numPixels * 2; // bpp is 16
    }

    __attribute__((weak))
    uint32_t blit_rgb332(uint8_t const * src, Color::RGB565 * dst, uint32_t numPixels, uint32_t transparentColor) {
        Color::RGB332 const * source = reinterpret_cast<Color::RGB332 const *>(src);
        uint16_t * destination = reinterpret_cast<uint16_t *>(dst);
        for (uint32_t i = 0; i < numPixels; ++i)
            if (static_cast<uint8_t>(source[i]) != transparentColor)
                destination[i] = source[i];
        return numPixels; // bpp is 8
    }

    __attribute__((weak))
    uint32_t blit_index256(uint8_t const * src, Color::RGB565 * dst, uint32_t numPixels, Color::RGB565 const * palette, uint32_t transparentColor) {
        Color::Index256 const * source = reinterpret_cast<Color::Index256 const *>(src);
        uint16_t * destination = reinterpret_cast<uint16_t *>(dst);
        for (uint32_t i = 0; i < numPixels; ++i) {
            if (static_cast<uint8_t>(source[i]) != transparentColor)
                destination[i] = palette[static_cast<uint8_t>(source[i])];
        }
        return numPixels; // bpp is 8
    }

    __attribute__((weak))
    uint32_t blit_index16(uint8_t const * src, Color::RGB565 * dst, uint32_t numPixels, Color::RGB565 const * palette, uint32_t transparentColor) {
        uint16_t * destination = reinterpret_cast<uint16_t *>(dst);
        uint32_t pixelsToDraw = numPixels;
        while (pixelsToDraw >= 2) {
            uint8_t byte = *src++;
            if ((byte & 0x0f) != transparentColor)
                 *(destination++) = palette[byte & 0x0f];
            else
                destination++;
            if ((byte >> 4) != transparentColor)
                 *(destination++) = palette[byte >> 4];
            else
                destination++;
            pixelsToDraw -= 2;
        }
        if (pixelsToDraw == 1) {
            uint8_t byte = *src;
            if ((byte & 0x0f) != transparentColor)
                 *destination = palette[byte & 0x0f];
        }   
        return numPixels / 2; 
    }


} // namespace rckid