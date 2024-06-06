#pragma once 

#include <hardware/pwm.h>

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/audio/audio.h"

namespace rckid {

    class MicTest: public App<FrameBuffer<ColorRGB>>, public audio::OutStream {
    public:

        static MicTest * create() { return new MicTest{}; }

        MicTest():
            buffer_{new uint16_t[16384]} {
        }

        void fillBuffer(uint16_t * buffer, size_t bufferSize) override {
            UNIMPLEMENTED;
        }


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
                audio::record(nullptr);
                last_ = 0;
                sum_ = 0;
            }
            if (pressed(Btn::Select)) {
                audio::stop();
                last_ = 0;
            }
            last_ = pwm_get_counter(RP_MIC_SLICE);
            sum_ = sum_ + last_;
        }

        void draw() override {
            driver_.fill();
            driver_.textMultiline(0,20) << 
                "last:      " << last_ << "\n" <<
                "sum:       " << sum_ << "\n";
        }


        uint16_t * buffer_; 
        volatile unsigned last_ = 0;
        volatile unsigned sum_ = 0;
        
    }; // RCKid::MicTest

}