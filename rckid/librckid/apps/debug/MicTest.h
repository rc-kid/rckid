#pragma once 


#include <limits>

//#include <hardware/pwm.h>

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/audio/audio.h"

namespace rckid {

    class MicTest: public App<FrameBuffer<ColorRGB>>, public audio::OutStream {
    public:

        static MicTest * create() { return new MicTest{}; }

        MicTest():
            buffer_{new uint8_t[64000]} {
        }

        void fillBuffer(uint16_t * buffer, size_t bufferSize) override {
            for (size_t i = 0; i < bufferSize; i += 4) {
                uint16_t x = buffer_[n_++];
                x = x << 4;
                buffer[i] = x;
                buffer[i + 1] = x;
                buffer[i + 2] = x;
                buffer[i + 3] = x;
                if (n_ == 64000)
                    n_ = 0;
            }
        }

        uint16_t sampleRate() override { return 32000; }

    protected:

        void onFocus() override {
            App::onFocus();
        }

        void onBlur() override {
            App::onBlur();
            audio::stop();
        }

        void update() override {
            App::update();
            if (pressed(Btn::A)) {
                min_ = 65535;
                max_ = 0;
                n_ = 0;
                audio::record([this](uint16_t const * buffer, size_t size) {
                    while (size-- > 0) {
                        if (n_ == 64000) {
                            audio::stop();
                            break;
                        } 
                        if (*buffer > max_)
                            max_ = *buffer;
                        if (*buffer < min_)
                            min_ = *buffer;
                        buffer_[n_++] = static_cast<uint8_t>(*buffer++);
                    }
                });
            }
            if (pressed(Btn::Select))
                audio::stop();
            if (pressed(Btn::Start)) {
                n_ = 0;
                audio::play(this);
            }
        }

        void draw() override {
            driver_.fill();
            driver_.textMultiline(0,20) << 
                "n:         " << n_ << "\n" <<
                "min:       " << min_ << "\n" << 
                "max:       " << max_ << "\n" << 
                "amp:       " << static_cast<uint16_t>(max_ - min_) << "\n";
        }


        uint8_t * buffer_;

        volatile unsigned n_ = 0;
        volatile uint16_t min_ = 0;
        volatile uint16_t max_ = 0;
        


    }; // RCKid::MicTest

}