#pragma once

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/audio.h"
#include "fonts/Iosevka_Mono6pt7b.h"

namespace rckid {

    class RawAudioTest : public App<FrameBuffer<ColorRGB>> {
    public:
        RawAudioTest() = default;

    protected:

        static constexpr size_t BUFFER_SIZE = 2048 * 10;

        void onFocus() override {
            App::onFocus();
            buffer_ = new uint16_t[BUFFER_SIZE];
            bOther_ = buffer_ + BUFFER_SIZE / 2;
            for (size_t i = 0; i < BUFFER_SIZE; ++i)
                buffer_[i] = 0;
            offset_ = 0;
            refill(buffer_, BUFFER_SIZE);
            audio::initialize();
            audio::setAudioEnabled(true);
            audio::startPlayback(SampleRate::kHz44_1, buffer_, BUFFER_SIZE, [this](uint16_t * buffer, size_t bufSize) {
                refill(buffer, bufSize);
            });
            driver_.setFg(ColorRGB::White());
            driver_.setFont(Iosevka_Mono6pt7b);
            driver_.setBg(ColorRGB::Black());

        }

        void update() override {
            if (pressed(Btn::A))
                audio::setAudioEnabled(false);
            if (pressed(Btn::B))
                audio::setAudioEnabled(true);
        }

        void draw() override {
            driver_.fill();
            driver_.textMultiline(5, 10) << offset_
                          << "    \n"
                          << " FPS: " << stats::fps() << " S:" << stats::systemUs() << " U:" << (stats::updateUs() / 1000) << " D:" << (stats::drawUs() / 1000)
                          << "    \n ERR:"
                          << errors_ << "    \n"
                          << " EN:" << (audio::audioEnabled() ? "Yes" : "No ") << " HP: " << (audio::headphones() ? "Yes" : "No ");
        }

    private:

        void refill(uint16_t * buffer, size_t size) {
            if (buffer != buffer_ && buffer != bOther_)
                ++errors_;
            //offset_ += size / 2;
            if (size != BUFFER_SIZE / 2)
                ++errors_;
            for (size_t i = 0; i < size; i += 2) {
                unsigned x = raw_[offset_];
                x = (x + 8) >> 4;
                //x = ((x + 128) >> 8) << 4;
                buffer[i] = x;
                buffer[i + 1] = x;
                if (++offset_ >= sizeof(raw_)/2)
                    offset_ = 0;
            }
        }

        bool ok_ = false;
        size_t offset_ = 0;


        uint16_t * bOther_;
        size_t errors_ = 0;


        uint16_t * buffer_ = nullptr;

        // some sample music, from https://pixabay.com/music/main-title-cinematic-dark-trailer-43sec-178297/
        static inline const uint16_t raw_[] = {
#include "test/audio/44100_16_signed.raw.data"
        };

    }; // rckid::RawAudiotest

} // namespace rckid