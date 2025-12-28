#pragma once

#include "../../app.h"
#include "../../ui/form.h"
#include "../../ui/label.h"

namespace rckid {

    /** Very simple about dialog.
     */
    class MicTest : public ui::Form<void> {
    public:

        static constexpr uint32_t RECORDING_SIZE = 65536;

        String name() const override { return "MicTest"; }

        MicTest():
            ui::Form<void>{},
            recording_{new int16_t[RECORDING_SIZE]}
        {
            status_ = g_.addChild(new ui::Label{Rect::XYWH(0, 130, 320, 20), "RCKid mkIII, v. 0.9.0"});
        }

        ~MicTest() override {
            delete recording_;
        }
    protected:

        void update() override {
            ui::Form<void>::update();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                exit();
            if (btnPressed(Btn::A) && (state_ == State::Idle)) {
                state_ = State::Recording;
                recorded_ = 0;
                min_ = 32767;
                max_ = -32768;
                samples_ = 0;
                audioRecordMic(8000, [this](int16_t * & samples, uint32_t & numSamples) {
                    if (samples == nullptr) {
                        // provide buffer to fill
                        samples = buf_.front();
                        numSamples = buf_.size() / 2;
                        buf_.swap();
                    } else {
                        uint32_t ns = numSamples;
                        int16_t * s = samples;
                        while (recorded_ < RECORDING_SIZE && ns > 0) {
                            int16_t sample = *s++;
                            s++;
                            if (min_ > sample)
                                min_ = sample;
                            if (max_ < sample)
                                max_ = sample;
                            ++samples_;
                            recording_[recorded_++] = sample;
                            recording_[recorded_++] = sample;
                            ns -= 2;
                        }
                        if (recorded_ >= RECORDING_SIZE) {
                            audioStop();
                            state_ = State::Playing;
                            audioPlay(8000, [this](int16_t * & samples, uint32_t & numSamples) {
                                if (samples == nullptr) {
                                    // provide buffer to fill
                                    samples = recording_;
                                    numSamples = RECORDING_SIZE / 2;
                                } else {
                                    audioStop();
                                    state_ = State::Idle;
                                }
                            });
                        }
                        //waveform_->processSamples(samples, numSamples);
                    }
                });
            }
        }

        void draw() override {
            ui::Form<void>::draw();
            if (state_ == State::Recording) {
                status_->setText(STR("Recording... " << recorded_ << " samples, min: " << min_ << ", max: " << max_));
            } else if (state_ == State::Playing) {
                status_->setText(STR("Playing... " << recorded_ << " samples, min: " << min_ << ", max: " << max_));
            } else {
                status_->setText(STR("Idle. Recorded samples: " << recorded_ << ", min: " << min_ << ", max: " << max_));
            }
        }

    private:
        ui::Image * icon_;
        ui::Label * status_;
        ui::Label * budget_;
        ui::Label * sd_;

        // buffer to deal with the microphone input
        DoubleBuffer<int16_t> buf_{1024};
        // buffer to store the recording in
        int16_t * recording_;
        uint32_t recorded_ = 0;
        int16_t min_ = 32767;
        int16_t max_ = -32768;
        uint32_t samples_ = 0;

        enum class State {
            Idle,
            Recording,
            Playing,
        };

        State state_ = State::Idle;

    }; // About
} // namespace rckid