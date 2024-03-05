#pragma once

#include "canvas.h"

#include "../ST7789.h"
#include "../app.h"

namespace rckid {

    /** A framebuffer is a canvas that knows how to send itself to the display. 
     */
    template<typename COLOR>
    class FrameBuffer;

    /** Framebufer for native 16bit colors. This is the fastest and simplest framebuffer that does however consume the largest amount of memory. Entire framebuffer is updated in a single DMA transfer. 
     */
    template<>
    class FrameBuffer<ColorRGB> : public Canvas<ColorRGB> {
    public:

        static constexpr int DEFAULT_WIDTH = 320;
        static constexpr int DEFAULT_HEIGHT = 240;

        FrameBuffer(int width = DEFAULT_WIDTH, int height = DEFAULT_HEIGHT): Canvas{width, height} {}
        FrameBuffer(Bitmap<ColorRGB> && bitmap) : Canvas{std::move(bitmap)} {}

        void initialize() {
            setBuffer(Bitmap<ColorRGB>::inVRAM(width(), height()));
            ST7789::enterContinuousMode(ST7789::Mode::Single);
        }

        void render() {
            ST7789::waitVSync();
            ST7789::writePixels(reinterpret_cast<uint16_t const *>(buffer_), numPixels());
        }

    }; // rckid::FrameBuffer<ColorRGB>

    /** Framebuffer for 8bit colors. 
     */
    template<>
    class FrameBuffer<Color256> : public Canvas<Color256> {
    public:

        static constexpr int DEFAULT_WIDTH = 320;
        static constexpr int DEFAULT_HEIGHT = 240;

        FrameBuffer(int width = DEFAULT_WIDTH, int height = DEFAULT_HEIGHT): Canvas{width, height} {}

        void initialize() {
            setBuffer(Bitmap<Color256>::inVRAM(width(), height()));
            ST7789::enterContinuousMode(ST7789::Mode::Single);
            renderBuffer_ = reinterpret_cast<uint32_t*>(allocateVRAM(height() * 4)); // 2 columns at a time
        }

        // TODO this is very slow rendering because we only prepare data *after* the DMA finishes, if we use two buffers, we can alternate between them and prepare next buffer while the first buffer is sent to the display
        void render() {
            toRender_ = buffer_;
            toRender_ = translatePixels(buffer_, renderBuffer_, height() * 2); 
            ST7789::waitVSync();
            ST7789::writePixels(reinterpret_cast<uint16_t const *>(renderBuffer_), height() * 2, [this](){
                if (toRender_ >= buffer_ + numPixels() / 4) {
                    ST7789::writePixelsDone();
                } else {
                    toRender_ = translatePixels(toRender_, renderBuffer_, height() * 2);
                    ST7789::writePixels(reinterpret_cast<uint16_t const *>(renderBuffer_), height() * 2);
                }
            });
        }

    private:

        uint32_t const * translatePixels(uint32_t const * src, uint32_t * dest, size_t numPixels) {
            uint32_t x;
            uint32_t y[2];
            for (size_t i = 0; i < numPixels; i += 4) { // 2 columns at a time
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

        uint32_t * renderBuffer_ = nullptr;
        uint32_t const * toRender_ = nullptr;
    }; // rckid::FrameBuffer<Color256>

    /** Double pixel size framebuffer. 
     */
    template<typename COLOR>
    class FrameBufferDouble;

    template<>
    class FrameBufferDouble<ColorRGB> : public Canvas<ColorRGB> {
    public:

        static constexpr int DEFAULT_WIDTH = 160;
        static constexpr int DEFAULT_HEIGHT = 120;

        FrameBufferDouble(int width = DEFAULT_WIDTH, int height = DEFAULT_HEIGHT): Canvas{width, height} {}

        void initialize() {
            setBuffer(Bitmap<ColorRGB>::inVRAM(width(), height()));
            ST7789::enterContinuousMode(ST7789::Mode::Double);
        }

        void render() {
            updateLine_ = 0;
            ST7789::waitVSync();
            ST7789::writePixels(reinterpret_cast<uint16_t const *>(buffer_), height(), [this]() {
                if (updateLine_ == width() -1) {
                    ST7789::writePixels(reinterpret_cast<uint16_t const *>(buffer_) + height() * (width() - 1), height(), nullptr);
                } else {
                    ST7789::writePixels(reinterpret_cast<uint16_t const *>(buffer_) + height() * updateLine_++, height() * 2);
                }
            });
        }

    private:

        int updateLine_ = 0;

    }; // rckid::FrameBuffer<ColorRGB>



    /** Base class for applications that render into a framebuffer. */
    template<typename FB> 
    class FBApp : public App2 {
    public:

        using Color = typename FB::Color;
        
        FBApp(int w = FB::DEFAULT_WIDTH, int h = FB::DEFAULT_HEIGHT):
            fb_{w, h} {
        }

    protected:

        void onFocus() override {
            fb_.initialize();
        }

        void onBlur() override {
            // nothing to do, the FB allocates on the VRAM and will be invalidated automatically
        }

        void render() override {
#ifdef RCKID_DEBUG_FPS
            GFXfont const & f = font();
            setFont(Iosevka_Mono6pt7b);
            ColorRGB c = fg();
            setFg(ColorRGB::White());
            text(0, 220) << Stats::fps() << " d: " << Stats::drawUs() << " mem: " << Stats::freeHeap();
            setFont(f);
            setFg(c);
#endif
            fb_.render();
        }

        FB fb_;

    }; 

} // namespace rckid



