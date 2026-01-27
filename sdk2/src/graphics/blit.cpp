#include <rckid/graphics/blit.h>

namespace rckid {

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
        ASSERT(numPixels % 2 == 0);
        uint16_t * destination = reinterpret_cast<uint16_t *>(dst);
        for (uint32_t i = 0; i < numPixels; numPixels += 2) {
            *(destination++) = palette[static_cast<uint8_t>(src[i] >> 4)];
            *(destination++) = palette[static_cast<uint8_t>(src[i] & 0x0f)];
        }
        return numPixels / 2; // bpp is just 4
    }


} // namespace rckid