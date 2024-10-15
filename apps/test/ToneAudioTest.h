#include <platform/buffer.h>
#include <rckid/app.h>
#include <rckid/graphics/canvas.h>
#include <rckid/assets/tests.h>
#include <rckid/audio/tone.h>

namespace rckid {

    class ToneAudioTest : public GraphicsApp<Canvas<ColorRGB>> {
    public:

        static void run() {
            ToneAudioTest app{};
            app.loop();
        }

    protected:
        ToneAudioTest():
            GraphicsApp{Canvas<Color>{320, 240}},
            buf_{BUFFER_FRAMES * 4, [this](DoubleBuffer &) {
                refill();
            }} {
                audioOn();
                refill();
                audioPlay(buf_, 44100);
            }


        void update() override {
            if (btnPressed(Btn::A)) {
                tone_.setFrequency(440, 1000);
            }
            if (btnPressed(Btn::B)) {
                tone_.off();
            }
        }

        void draw() override {
            g_.fill();
            g_.text(10,20) << 
                "\nUpdate calls: " << updates_;
        }


        /*

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

        */

    private:

        static constexpr uint32_t BUFFER_FRAMES = 512;
        uint32_t updates_ = 0;

        Tone tone_{Tone::square()};

        void refill() {
            ++updates_;
            int16_t * buf = reinterpret_cast<int16_t*>(buf_.getBackBuffer());
            for (uint32_t i = 0; i < BUFFER_FRAMES; ++i) {
                int16_t v = tone_.next();
                buf[i * 2] = v;
                buf[i * 2 + 1] = v;
            }
        }

        DoubleBuffer buf_;

    }; // rckid::RawAudiotest

} // namespace rckid