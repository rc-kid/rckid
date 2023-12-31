#pragma once


#include "fonts/Iosevka_Mono6pt7b.h"

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/audio.h"
#include "rckid/audio/tone.h"

namespace rckid {

    class AudioTestTone : public App<FrameBuffer> {
    public:
        AudioTestTone() = default;

    protected:

        void onFocus(BaseApp * previous) override {
            App::onFocus(previous);
            buffer_ = new uint16_t[8192];
            offset_ = 0;
            tone_.play(440);
            refill(buffer_, 4096);
            Audio::initialize();
            Audio::setAudioEnabled(true);
            Audio::startPlayback(SampleRate::kHz44_1, buffer_, 4096, [this](uint16_t * buffer, size_t stereoSamples) {
                refill(buffer, stereoSamples);
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
            tone_.fillBuffer(buffer, size);
            offset_ += size / 2;
        }

        Tone tone_;

        size_t offset_ = 0;

        uint16_t * buffer_ = nullptr;

    }; // rckid::RawAudiotest

} // namespace rckid