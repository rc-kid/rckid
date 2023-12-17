#pragma once


#include "fonts/Iosevka_Mono6pt7b.h"

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/audio.h"

namespace rckid {

    class RawAudioTest : public App<Framebuffer<display_profile::RGB>> {
    public:
        RawAudioTest() = default;
        RawAudioTest(App * parent): App{parent} {}

    protected:

        void onFocus() override {
            App::onFocus();
            buffer_ = new uint16_t[4096];
            offset_ = 0;
            refill(buffer_, 4096);
            Audio::initialize();
            Audio::startPlayback(SampleRate::kHz44_1, buffer_, 2048, [this](uint16_t * buffer, size_t stereoSamples) {
                refill(buffer, stereoSamples * 2);
            });
            Renderer & r = renderer();
            r.setFg(Color{255,255,255});
            r.setFont(Iosevka_Mono6pt7b);
            r.setBg(Color{0, 0, 0});

        }

        void update() override {

        }

        void draw() override {
            Renderer & r = renderer();
            r.fill();
            r.text(5, 10);
            r.text() << offset_; 
        }

    private:

        void refill(uint16_t * buffer, size_t size) {
            if (offset_ + size/2 >= sizeof(raw_))
                offset_ = 0;
            for (size_t i = 0; i < size; i += 2) {
                buffer[i] = raw_[offset_] / 8;
                buffer[i + 1] = raw_[offset_++] / 8;
            }
        }

        bool ok_ = false;
        size_t offset_ = 0;


        uint16_t * buffer_ = nullptr;

        // some sample music, from https://pixabay.com/music/main-title-cinematic-dark-trailer-43sec-178297/
        static inline const uint8_t raw_[] = {
#include "dark-trailer.raw.data"
        };

    }; // rckid::RawAudiotest

} // namespace rckid