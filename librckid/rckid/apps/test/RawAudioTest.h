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

        static constexpr size_t BUFFER_SIZE = 8192;

        void onFocus(BaseApp * previous) override {
            App::onFocus(previous);
            buffer_ = new uint16_t[BUFFER_SIZE];
            offset_ = 0;
            refill(buffer_, BUFFER_SIZE / 2);
            Audio::initialize();
            Audio::setAudioEnabled(true);
            Audio::startPlayback(SampleRate::kHz8, buffer_, BUFFER_SIZE / 2, [this](uint16_t * buffer, size_t stereoSamples) {
                refill(buffer, stereoSamples * 2);
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
                          << " FPS: " << fps() << " S:" << systemUs() << " U:" << (updateUs() / 1000) << " D:" << (drawUs() / 1000);
        }

    private:

        void refill(uint16_t * buffer, size_t size) {

            for (size_t i = 0; i < size; i += 2) {
                buffer[i] = raw_[offset_];
                buffer[i + 1] = raw_[offset_];
                if (++offset_ >= sizeof(raw_))
                    offset_ = 0;
            }
        }

        bool ok_ = false;
        size_t offset_ = 0;


        uint16_t * buffer_ = nullptr;

        // some sample music, from https://pixabay.com/music/main-title-cinematic-dark-trailer-43sec-178297/
        static inline const uint8_t raw_[] = {
#include "test/audio/8000.raw.data"
        };

    }; // rckid::RawAudiotest

} // namespace rckid