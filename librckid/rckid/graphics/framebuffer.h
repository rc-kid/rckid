#pragma once

#include "canvas.h"

#include "../ST7789.h"
#include "../app.h"

namespace rckid {

    template<typename COLOR>
    class FrameBuffer : public Canvas<COLOR> {
    public:
        static unsigned const RENDERER_ID;

        FrameBuffer(): Canvas<COLOR>{320, 240} {
            ST7789::enterContinuousMode(ST7789::Mode::Single);
        }

        void startRendering();
    }; 

    template<>
    inline unsigned const FrameBuffer<ColorRGB>::RENDERER_ID = 0;

    template<> 
    inline void FrameBuffer<ColorRGB>::startRendering() {
#ifdef RCKID_DEBUG_FPS
        GFXfont const & f = font();
        setFont(Iosevka_Mono6pt7b);
        ColorRGB c = fg();
        setFg(ColorRGB::White());
        text(0, 220) << BaseApp::fps() << " d: " << BaseApp::drawUs() << " mem: " << freeHeap();
        setFont(f);
        setFg(c);
#endif
        ST7789::waitVSync();
        ST7789::updatePixels(reinterpret_cast<uint16_t const *>(buffer_), numPixels());
    } 

    template<>
    class FrameBuffer<Color256> : public Canvas<Color256> {
    public:
        static inline unsigned const RENDERER_ID = 1;

        FrameBuffer(): Canvas{320, 240} {
            ST7789::enterContinuousMode(ST7789::Mode::Single);
            gpio_init(RP_PIN_GPIO_16);
            gpio_set_dir(RP_PIN_GPIO_16, GPIO_OUT);
            gpio_init(RP_PIN_GPIO_17);
            gpio_set_dir(RP_PIN_GPIO_17, GPIO_OUT);
            gpio_put(RP_PIN_GPIO_16, false);
            gpio_put(RP_PIN_GPIO_17, false);
        }

        void startRendering() {
    #ifdef RCKID_DEBUG_FPS
            GFXfont const & f = font();
            setFont(Iosevka_Mono6pt7b);
            Color c = fg();
            setFg(Color::White());
            text(0, 220) << BaseApp::fps() << " d: " << BaseApp::drawUs() << " mem: " << freeHeap();
            setFont(f);
            setFg(c);
    #endif
            /// TODO: This is extremely slow, we should process the line, start the transfer and process the second line while we are transferring still. Problems could be that the ISR will be too long thanks to the processing, can be made into an event? 
            /// The static column is also not very good way of representing stuff
            /// the color to RGB can be made faster with a palette lookup from index to color
            size_t updateCol = 319;
            colCurr_ = column_;
            colNext_ = column_ + 240;
            gpio_put(RP_PIN_GPIO_16, true);
            for (size_t i = 0; i < 240; ++i) {
                colCurr_[i] = pixelAt(updateCol, i).toRGB().rawValue16();
                colNext_[i] = pixelAt(updateCol - 1, i).toRGB().rawValue16();
            }
            gpio_put(RP_PIN_GPIO_16, false);
            updateCol -= 1;

            ST7789::waitVSync();
            gpio_put(RP_PIN_GPIO_17, true);
            ST7789::updatePixelsPartial(colCurr_, height(), [this, updateCol]() mutable {
                gpio_put(RP_PIN_GPIO_17, false);
                std::swap(colCurr_, colNext_);
                if (updateCol-- == 0) {
                    ST7789::updatePixels(colCurr_, height());
                } else {
                    gpio_put(RP_PIN_GPIO_17, true);
                    ST7789::updatePixelsPartial(colCurr_, height());
                    gpio_put(RP_PIN_GPIO_16, true);
                    for (size_t i = 0; i < 240; ++i)
                        colNext_[i] = pixelAt(updateCol, i).toRGB().rawValue16();
                    gpio_put(RP_PIN_GPIO_16, false);
                }
            });
        }

    private:
        uint16_t column_[240 * 2];
        uint16_t * colCurr_;
        uint16_t * colNext_;

    }; // FrameBuffer<Color256>

/*

    template<>
    inline unsigned const FrameBuffer<Color256>::RENDERER_ID = 1;

    template<>
    inline void FrameBuffer<Color256>::startRendering() {
#ifdef RCKID_DEBUG_FPS
        GFXfont const & f = font();
        setFont(Iosevka_Mono6pt7b);
        Color c = fg();
        setFg(Color::White());
        text(0, 220) << BaseApp::fps() << " d: " << BaseApp::drawUs() << " mem: " << freeHeap();
        setFont(f);
        setFg(c);
#endif
        /// TODO: This is extremely slow, we should process the line, start the transfer and process the second line while we are transferring still. Problems could be that the ISR will be too long thanks to the processing, can be made into an event? 
        /// The static column is also not very good way of representing stuff
        /// the color to RGB can be made faster with a palette lookup from index to color
        static uint16_t column[240];
        size_t updateCol = 319;
        ST7789::waitVSync();
        ST7789::updatePixelsPartial(reinterpret_cast<uint16_t const *>(column), height(), [this, updateCol]() mutable {
            for (size_t i = 0; i < 240; ++i)
                column[i] = pixelAt(updateCol, i).toRGB().rawValue16();
            if (--updateCol == 0)
                ST7789::updatePixels(column, height());
            else
                ST7789::updatePixelsPartial(column, height());
        });
    }

*/
    /** Double framebuffer that uses 1/4 of the memory and doubles each pixel. Inspired by the picosystem for a pixelated look. 
     */
    class FrameBufferDouble : public Canvas<ColorRGB> {
    public:    

        static constexpr unsigned RENDERER_ID = 2;

        FrameBufferDouble(): Canvas{160, 120} {
            ST7789::enterContinuousMode(ST7789::Mode::Double);
        }

        void startRendering() {
            updateLine_ = 0;
            ST7789::waitVSync();
            ST7789::updatePixelsPartial(reinterpret_cast<uint16_t const *>(buffer_), height(), [this](){
                if (updateLine_ == width() - 1)
                    ST7789::updatePixels(reinterpret_cast<uint16_t const *>(buffer_) + height() * (width() - 1), height());
                else 
                ST7789::updatePixelsPartial(reinterpret_cast<uint16_t const *>(buffer_) + height() * updateLine_++, height() * 2);
            }); 
        }

    private:

        uint16_t updateLine_;

    }; // rckid::FrameBufferDouble

} // namespace rckid