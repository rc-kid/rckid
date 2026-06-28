#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>
#include <assets/OpenDyslexic128.h>
#include <assets/OpenDyslexic64.h>

namespace rckid {

    class Recorder : public ui::App<void> {
    public:

        enum class Source {
            Mic,
            Cartridge,
        };

        String name() const override { return "Recording"; }

        Recorder(Source source = Source::Mic): 
            ui::App<void>{Rect::XYWH(0, 140, 320, 96)}
        {
            using namespace ui;
            time_ = addChild(new Label{})
                << SetRect(Rect::XYWH(0, 0, 320, 20))
                << SetText("");

            waveform_ = addChild(new Waveform{});

            //time_ = g_.addChild(new ui::Label{Rect::XYWH(0, 0, 320, 20), ""});
            //waveform_ = g_.addChild(new Waveform{});
            
            auto f = [this](int16_t * & samples, uint32_t & numSamples) {
                    if (samples == nullptr) {
                        // provide buffer to fill
                        samples = buf_.front().data();
                        numSamples = buf_.size() / 2;
                        buf_.swap();
                    } else {
                        waveform_->processSamples(samples, numSamples);
                    }
                };
            if (source == Source::Mic) {
                audio::recordMic(8000, f);
            } else {
                audio::recordLineIn(8000, f);
            }
        }

        ~Recorder() override {
            audio::stop();
        }

        void loop() override {
            ui::App<void>::loop();
            //time_->setText(STR(min_ << " -- " << max_ << ", " << delta_));
            if (!btnDown(Btn::A))
                exit();
        }   

    private:

    class Waveform : public ui::Widget {
        public:

            Waveform() {
                memset(wave_, 0, sizeof(wave_));
                setRect(Rect::XYWH(100, 32, 200, 64));
            }

            void processSamples(int16_t const * samples, uint32_t numSamples) {
                for (uint32_t i = 0; i < numSamples; i += 2) {
                    if (sampleIndex_ >= samplesPerPixel_) {
                        sampleIndex_ = 0;
                        wave_[waveIndex_++] = (max_ - min_) / 256;
                        if (waveIndex_ >= 200)
                            waveIndex_ = 0;
                        min_ = 32767;
                        max_ = -32768;
                    }
                    if (samples[i] < min_)
                        min_ = samples[i];
                    if (samples[i] > max_)
                        max_ = samples[i];
                    ++sampleIndex_;
                }
            }

        protected:
            
            void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
                column = (column + waveIndex_) % 200;
                // TODO render the column here
                for (Coord y = startRow; y < startRow + numPixels; ++y) {
                    if (y >= 32 - wave_[column] && y <= 32 + wave_[column]) {
                        buffer[y] = fg_;
                    } else {
                        buffer[y] = bg_;
                    }
                }
            }


        private:
            Color fg_ = Color::Green();
            Color bg_ = Color::Black();
            uint8_t wave_[200];
            uint32_t waveIndex_ = 0;
            uint32_t sampleIndex_ = 0;
            uint32_t samplesPerPixel_ = 40;
            int16_t min_ = 32767;
            int16_t max_ = -32768;
        };

        ui::Label * time_;

        uint32_t samples_ = 0;

        DoubleBuffer<int16_t> buf_{1024};
        Waveform * waveform_;
        
    }; // rckid::Recorder

} // namespace rckid
