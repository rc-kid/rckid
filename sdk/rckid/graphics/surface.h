#pragma once

#include "../rckid.h"
#include "image_decoder.h"
#include "font.h"

namespace rckid {

    /** Maps coordinates into a 2D array into one dimensional array in a column first manner where the first indes is mapped to the last column, first row. This mapping is tailored to the native display rendering where column by column rendering means simply incrementing the array index after the first one.  
     */
    constexpr inline uint32_t mapIndexColumnFirst(Coord x, Coord y, Coord width, Coord height) {
        return (width - x - 1) * height + y;
    }

    template<uint32_t BITS_PER_PIXEL>
    class Surface {
    public:

        static constexpr uint32_t BPP = BITS_PER_PIXEL;

        static_assert(BPP == 16 || BPP == 8 || BPP == 4 || BPP == 2 || BPP == 1, "Only 16, 8, 4, 2 or 1 bpp supported");

        /** Returns the size in bytes for a surface of given dimensions. 
         */
        static constexpr uint32_t numBytes(Coord width, Coord height) {
            return (width * height * BPP) / 8;
        }

        /** Returns the sise in half-words (which is the required type for surface pixels) for surface of given dimensions.
         */
        static constexpr uint32_t numHalfWords(Coord width, Coord height) {
            return numBytes(width, height) / sizeof(uint16_t);
        }

        /** Single pixel access
         
            This is excruciatingly slow interface that provides pixel granularity access to the surface. They serve as a backup for default implementations and for tiny single pixel adjustments. For any real graphics, the blitting and rendering functions below should be used.  
         */
        static constexpr uint32_t pixelAt(Coord x, Coord y, Coord width, Coord height, uint16_t const * pixels_) {
            uint32_t offset = mapIndexColumnFirst(x, y, width, height);
            switch (BPP) {
                case 16: 
                    return pixels_[offset];
                case 8:
                    return reinterpret_cast<uint8_t const *>(pixels_)[offset];
                case 4: {
                    uint8_t x = reinterpret_cast<uint8_t const *>(pixels_)[offset / 2];
                    return (offset & 1) ? (x >> 4) : (x & 0xf);
                }
                case 2: {
                    uint8_t x = reinterpret_cast<uint8_t const *>(pixels_)[offset / 4];
                    return (x >> ((offset & 4) * 2)) & 0x3;
                }
                case 1: {
                    uint8_t x = reinterpret_cast<uint8_t const *>(pixels_)[offset / 8];
                    return (x >> (offset & 7)) & 0x1;
                }
            }
        }

        static constexpr void setPixelAt(Coord x, Coord y, Coord width, Coord height, uint16_t * pixels_, uint32_t color) {
            uint32_t offset = mapIndexColumnFirst(x, y, width, height);
            switch (BPP) {
                case 16: 
                    pixels_[offset] = color;
                    return;
                case 8:
                    reinterpret_cast<uint8_t*>(pixels_)[offset] = color;
                    return;
                case 4: {
                    uint8_t & x = reinterpret_cast<uint8_t*>(pixels_)[offset / 2];
                    x = x & ((offset & 1) ? 0x0f : 0xf0);
                    x |= color << ((offset & 1) * 4);
                    return;
                }
                case 2: {
                    uint8_t & x = reinterpret_cast<uint8_t*>(pixels_)[offset / 4];
                    x = x & ~ (0x03 << ((offset & 4) * 2));
                    x |= color << ((offset & 4) * 2);
                    return;
                }
                case 1: {
                    uint8_t & x = reinterpret_cast<uint8_t*>(pixels_)[offset / 8];
                    x = x & ~(0x01 << (offset & 7));
                    x |= color << (offset & 7);
                    return;
                }
            }
        }

        /** Blitting functions
          
            The blitting functions copy regions (entire surface, rectangle, single column or single row) from one surface to another, assuming identical source and target bpp. This means blitting does not have to deal with palettes and palette to color transformations, unlike rendering, which always renders to full 16bpp colors. 

         */
        static void blit(uint16_t const * src, Coord srcWidth, Coord srcHeight, uint16_t * dst, Coord dstX, Coord dstY, Coord dstWidth, Coord dstHeight) {
            for (Coord x = srcWidth - 1; x < srcWidth; --x) 
                for (Coord y = 0; y < srcHeight; ++y)
                    setPixelAt(x + dstX, y + dstY, dstWidth, dstHeight, dst, pixelAt(x, y, srcWidth, srcHeight, src));
        }

        static void blitRect(uint16_t const * src, Rect srcRect, Coord srcWidth, Coord srcHeight, uint16_t * dst, Coord dstX, Coord dstY, Coord dstWidth, Coord dstHeight) {
            for (Coord x = srcRect.left(), xe = srcRect.right(); x < xe; ++x)
                for (Coord y = srcRect.top(), ye = srcRect.bottom(); y < ye;++y)
                    setPixelAt(x + dstX, y + dstY, dstWidth, dstHeight, dst, pixelAt(x, y, srcWidth, srcHeight, src));
        }

        static uint32_t blitColumn(uint16_t const * src, Coord srcColumn, Coord srcStartRow, Coord numPixels, Coord srcWidth, Coord srcHeight, uint16_t * dst) {
            ASSERT(numPixels * BPP % 16 == 0);
            // extremely inefficient implementation where we go pixel by pixel and treat the output buffer as a single column surface
            for (Coord y = srcStartRow, ye = srcStartRow + numPixels, i = 0; y < ye; ++y, ++i)
                setPixelAt(0, i, 1, numPixels, dst, pixelAt(srcColumn, y, srcWidth, srcHeight, src));
            return numPixels * BPP / 16;
        }

        static uint32_t blitRow(uint16_t const * src, Coord srcRow, Coord srcStartColumn, Coord numPixels, Coord srcWidth, Coord srcHeight, uint16_t * dst) {
            ASSERT(numPixels * BPP % 16 == 0);
            // inefficient implementation where we go pixel by pixel and treat the output buffer as a single column surface (although rows are bound to be inefficient)
            for (Coord x = srcStartColumn, xe = srcStartColumn + numPixels, i = 0; x < xe; ++x, ++i)
                setPixelAt(0, i, 1, numPixels, dst, pixelAt(x, srcRow, srcWidth, srcHeight, src));
            return numPixels * BPP / 16;
        }

        /** Transparent blitting. 
         
            This is identical to normal blitting, i.e. only works between surfaces with identical bit depth, but also allows specifying transparent color for the source. 
         */
        static void blit(uint16_t const * src, Coord srcWidth, Coord srcHeight, uint16_t * dst, Coord dstX, Coord dstY, Coord dstWidth, Coord dstHeight, uint32_t transparent) {
            for (Coord x = srcWidth - 1; x < srcWidth; --x) 
                for (Coord y = 0; y < srcHeight; ++y) {
                    uint32_t c = pixelAt(x, y, srcWidth, srcHeight, src);
                    if (c != transparent)
                        setPixelAt(x + dstX, y + dstY, dstWidth, dstHeight, dst, c);
                }
        }

        static void blitRect(uint16_t const * src, Rect srcRect, Coord srcWidth, Coord srcHeight, uint16_t * dst, Coord dstX, Coord dstY, Coord dstWidth, Coord dstHeight, uint32_t transparent) {
            for (Coord x = srcRect.left(), xe = srcRect.right(); x < xe; ++x)
                for (Coord y = srcRect.top(), ye = srcRect.bottom(); y < ye;++y) {
                    uint32_t c = pixelAt(x, y, srcWidth, srcHeight, src);
                    if (c != transparent)
                        setPixelAt(x + dstX, y + dstY, dstWidth, dstHeight, dst, c);
                }
        }

        static uint32_t blitColumn(uint16_t const * src, Coord srcColumn, Coord srcStartRow, Coord numPixels, Coord srcWidth, Coord srcHeight, uint16_t * dst, uint32_t transparent) {
            ASSERT(numPixels * BPP % 16 == 0);
            // extremely inefficient implementation where we go pixel by pixel and treat the output buffer as a single column surface
            for (Coord y = srcStartRow, ye = srcStartRow + numPixels, i = 0; y < ye; ++y, ++i) {
                uint32_t c = pixelAt(srcColumn, y, srcWidth, srcHeight, src);
                if (c != transparent)
                    setPixelAt(0, i, 1, numPixels, dst, c);
            }
            return numPixels * BPP / 16;
        }

        static uint32_t blitRow(uint16_t const * src, Coord srcRow, Coord srcStartColumn, Coord numPixels, Coord srcWidth, Coord srcHeight, uint16_t * dst, uint32_t transparent) {
            ASSERT(numPixels * BPP % 16 == 0);
            // inefficient implementation where we go pixel by pixel and treat the output buffer as a single column surface (although rows are bound to be inefficient)
            for (Coord x = srcStartColumn, xe = srcStartColumn + numPixels, i = 0; x < xe; ++x, ++i) {
                uint32_t c = pixelAt(x, srcRow, srcWidth, srcHeight, src);
                if (c != transparent)
                    setPixelAt(0, i, 1, numPixels, dst, c);
            }
            return numPixels * BPP / 16;
        }


        /** Rendering functions
         
            Unlike blitting, rendering always copies the source surface to a 16bpp destination surface or buffers. So for 16bpp sources, rendering is identical to blitting and the palette must be ingnored, while for other bit depths, palette that translates from color indices to actual RGB565 colors must be provided.

         */
        static void render(uint16_t const * src, Coord srcWidth, Coord srcHeight, uint16_t * dst, Coord dstX, Coord dstY, Coord dstWidth, Coord dstHeight, uint16_t const * palette = nullptr) {
            if (BPP == 16) {
                ASSERT(palette == nullptr);
                blit(src, srcWidth, srcHeight, dst, dstX, dstY, dstWidth, dstHeight);
            } else {
                ASSERT(palette != nullptr);
                for (Coord x = srcWidth - 1; x < srcWidth; --x) 
                    for (Coord y = 0; y < srcHeight; ++y)
                        setPixelAt(x + dstX, y + dstY, dstWidth, dstHeight, dst, palette[pixelAt(x, y, srcWidth, srcHeight, src)]);
            }
        }

        static void renderRect(uint16_t const * src, Rect srcRect, Coord srcWidth, Coord srcHeight, uint16_t * dst, Coord dstX, Coord dstY, Coord dstWidth, Coord dstHeight, uint16_t const * palette = nullptr) {
            if (BPP == 16) {
                ASSERT(palette == nullptr);
                blitRect(src, srcRect, srcWidth, srcHeight, dst, dstX, dstY, dstWidth, dstHeight);
            } else {
                ASSERT(palette != nullptr);
                for (Coord x = srcRect.left(), xe = srcRect.right(); x < xe; ++x)
                    for (Coord y = srcRect.top(), ye = srcRect.bottom(); y < ye;++y)
                        setPixelAt(x + dstX, y + dstY, dstWidth, dstHeight, dst, palette[pixelAt(x, y, srcWidth, srcHeight, src)]);
            }
        }

        static uint32_t renderColumn(uint16_t const * src, Coord srcColumn, Coord srcStartRow, Coord numPixels, Coord srcWidth, Coord srcHeight, uint16_t * dst, uint16_t const * palette = nullptr) {
            if (BPP == 16) {
                ASSERT(palette == nullptr);
                return blitColumn(src, srcColumn, srcStartRow, numPixels, srcWidth, srcHeight, dst);
            } else {
                ASSERT(palette != nullptr);
                for (Coord y = srcStartRow, ye = srcStartRow + numPixels, i = 0; y < ye; ++y, ++i)
                    dst[i] = palette[pixelAt(srcColumn, y, srcWidth, srcHeight, src)];
                return numPixels;
            }
        }

        static uint32_t renderRow(uint16_t const * src, Coord srcRow, Coord srcStartColumn, Coord numPixels, Coord srcWidth, Coord srcHeight, uint16_t * dst, uint16_t const * palette = nullptr) {
            if (BPP == 16) {
                ASSERT(palette == nullptr);
                return blitRow(src, srcRow, srcStartColumn, numPixels, srcWidth, srcHeight, dst);
            } else {
                ASSERT(palette != nullptr);
                for (Coord x = srcStartColumn, xe = srcStartColumn + numPixels, i = 0; x < xe; ++x, ++i)
                    dst[i] = palette[pixelAt(x, srcRow, srcWidth, srcHeight, src)];
                return numPixels;
            }
        }

        /** Rendering with transparent color. 
         
            Similar to blitting with transparent color, rendering will ignore the transparent source color and leave the existing values in target surfaces or buffers. 
         */
        static void render(uint16_t const * src, Coord srcWidth, Coord srcHeight, uint16_t * dst, Coord dstX, Coord dstY, Coord dstWidth, Coord dstHeight, uint32_t transparent, uint16_t const * palette = nullptr) {
            if (BPP == 16) {
                ASSERT(palette == nullptr);
                blit(src, srcWidth, srcHeight, dst, dstX, dstY, dstWidth, dstHeight, transparent);
            } else {
                ASSERT(palette != nullptr);
                for (Coord x = srcWidth - 1; x < srcWidth; --x) 
                    for (Coord y = 0; y < srcHeight; ++y) {
                        uint32_t c = pixelAt(x, y, srcWidth, srcHeight, src);
                        if (c != transparent)
                            setPixelAt(x + dstX, y + dstY, dstWidth, dstHeight, dst, palette[c]);
                    }
            }
        }

        static void renderRect(uint16_t const * src, Rect srcRect, Coord srcWidth, Coord srcHeight, uint16_t * dst, Coord dstX, Coord dstY, Coord dstWidth, Coord dstHeight, uint32_t transparent, uint16_t const * palette = nullptr) {
            if (BPP == 16) {
                ASSERT(palette == nullptr);
                blitRect(src, srcRect, srcWidth, srcHeight, dst, dstX, dstY, dstWidth, dstHeight, transparent);
            } else {
                ASSERT(palette != nullptr);
                for (Coord x = srcRect.left(), xe = srcRect.right(); x < xe; ++x)
                    for (Coord y = srcRect.top(), ye = srcRect.bottom(); y < ye;++y) {
                        uint32_t c = pixelAt(x, y, srcWidth, srcHeight, src);
                        if (c != transparent)
                            setPixelAt(x + dstX, y + dstY, dstWidth, dstHeight, dst, palette[c]);
                    }
            }
        }

        static uint32_t renderColumn(uint16_t const * src, Coord srcColumn, Coord srcStartRow, Coord numPixels, Coord srcWidth, Coord srcHeight, uint16_t * dst, uint32_t transparent, uint16_t const * palette = nullptr) {
            if (BPP == 16) {
                ASSERT(palette == nullptr);
                return blitColumn(src, srcColumn, srcStartRow, numPixels, srcWidth, srcHeight, dst, transparent);
            } else {
                ASSERT(palette != nullptr);
                for (Coord y = srcStartRow, ye = srcStartRow + numPixels, i = 0; y < ye; ++y, ++i) {
                    uint32_t c = pixelAt(srcColumn, y, srcWidth, srcHeight, src);
                    if (c != transparent)
                        dst[i] = palette[c];
                }
                return numPixels;
            }
        }

        static uint32_t renderRow(uint16_t const * src, Coord srcRow, Coord srcStartColumn, Coord numPixels, Coord srcWidth, Coord srcHeight, uint16_t * dst, uint32_t transparent, uint16_t const * palette = nullptr) {
            if (BPP == 16) {
                ASSERT(palette == nullptr);
                return blitRow(src, srcRow, srcStartColumn, numPixels, srcWidth, srcHeight, dst, transparent);
            } else {
                ASSERT(palette != nullptr);
                for (Coord x = srcStartColumn, xe = srcStartColumn + numPixels, i = 0; x < xe; ++x, ++i) {
                    uint32_t c = pixelAt(x, srcRow, srcWidth, srcHeight, src);
                    if (c != transparent)
                        dst[i] = palette[c];
                }
                return numPixels;
            }
        }

        /** Drawing primitives. 
         */
        static constexpr Coord putChar(Coord x, Coord y, Coord width, Coord height, Font const & font, char c, uint16_t const * palette, uint16_t * pixels) {
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
                    if (a != 0 && xx >= 0 && xx < width && yy >= 0 && yy < height)
                        setPixelAt(xx, yy, width, height, pixels, palette[a]);
                    col = col << 2;
                    bits -= 2;
                }
            }
            return g.advanceX;
        }

    protected:

        /** Returns the offset of the column in half-word (uint16_t) array taking into account the bits per pixel. 
         */
        static uint32_t columnOffset(Coord column, Coord width, Coord height) {
            return (width - column - 1) * height * 8 / BPP;
        }

    }; // Surface

}
