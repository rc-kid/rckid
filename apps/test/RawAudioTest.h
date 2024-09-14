#include <platform/buffer.h>
#include <rckid/app.h>
#include <rckid/graphics/canvas.h>
#include <rckid/assets/tests.h>

namespace rckid {

    class RawAudioTest : public GraphicsApp<Canvas<ColorRGB>> {
    public:
        RawAudioTest():
            GraphicsApp{Canvas<Color>{320, 240}},
            buf_{BUFFER_FRAMES * 4, [this](DoubleBuffer &) {
                refill();
            }} {
                audioEnable();
                refill();
            }

    protected:

        void update() override {
            if (btnPressed(Btn::A)) {
                audioPlay(buf_, 44100);                
            }
            if (btnPressed(Btn::B)) {
                audioStop();
            }
        }

        void draw() override {
            g_.fill();
            //g_.text(10,10) << "Music to my ears:";
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
        static constexpr uint32_t INPUT_SIZE = sizeof(assets::tests::raw_audio_44100_16_signed) / 2;

        void refill() {
            int16_t * buf = reinterpret_cast<int16_t*>(buf_.getBackBuffer());
            int16_t const * input = reinterpret_cast<int16_t const *>(assets::tests::raw_audio_44100_16_signed);
            for (uint32_t i = 0; i < BUFFER_FRAMES; ++i) {
                int16_t v = input[ii_];
                if (++ii_ == INPUT_SIZE)
                    ii_ = 0;
                buf[i * 2] = v;
                buf[i * 2 + 1] = v;
            }
        }

        DoubleBuffer buf_;
        uint32_t ii_ = 0;

        /*

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

        */


        // some sample music, from https://pixabay.com/music/main-title-cinematic-dark-trailer-43sec-178297/
//        static inline const uint16_t raw_[] = {
//#include "test/audio/44100_16_signed.raw.data"
//        };

    }; // rckid::RawAudiotest

} // namespace rckid