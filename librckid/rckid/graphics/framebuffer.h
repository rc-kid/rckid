#pragma once

#include "canvas.h"
#include "ST7789.h"

namespace rckid {

    template<typename Color, DisplayMode DISPLAY_MODE = DisplayMode::Native_RGB565>
    class FrameBuffer;

    template<>
    class FrameBuffer<ColorRGB, DisplayMode::Native_RGB565> : public Canvas<ColorRGB> {
    public:

        static constexpr int DEFAULT_WIDTH = 320;
        static constexpr int DEFAULT_HEIGHT = 240; 

        FrameBuffer(int width = DEFAULT_WIDTH, int height = DEFAULT_HEIGHT): Canvas{width, height} {}

        void enable() {
            ST7789::configure(DisplayMode::Native_RGB565);
            ST7789::enterContinuousUpdate(Rect::XYWH(left_, top_, width(), height()));
        }

        void disable() {
            ST7789::leaveContinuousUpdate();
        }

        void render() {
            ST7789::waitVSync();
            ST7789::writePixels(reinterpret_cast<uint16_t const *>(buffer_), numPixels());
        }
    private:
        int top_ = 0;
        int left_ = 0;
    }; // FrameBuffer<ColorRGB, DisplayMode::Native_RGB565>

    template<>
    class FrameBuffer<ColorRGB, DisplayMode::Native_2X_RGB565> : public Canvas<ColorRGB>{
    public:
        static constexpr int DEFAULT_WIDTH = 160;
        static constexpr int DEFAULT_HEIGHT = 120;

        FrameBuffer(int width = DEFAULT_WIDTH, int height = DEFAULT_HEIGHT): Canvas{width, height} {}

        void enable() {
            ST7789::configure(DisplayMode::Native_2X_RGB565);
            ST7789::enterContinuousUpdate(width() * 2, height() * 2);
        }

        void disable() {
            ST7789::leaveContinuousUpdate();
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

        FrameBuffer(int width = DEFAULT_WIDTH, int height = DEFAULT_HEIGHT): 
            Canvas{width, height},
            renderBuffer1_{new uint32_t[height / 2] }, 
            renderBuffer2_{new uint32_t[height / 2] } {
        }

        void enable() {
            ST7789::configure(DisplayMode::Native_RGB565);
            ST7789::enterContinuousUpdate(Rect::XYWH(left_, top_, width(), height()));
        }

        void disable() { 
            ST7789::leaveContinuousUpdate();
        }

        /** Renders the display using a 2 column buffer column by column so that while one columh is being rendered, the other column is being processed. 
         */
        void render() {
            toRender_ = buffer_;
            // translate first column
            toRender_ = Color256::translatePixelBuffer(buffer_, renderBuffer1_, height());
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
                    toRender_ = Color256::translatePixelBuffer(toRender_, (column_ % 2 == 1) ? renderBuffer1_ : renderBuffer2_, height());
                return false;
            });
            // process the next colum
            toRender_ = Color256::translatePixelBuffer(toRender_, renderBuffer2_, height()); 
        }

    private:

        int top_ = 0;
        int left_ = 0;
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

        FrameBuffer(int width = DEFAULT_WIDTH, int height = DEFAULT_HEIGHT): 
            Canvas{width, height},
            renderBuffer1_{new uint32_t[height / 2] }, 
            renderBuffer2_{new uint32_t[height / 2] } {
        }

        void enable() {
            ST7789::configure(DisplayMode::Native_2X_RGB565);
            ST7789::enterContinuousUpdate(width() * 2, height() * 2);
        }

        void disable() { 
            ST7789::leaveContinuousUpdate();
        }

        /** Renders the display using a 2 column buffer column by column so that while one columh is being rendered, the other column is being processed. 
         */
        void render() {
            toRender_ = buffer_;
            // translate one column
            toRender_ = Color256::translatePixelBuffer(buffer_, renderBuffer1_, height());
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
                    toRender_ = Color256::translatePixelBuffer(toRender_, (column_ % 2 == 1) ? renderBuffer1_ : renderBuffer2_, height());
                return false;
            });
            // process the next colum
            toRender_ = Color256::translatePixelBuffer(toRender_, renderBuffer2_, height()); 
        }

    private:

        uint32_t * renderBuffer1_ = nullptr;
        uint32_t * renderBuffer2_ = nullptr;
        uint32_t const * toRender_ = nullptr;
        int column_ = 0;

    }; // FrameBuffer<Color256, DisplayMode::Native_2X_RGB565> 



} // namespace rckid



