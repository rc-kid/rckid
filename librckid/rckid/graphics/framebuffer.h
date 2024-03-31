#pragma once

#include "canvas.h"

#include "../ST7789.h"
#include "../app.h"

namespace rckid {

    template<typename Color, DisplayMode DISPLAY_MODE = DisplayMode::Native_RGB565>
    class FrameBuffer;

    template<>
    class FrameBuffer<ColorRGB, DisplayMode::Native_RGB565> : public Canvas<ColorRGB> {
    public:

        static constexpr int DEFAULT_WIDTH = 320;
        static constexpr int DEFAULT_HEIGHT = 240; 

        FrameBuffer(int width = DEFAULT_WIDTH, int height = DEFAULT_HEIGHT): Canvas{width, height} {}
        FrameBuffer(Bitmap<ColorRGB> && bitmap): Canvas{std::move(bitmap)} {}

        void enable() {
            setBuffer(Bitmap<ColorRGB>::inVRAM(width(), height()));
            ST7789::configure(DisplayMode::Native_RGB565);
            ST7789::enterContinuousUpdate(width(), height());
        }

        void disable() { }

        void render() {
            ST7789::waitVSync();
            ST7789::writePixels(reinterpret_cast<uint16_t const *>(buffer_), numPixels());
        }
    }; // FrameBuffer<ColorRGB, DisplayMode::Native_RGB565>

    template<>
    class FrameBuffer<ColorRGB, DisplayMode::Native_2X_RGB565> : public Canvas<ColorRGB>{
    public:
        static constexpr int DEFAULT_WIDTH = 160;
        static constexpr int DEFAULT_HEIGHT = 120;

        FrameBuffer(int width = DEFAULT_WIDTH, int height = DEFAULT_HEIGHT): Canvas{width, height} {}
        FrameBuffer(Bitmap<ColorRGB> && bitmap): Canvas{std::move(bitmap)} {}

        void enable() {
            setBuffer(Bitmap<ColorRGB>::inVRAM(width(), height()));
            ST7789::configure(DisplayMode::Native_2X_RGB565);
            ST7789::enterContinuousUpdate(width() * 2, height() * 2);
        }

        void render() {
            updateLine_ = 0;
            ST7789::waitVSync();
            ST7789::writePixels(reinterpret_cast<uint16_t const *>(buffer_), height(), [this]() {
                if (updateLine_ == width())
                    return true;
                if (updateLine_ == width() -1) {
                    ++updateLine_;
                    ST7789::writePixels(reinterpret_cast<uint16_t const *>(buffer_) + height() * (width() - 1), height()); 
                } else {
                    ST7789::writePixels(reinterpret_cast<uint16_t const *>(buffer_) + height() * updateLine_++, height() * 2);
                }
                return false;
            });
        }

    private:

        int updateLine_ = 0;
    }; // FrameBuffer<Color565, DisplayMode::Native_2X_RGB565>





    template<>
    class FrameBuffer<Color256, DisplayMode::Native_RGB565> : public Canvas<Color256> {
    public:

        static constexpr int DEFAULT_WIDTH = 320;
        static constexpr int DEFAULT_HEIGHT = 240;

        FrameBuffer(int width = DEFAULT_WIDTH, int height = DEFAULT_HEIGHT): Canvas{width, height} {}
        FrameBuffer(Bitmap<Color256> && bitmap): Canvas{std::move(bitmap)} {}

        void enable() {
            setBuffer(Bitmap<Color256>::inVRAM(width(), height()));
            ST7789::configure(DisplayMode::Native_RGB565);
            ST7789::enterContinuousUpdate(width(), height());
            renderBuffer1_ = reinterpret_cast<uint32_t*>(allocateVRAM(height() * 2)); // 
            renderBuffer2_ = reinterpret_cast<uint32_t*>(allocateVRAM(height() * 2)); // 
        }

        void disable() { }

        /** Renders the display using a 2 column buffer column by column so that while one columh is being rendered, the other column is being processed. 
         */
        void render() {
            toRender_ = buffer_;
            // translate one column
            toRender_ = translatePixels(buffer_, renderBuffer1_, height());
            column_ = 0;
            ST7789::waitVSync();
            ST7789::writePixels(reinterpret_cast<uint16_t const*>(renderBuffer1_), height(), [this]() {
                if (++column_ == width())
                    return true;
                // write the already processed pixels
                ST7789::writePixels(
                    reinterpret_cast<uint16_t const *>((column_ % 2 == 0) ? renderBuffer1_ : renderBuffer2_),
                    height()
                );
                if (column_ + 1 < width())
                    toRender_ = translatePixels(toRender_, (column_ % 2 == 1) ? renderBuffer1_ : renderBuffer2_, height());
                return false;
            });
            // process the next colum
            toRender_ = translatePixels(toRender_, renderBuffer2_, height()); 
        }

    private:

        static uint32_t const * translatePixels(uint32_t const * src, uint32_t * dest, size_t numPixels) {
            uint32_t x;
            uint32_t y[2];
            for (size_t i = 0; i < numPixels; i += 4) { // 4 pixels at a time
                x = *src++;
                y[0] = Color256::palette[x >> 24].rawValue16() << 16;
                y[0] |= Color256::palette[(x >> 16) & 0xff].rawValue16();
                y[1] = Color256::palette[(x >> 8) & 0xff].rawValue16() << 16;
                y[1] |= Color256::palette[x & 0xff].rawValue16();
                *dest++ = y[1];
                *dest++ = y[0];
            }
            return src;
        }

        uint32_t * renderBuffer1_ = nullptr;
        uint32_t * renderBuffer2_ = nullptr;
        uint32_t const * toRender_ = nullptr;
        int column_ = 0;

    }; // FrameBuffer<Color256, DisplayMode::Native_RGB565> 


    template<>
    class FrameBuffer<Color256, DisplayMode::Native_2X_RGB565> : public Canvas<Color256> {
    public:

        static constexpr int DEFAULT_WIDTH = 160;
        static constexpr int DEFAULT_HEIGHT = 120;

        FrameBuffer(int width = DEFAULT_WIDTH, int height = DEFAULT_HEIGHT): Canvas{width, height} {}
        FrameBuffer(Bitmap<Color256> && bitmap): Canvas{std::move(bitmap)} {}

        void enable() {
            setBuffer(Bitmap<Color256>::inVRAM(width(), height()));
            ST7789::configure(DisplayMode::Native_2X_RGB565);
            ST7789::enterContinuousUpdate(width() * 2, height() * 2);
            renderBuffer1_ = reinterpret_cast<uint32_t*>(allocateVRAM(height() * 2)); // 
            renderBuffer2_ = reinterpret_cast<uint32_t*>(allocateVRAM(height() * 2)); // 
        }

        void disable() { }

        /** Renders the display using a 2 column buffer column by column so that while one columh is being rendered, the other column is being processed. 
         */
        void render() {
            toRender_ = buffer_;
            // translate one column
            toRender_ = translatePixels(buffer_, renderBuffer1_, height());
            column_ = 0;
            ST7789::waitVSync();
            ST7789::writePixels(reinterpret_cast<uint16_t const*>(renderBuffer1_), height(), [this]() {
                if (++column_ == width())
                    return true;
                // write the already processed pixels
                ST7789::writePixels(
                    reinterpret_cast<uint16_t const *>((column_ % 2 == 0) ? renderBuffer1_ : renderBuffer2_),
                    height()
                );
                if (column_ + 1 < width())
                    toRender_ = translatePixels(toRender_, (column_ % 2 == 1) ? renderBuffer1_ : renderBuffer2_, height());
                return false;
            });
            // process the next colum
            toRender_ = translatePixels(toRender_, renderBuffer2_, height()); 
        }

    private:

        static uint32_t const * translatePixels(uint32_t const * src, uint32_t * dest, size_t numPixels) {
            uint32_t x;
            uint32_t y[2];
            for (size_t i = 0; i < numPixels; i += 4) { // 4 pixels at a time
                x = *src++;
                y[0] = Color256::palette[x >> 24].rawValue16() << 16;
                y[0] |= Color256::palette[(x >> 16) & 0xff].rawValue16();
                y[1] = Color256::palette[(x >> 8) & 0xff].rawValue16() << 16;
                y[1] |= Color256::palette[x & 0xff].rawValue16();
                *dest++ = y[1];
                *dest++ = y[0];
            }
            return src;
        }

        uint32_t * renderBuffer1_ = nullptr;
        uint32_t * renderBuffer2_ = nullptr;
        uint32_t const * toRender_ = nullptr;
        int column_ = 0;

    }; // FrameBuffer<Color256, DisplayMode::Native_RGB565> 



} // namespace rckid



