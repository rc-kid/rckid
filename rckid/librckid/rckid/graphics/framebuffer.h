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
            ST7789::setUpdateRegion(Rect::XYWH(left_, top_, width(), height()));
            ST7789::beginDMAUpdate();
        }

        void disable() {
            ST7789::endDMAUpdate();
        }

        void render() {
            ST7789::waitVSync();
            ST7789::dmaUpdateAsync(buffer_, numPixels());
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
            ST7789::setUpdateRegion(width() * 2, height() * 2);
            ST7789::beginDMAUpdate();
        }

        void disable() {
            ST7789::endDMAUpdate();
        }

        void render() {
            updateLine_ = 0;
            ST7789::waitVSync();
            ST7789::dmaUpdateAsync(buffer_, height(), [this]() {
                if (updateLine_ == width())
                    return true;
                if (updateLine_ == width() -1) {
                    ++updateLine_;
                    ST7789::dmaUpdateAsync(buffer_ + height() * (width() - 1), height()); 
                } else {
                    ST7789::dmaUpdateAsync(buffer_ + height() * updateLine_++, height() * 2);
                }
                return false;
            });
        }

    private:

        int updateLine_ = 0;
    }; // FrameBuffer<Color565, DisplayMode::Native_2X_RGB565>

    template<>
    class FrameBuffer<ColorRGB_332, DisplayMode::Native_RGB565> : public Canvas<ColorRGB_332> {
    public:

        static constexpr int DEFAULT_WIDTH = 320;
        static constexpr int DEFAULT_HEIGHT = 240;

        FrameBuffer(int width = DEFAULT_WIDTH, int height = DEFAULT_HEIGHT): 
            Canvas{width, height},
            renderBuffer_{new ColorRGB[height * 2]} {
        }

        void enable() {
            ST7789::configure(DisplayMode::Native_RGB565);
            ST7789::setUpdateRegion(Rect::XYWH(left_, top_, width(), height()));
            ST7789::beginDMAUpdate();
        }

        void disable() { 
            ST7789::endDMAUpdate();
        }

        /** Renders the display using a 2 column buffer column by column so that while one columh is being rendered, the other column is being processed. 
         */
        void render() {
            //toRender_ = (uint16_t *)buffer_;
            // translate first two columns column
            toRender_ =  rckid_color256_to_rgb(
                (uint8_t const *)buffer_, 
                reinterpret_cast<uint16_t *>(renderBuffer_), 
                height() * 2, 
                Palette_332_to_565
            );
            column_ = 0;
            ST7789::waitVSync();
            ST7789::dmaUpdateAsync(renderBuffer_, height(), [this]() {
                if (++column_ == width())
                    return true;
                // write the already processed pixels
                ST7789::dmaUpdateAsync(
                    renderBuffer_ + ((column_ % 2 == 0) ? 0 : height()), 
                    height()
                );
                if (column_ + 1 < width())
                    toRender_ = rckid_color256_to_rgb(
                        toRender_,           
                        reinterpret_cast<uint16_t*>(renderBuffer_ + ((column_ % 2 == 1) ? 0 : height())), 
                        height(), 
                        Palette_332_to_565
                    );
                return false;
            });
        } 

    private:

        int top_ = 0;
        int left_ = 0;
        ColorRGB * renderBuffer_ = nullptr;
        uint8_t const * toRender_ = nullptr;
        int column_ = 0;

    }; // FrameBuffer<ColorRGB_332, DisplayMode::Native_RGB565> 

    template<>
    class FrameBuffer<ColorRGB_332, DisplayMode::Native_2X_RGB565> : public Canvas<ColorRGB_332> {
    public:

        static constexpr int DEFAULT_WIDTH = 160;
        static constexpr int DEFAULT_HEIGHT = 120;

        FrameBuffer(int width = DEFAULT_WIDTH, int height = DEFAULT_HEIGHT): 
            Canvas{width, height},
            renderBuffer_{new ColorRGB[height * 2] } {
        }

        void enable() {
            ST7789::configure(DisplayMode::Native_2X_RGB565);
            ST7789::setUpdateRegion(width() * 2, height() * 2);
            ST7789::beginDMAUpdate();
        }

        void disable() { 
            ST7789::endDMAUpdate();
        }

        /** Renders the display using a 2 column buffer column by column so that while one columh is being rendered, the other column is being processed. 
         */
        void render() {
            //toRender_ = buffer_;
            // translate one column
            toRender_ =  rckid_color256_to_rgb(
                (uint8_t const *)buffer_, 
                reinterpret_cast<uint16_t *>(renderBuffer_), 
                height() * 2, 
                Palette_332_to_565
            );
            column_ = 0;
            ST7789::waitVSync();
            ST7789::dmaUpdateAsync(renderBuffer_, height(), [this]() {
                if (++column_ == width())
                    return true;
                // write the already processed pixels
                ST7789::dmaUpdateAsync(
                    renderBuffer_ + ((column_ % 2 == 0) ? 0 : height()), 
                    height()
                );
                if (column_ + 1 < width())
                    toRender_ = rckid_color256_to_rgb(
                        toRender_,           
                        reinterpret_cast<uint16_t*>(renderBuffer_ + ((column_ % 2 == 1) ? 0 : height())), 
                        height(), 
                        Palette_332_to_565
                    );
                return false;
            });
        }

    private:

        ColorRGB * renderBuffer_ = nullptr;
        uint8_t const * toRender_ = nullptr;
        int column_ = 0;

    }; // FrameBuffer<ColorRGB_332, DisplayMode::Native_2X_RGB565> 



} // namespace rckid



