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

        void onFocus(BaseApp * previous) override {
            App::onFocus(previous);
            buffer_ = new uint16_t[BUFFER_SIZE];
            bOther_ = buffer_ + BUFFER_SIZE / 2;
            for (size_t i = 0; i < BUFFER_SIZE; ++i)
                buffer_[i] = 0;
            offset_ = 0;
            refill(buffer_, BUFFER_SIZE);
            Audio::initialize();
            Audio::setAudioEnabled(true);
            Audio::startPlayback(SampleRate::kHz44_1, buffer_, BUFFER_SIZE, [this](uint16_t * buffer, size_t bufSize) {
                refill(buffer, bufSize);
            });
            Renderer & r = renderer();
            r.setFg(ColorRGB::White());
            r.setFont(Iosevka_Mono6pt7b);
            r.setBg(ColorRGB::Black());

        }

        void update() override {

        }

        void draw() override {
            Renderer & r = renderer();
            r.fill();
            r.text(5, 10) << offset_
                          << "\n\n"
                          << " FPS: " << fps() << " S:" << systemUs() << " U:" << (updateUs() / 1000) << " D:" << (drawUs() / 1000)
                          << "    \n\n ERR:"
                          << errors_;
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