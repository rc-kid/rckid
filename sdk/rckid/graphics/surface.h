#pragma once

#include "../rckid.h"
#include "image_decoder.h"

namespace rckid {

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
            uint32_t offset = pixelOffset(x, y, width, height);
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
            uint32_t offset = pixelOffset(x, y, width, height);
            switch (BPP) {
                case 16: 
                    pixels_[offset] = color;
                    return;
                case 8:
                    reinterpret_cast<uint8_t*>(pixels_)[offset] = color;
                    return;
                case 4: {
                    uint8_t & x = reinterpret_cast<uint8_t*>(pixels_)[offset / 2];
                    x = x & (offset & 1) ? 0x0f : 0xf0;
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
                return numPixels * BPP / 16;
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
                return numPixels * BPP / 16;
            }
        }

    protected:

        /** Returns the pixel offset for pixel at coordinates (x,y) in a pixel buffer of specified width and height. Assumes the native display orientation, i.e. right-top corner is index 0, column-first format. 
         */
        static uint32_t pixelOffset(Coord x, Coord y, Coord width, Coord height) {
            return (width - x - 1) * height + y;
        }

        /** Returns the offset of the column in half-word (uint16_t) array taking into account the bits per pixel. 
         */
        static uint32_t columnOffset(Coord column, Coord width, Coord height) {
            return (width - column - 1) * height * 8 / BPP;
        }

    }; // Surface


    // renderable bitmap is different

    #ifdef HAHA

    /** Single tile. 
     
        Tile is a surface with statically known width and height. It supports the basic  
     */
    template<Coord WIDTH, Coord HEIGHT, typename PIXEL>
    class Tile : protected Surface<PIXEL::BPP> {
    public:
        using Pixel = PIXEL;
        static constexpr uint32_t BPP = PIXEL::BPP;

        Coord width() const { return WIDTH; }
        Coord height() const { return HEIGHT; }


        // palette is not nullptr as we expect most tiles to use palette indices instead of RGB colors
        uint32_t renderColumn(Coord column, Coord startRow, Coord numPixels, uint16_t * buffer, uint16_t const * palette) const {
            return renderColumn(pixels_, column, startRow, numPixels, WIDTH, HEIGHT, buffer, palette);
        }

    private:
        using Surface<BPP>::numHalfWords;
        using Surface<BPP>::renderColumn;

        uint16_t pixels_[numHalfWords(WIDTH, HEIGHT)];
    }; 

    #endif

}
