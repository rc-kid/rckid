#pragma once


#include "fonts/Iosevka_Mono6pt7b.h"

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/audio.h"
#include "rckid/audio/tone.h"

namespace rckid {

    class AudioTestTone : public FBApp<FrameBuffer<ColorRGB>> {
    public:
        AudioTestTone() = default;

    protected:

        void onFocus() override {
            FBApp::onFocus();
            buffer_ = new uint16_t[8192];
            offset_ = 0;
            tone_.play(440);
            refill(buffer_, 4096);
            Audio::initialize();
            Audio::setAudioEnabled(true);
            Audio::startPlayback(SampleRate::kHz44_1, buffer_, 4096, [this](uint16_t * buffer, size_t stereoSamples) {
                refill(buffer, stereoSamples);
            });
            fb_.setFg(ColorRGB::White());
            fb_.setFont(Iosevka_Mono6pt7b);
            fb_.setBg(ColorRGB::Black());
        }

        void update() override {

        }

        void draw() override {
            fb_.fill();
            fb_.text(5, 10) << offset_;
        }

    private:

        void refill(uint16_t * buffer, size_t size) {
            tone_.fillBuffer(buffer, size);
            offset_ += size / 2;
        }

        Tone tone_;

        size_t offset_ = 0;

        uint16_t * buffer_ = nullptr;

    }; // rckid::RawAudiotest

} // namespace rckid