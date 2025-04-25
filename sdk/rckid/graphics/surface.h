#pragma once

#include "../rckid.h"

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
         
            This is excruciatingly slow interface that provides pixel granularity access to the surface. 
         */
        static constexpr uint32_t pixelAt(uint16_t const * pixels_, Coord x, Coord y, Coord width, Coord height) {
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

        static constexpr void setPixelAt(uint16_t * pixels_, Coord x, Coord y, Coord width, Coord height, uint32_t color) {
            uint32_t offset = pixelOffset(x, y, width, height);
            switch (BPP) {
                case 16: 
                    pixels_[offset] = color;
                    return;
                case 8:
                    reinterpret_cast<uint8_t const *>(pixels_)[offset] = color;
                    return;
                case 4: {
                    uint8_t & x = reinterpret_cast<uint8_t const *>(pixels_)[offset / 2];
                    x = x & (offset & 1) ? 0x0f : 0xf0;
                    x |= color << ((offset & 1) * 4);
                    return;
                }
                case 2: {
                    uint8_t & x = reinterpret_cast<uint8_t const *>(pixels_)[offset / 4];
                    x = x & ~ (0x03 << ((offset & 4) * 2));
                    x |= color << ((offset & 4) * 2);
                    return;
                }
                case 1: {
                    uint8_t & x = reinterpret_cast<uint8_t const *>(pixels_)[offset / 8];
                    x = x & ~(0x01 << (offset & 7));
                    x |= color << (offset & 7);
                    return;
                }
            }
        }

        // blitting

        static void blit(uint16_t const * src, Coord srcWidth, Coord srcHeight, uint16_t * dst, Coord dstX, Coord dstY, Coord dstWidth, Coord dstHeight) {

        }

        static void blitRect(uint16_t const * src, Rect srcRect, Coord srcWidth, Coord srcHeight, uint16_t * dst, Coord dstX, Coord dstY, Coord dstWidth, Coord dstHeight) {
            // TODO implement
        }

        static uint32_t blitColumn(uint16_t const * src, Coord srcColumn, Coord srcStartRow, Coord srcEndRow, Coord srcWidth, Coord srcHeight, uint16_t * dst) {

        }

        static uint32_t blitRow(uint16_t const * src, Coord srcRow, Coord srcStartColumn, Coord srcEndColumn, Coord srcWidth, Coord srcHeight, uint16_t * dst) {

        }

        // rendering 

        static void render(uint16_t const * src, Coord srcWidth, Coord srcHeight, uint16_t * dst, Coord dstX, Coord dstY, Coord dstWidth, Coord dstHeight, uint16_t const * palette = nullptr) {
            if (BPP == 16)
                ASSERT(palette == nullptr);
            else 
                ASSERT(palette != nullptr);

        }

        static void renderRect(uint16_t const * src, Rect srcRect, Coord srcWidth, Coord srcHeight, uint16_t * dst, Coord dstX, Coord dstY, Coord dstWidth, Coord dstHeight, uint16_t const * palette = nullptr) {
            if (BPP == 16)
                ASSERT(palette == nullptr);
            else 
                ASSERT(palette != nullptr);
        }

        static uint32_t renderColumn(uint16_t const * src, Coord srcColumn, Coord srcStartRow, Coord srcEndRow, Coord srcWidth, Coord srcHeight, uint16_t * dst, uint16_t const * palette = nullptr) {
            if (BPP == 16)
                ASSERT(palette == nullptr);
            else 
                ASSERT(palette != nullptr);

        }

        static uint32_t renderRow(uint16_t const * src, Coord srcRow, Coord srcStartColumn, Coord srcEndColumn, Coord srcWidth, Coord srcHeight, uint16_t * dst, uint16_t const * palette = nullptr) {
            if (BPP == 16)
                ASSERT(palette == nullptr);
            else 
                ASSERT(palette != nullptr);

        }


    private:

        /** Returns the pixel offset for pixel at coordinates (x,y) in a pixel buffer of specified width and height. Assumes the native display orientation, i.e. right-top corner is index 0, column-first format. 
         */
        static uint32_t pixelOffset(Coord x, Coord y, Coord width, Coord height) {
            return (width - x - 1) * height + y;
        }

    }; // Surface




}