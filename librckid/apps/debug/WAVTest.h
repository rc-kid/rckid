#pragma once 

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/audio/audio_stream.h"

namespace rckid {

    /** Plays audio stored in ROM. 
     
        A very simple test of audio output quality. Copies data from ROM to RAM buffer from which it fills the audio stream buffer. 
     */
    class WAVTest: public App<FrameBuffer<ColorRGB>>, public AudioStream {
    public:

        static WAVTest * create() { return new WAVTest{}; }

        WAVTest():
            buffer_{new uint16_t[16384]} {
        }

        void fillBuffer(uint16_t * buffer, size_t bufferSize) override {
            for (int i = 0; i < bufferSize; i += 2) {
                uint16_t x = buffer_[ri_++];
                buffer[i] = x;
                buffer[i+1] = x;
            }

            if (ri_ >= 16384)
                ri_ = 0;

            ++refills_;
        }


    protected:

        void onFocus() override {
            App::onFocus();
            // fill the entire buffer
            for (romi_ = 0; romi_ < 16384; ++romi_)
                buffer_[romi_] = wave_[romi_] / 16;
        }

        void onBlur() override {
            App::onBlur();
            stop();
        }

        void update() override {
            App::update();
            if (pressed(Btn::A)) {
                playing_ ? pause() : play(this);
                playing_ = !playing_;
            }
            fillBufferFromROM();
        }

        void draw() override {
            driver_.fill();
            driver_.textMultiline(0,20) << 
                "ROM:     " << romi_ << "\n" <<
                "R:       " << ri_ << "\n" <<
                "W:       " << wi_ << "\n" <<
                "DATA:    " << buffer_[ri_] << "\n" << 
                "Refills: " << refills_;
        }

        void fillBufferFromROM() {
            unsigned end = ri_;
            while (wi_ != end) {
                buffer_[wi_++] = wave_[romi_++] / 16;
                if (romi_ >= sizeof(wave_) / 2)
                    romi_ = 0;
                if (wi_ >= 16384)
                    wi_ = 0;
            }
        }

        uint16_t * buffer_; 
        // where to read from in the RAM buffer
        volatile unsigned ri_ = 0;
        // where to write to in the RAM buffer
        volatile unsigned wi_ = 0;
        // where to read from in the ROM data
        volatile unsigned romi_ = 0;

        unsigned refills_ = 0;

        bool playing_ = false;

        static constexpr uint16_t wave_[] = {
#include "assets-debug/faryra.raw.inc"
        };
        
    }; // RCKid::WAVTest


}