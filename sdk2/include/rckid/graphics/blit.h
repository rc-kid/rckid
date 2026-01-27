#pragma once

#include <rckid/graphics/color.h>

namespace rckid {

    /** \name Simple blitting to RGB565
     
        Those are the simplest blitters that convert from various color representations to RGB565, which is the rendering format on RCKid. All functions take the pixel array source point, Color::RGB565 destination buffer and number of pixels to blit. Indexed formats also take a palette that will be used to convert the indices to the actual RGB565 colors.

        The functions return the number of bytes processed from the source buffer. This value depends on the bit depth of the source format and will always be an integer number (hence why numPixels must be even for 4bpp sources).
     */
    //@{
    uint32_t blit_rgb565(uint8_t const * src, Color::RGB565 * dst, uint32_t numPixels); 

    uint32_t blit_rgb332(uint8_t const * src, Color::RGB565 * dst, uint32_t numPixels);

    uint32_t blit_index256(uint8_t const * src, Color::RGB565 * dst, uint32_t numPixels, Color::RGB565 const * palette);

    uint32_t blit_index16(uint8_t const * src, Color::RGB565 * dst, uint32_t numPixels, Color::RGB565 const * palette);
    //@}
}