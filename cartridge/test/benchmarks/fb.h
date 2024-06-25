#pragma once

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "fonts/Iosevka_Mono6pt7b.h"
#include "rckid/assets.h"

#define BENCHMARK(...) CALCULATE_TIME( for (int i = 0; i < NUM_ITERATIONS; ++i) { __VA_ARGS__ }) / NUM_ITERATIONS

namespace rckid {

    /** Framebuffer benchmark test. 
     
        Default: fill 11699
                 blit 10646

        Updated: fill 1230 x8
                      925 x16

        With fullscreen special : 712

     */
    class FB : public App<FrameBuffer<ColorRGB>> {
    public:

        static constexpr int NUM_ITERATIONS = 100;

        FB()  = default;

    protected:

        void onFocus() override {
            App::onFocus();
            setBrightness(32);
            mode_ = Mode::Fill;
            driver_.setFg(Color::White());
            driver_.setBg(Color::Black());
            driver_.setFont(Iosevka_Mono6pt7b);
        }

        void update() override {
        }

        void draw() override /*__attribute__ ((optimize(3))) */ {
            switch (mode_) {
                case Mode::Fill: {
                    fill_ = BENCHMARK(
                        driver_.fill(Rect::WH(320, 240));
                    );
                    mode_ = Mode::Blit;
                    break;
                }
                case Mode::Blit: {
                    Bitmap<Color> bmp = Bitmap<Color>{64, 64, MemArea::Heap};
                    bmp.loadImage(assets::Gameboy, sizeof(assets::Gameboy));
                    blit_ = BENCHMARK(
                        for (int j = 0; j < 10; j += 2)
                            driver_.draw(Point{(i + j) % 250, (i + j) % 170}, bmp);
                    );
                    mode_ = Mode::Done;
                    break;
                }
                default: 
                    break;
            }
            driver_.fill(Color::Blue());
            driver_.setPixelAt(0,0, Color::Red());
            driver_.setPixelAt(319,0, Color::Red());
            driver_.setPixelAt(0,239, Color::Red());
            driver_.setPixelAt(319,239, Color::Red());
            auto wr = driver_.textMultiline(5, 5);
            wr << "fill: " << fill_ << "\n";
            if (mode_ == Mode::Blit)
                return;
            wr << "blit: " << blit_ << "\n";
        }

    private:
        enum class Mode {
            Fill,
            Blit,
            Done,
        }; // FBFill::Mode

        Mode mode_;

        size_t fill_ = 0;
        size_t blit_ = 125;

    }; 
}