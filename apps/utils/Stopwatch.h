#pragma once

#include <rckid/app.h>
#include <rckid/graphics/canvas.h>
#include <rckid/assets/fonts/OpenDyslexic64.h>
#include <rckid/assets/fonts/OpenDyslexic128.h>

#include <rckid/ui/header.h>

namespace rckid {

    /** A simple stopwatch. 
     
        Pressing button A starts the stopwatch, pressing it again pauses it. Start button resets the stopwatch clock. 
     */
    class Stopwatch : public GraphicsApp<Canvas<ColorRGB>> {
    public:
        static void run() {
            Stopwatch app{};
            
            app.loop();
        }

    protected:

        Stopwatch(): GraphicsApp{Canvas<ColorRGB>{320, 240}} {}

        void update() override {
            GraphicsApp::update();
            if (btnPressed(Btn::A)) {
                if (running_) {
                    running_ = false;
                } else {
                    running_ = true;
                    lastTime_ = uptimeUs();
                }
            }
            if (btnPressed(Btn::Select)) {
                running_ = false;
                ms_ = 0;
                s_ = 0;
                m_ = 0;
                h_ = 0;
            }
        }

        void draw() override {
            if (running_) {
                uint32_t now = uptimeUs();
                uint32_t delta = (now - lastTime_ + 500) / 1000;
                lastTime_ = now;
                ms_ += delta;
                if (ms_ >=1000) {
                    ms_ -= 1000;
                    if (++s_ == 60) {
                        s_ = 0;
                        if (++m_ == 60) {
                            m_ = 0;
                            ++h_;
                        }
                    }
                }
            }
            g_.fill();
            std::string time{STR(h_ << ":" << m_ << ":" << s_)};
            std::string ms{STR("." << ms_)};
            int tWidth = assets::font::OpenDyslexic128::font.textWidth(time.c_str());
            int msWidth = assets::font::OpenDyslexic64::font.textWidth(ms.c_str());
            g_.text(160 - tWidth / 2, 30, assets::font::OpenDyslexic128::font, color::White) << time;
            g_.text(160 - msWidth / 2, 125, assets::font::OpenDyslexic64::font, color::White) << ms;

            Header::drawOn(g_);
        }

        int32_t ms_ = 0;
        int32_t s_ = 0;
        int32_t m_ = 0;
        int32_t h_ = 0;

        bool running_ = false;
        uint32_t lastTime_;

    }; // rckid::Stopwatch
} // namespace rckid
