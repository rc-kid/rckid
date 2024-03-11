#pragma once

#include "rckid/app.h"
#include "rckid/audio.h"
#include "rckid/graphics/framebuffer.h"
#include "fonts/Iosevka_Mono6pt7b.h"


namespace rckid {

    /**  
     */
    class FBFill : public FBApp<FrameBuffer<ColorRGB>> {
    public:

        FBFill()  = default;

    protected:

        void onFocus() override {
            FBApp::onFocus();
            setBrightness(32);
            mode_ = Mode::Start;
            fb_.setFg(Color::White());
            fb_.setBg(Color::Black());
            fb_.setFont(Iosevka_Mono6pt7b);
        }

        void update() override {
            if (pressed(Btn::A))
                setBrightness(254);
            if (pressed(Btn::B)) {
               setBrightness(32);
               FATAL_ERROR(45);
            }
        }

        void draw() override __attribute__ ((optimize(3))) {
            switch (mode_) {
                case Mode::Start:
                    break;
                case Mode::Full2: {
                    full2_ = CALCULATE_TIME(
                        Color * c = fb_.rawBuffer();
                        Color tgt = Color::RGB(0,0,0);
                        for (int i = 0; i < 100; ++i) {
                            tgt.setB(i);
                            for (size_t j = 0, je = fb_.numPixels(); j < je; ++j)
                                c[j] = tgt;
                        }
                    );
                    full2_ /= 100;
                    break;
                }
                case Mode::Full4:
                    full4_ = CALCULATE_TIME(
                        uint32_t * c = assumeAligned<uint32_t*>(fb_.rawBuffer());
                        uint32_t tgt = 0;
                        for (int i = 0; i < 100; ++i) {
                            tgt = (i << 8) | (i << 24);
                            for (size_t j = 0, je = fb_.numPixels() / 2; j < je; ++j)
                                c[j] = tgt;
                        }
                    );
                    full4_ /= 100;
                    break;
                case Mode::Full4Unroll:
                    full4Unroll_ = CALCULATE_TIME(
                        uint32_t * c = assumeAligned<uint32_t*>(fb_.rawBuffer());
                        uint32_t tgt = 0;
                        for (int i = 0; i < 100; ++i) {
                            tgt = (i << 8) | (i << 24);
                            for (size_t j = 0, je = fb_.numPixels() / 8; j < je;) {
                                c[j++] = tgt;
                                c[j++] = tgt;
                                c[j++] = tgt;
                                c[j++] = tgt;
                            }
                        }
                    );
                    full4Unroll_ /= 100;
                    break;
                    
                case Mode::Results:
                    fb_.fill();
                    fb_.textMultiline(0,0)
                        << "full2 " << full2_ << '\n'
                        << "full4 " << full4_ << "\n"
                        << "full4Unroll " << full4Unroll_ << "\n"
                        ;
                    return;

            }
            mode_ = static_cast<Mode>(static_cast<unsigned>(mode_) + 1);
            fb_.fill();
            fb_.textMultiline(10,100) 
                << "Running benchmark " << static_cast<unsigned>(mode_) << "/" << (static_cast<unsigned>(Mode::Results) - 1);
        }

    private:
        enum class Mode {
            Start,
            Full2,
            Full4,
            Full4Unroll,
            Results,
        }; // FBFill::Mode

        Mode mode_;

        size_t full2_ = 0;
        size_t full4_ = 0;
        size_t full4Unroll_ = 0;

    }; 
}