#pragma once

#include "canvas.h"

#include "../ST7789.h"
#include "../app.h"

namespace rckid {

    /** A framebuffer is nothing more than a canvas that knows how to display itself on the display. 
     */
    class FrameBuffer : public Canvas {
    public:

        static constexpr unsigned RENDERER_ID = 1;

        FrameBuffer(): Canvas{320, 240} {
            ST7789::enterContinuousMode(ST7789::Mode::Single);
        }

        void startRendering() {
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
    }; // rckid::FrameBuffer

    /** Double framebuffer that uses 1/4 of the memory and doubles each pixel. Inspired by the picosystem for a pixelated look. 
     */
    class FrameBufferDouble : public Canvas {
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