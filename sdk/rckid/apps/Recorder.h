#pragma once

#include "../app.h"
#include "../ui/form.h"
#include "../ui/label.h"
#include "../ui/image.h"
#include "../assets/fonts/OpenDyslexic128.h"
#include "../assets/fonts/OpenDyslexic64.h"

namespace rckid {


    class Recorder : public ui::Form<void> {
    public:

        String name() const override { return "InfoDialog"; }

        Recorder(): 
            ui::Form<void>{Rect::XYWH(0, 144, 320, 96), /* raw */ true}
        {
            time_ = g_.addChild(new ui::Label{Rect::XYWH(0, 0, 320, 20), ""});
            waveform_ = g_.addChild(new Waveform{});
            audioRecordMic(8000, [this](int16_t * & samples, uint32_t & numSamples) {
                if (samples == nullptr) {
                    // provide buffer to fill
                    samples = buf_.front();
                    numSamples = buf_.size() / 2;
                    buf_.swap();
                } else {
                    waveform_->processSamples(samples, numSamples);
                }
            });
        }

        ~Recorder() override {
            audioStop();
        }


        void update() override {
            if (!btnDown(Btn::A))
                exit();
        }   

        void draw() override {
            //time_->setText(STR(min_ << " -- " << max_ << ", " << delta_));
            ui::Form<void>::draw();
        }

    private:
        class Waveform : public ui::Widget {
        public:

            Waveform(): ui::Widget{Rect::XYWH(100, 32, 200, 64)} {
                memset(wave_, 0, sizeof(wave_));
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
            
            void renderColumn(Coord column, uint16_t * buffer, Coord starty, Coord numPixels) override {
                column = (column + waveIndex_) % 200;
                // TODO render the column here
                for (Coord y = starty; y < starty + numPixels; ++y) {
                    if (y >= 32 - wave_[column] && y <= 32 + wave_[column]) {
                        buffer[y] = ui::Style::accentFg().toRaw();
                    } else {
                        buffer[y] = ui::Style::bg().toRaw();
                    }
                }
            }


        private:
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
