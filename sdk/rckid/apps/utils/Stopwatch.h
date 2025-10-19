#pragma once

#include "../../app.h"
#include "../../ui/form.h"
#include "../../ui/label.h"
#include "../../ui/image.h"
#include "../../assets/fonts/OpenDyslexic128.h"
#include "../../assets/fonts/OpenDyslexic64.h"

namespace rckid {
    class Stopwatch : public ui::Form<void> {
    public:

        String name() const override { return "Stopwatch"; }

        Stopwatch(): 
            ui::Form<void>{},
            time_{Rect::XYWH(0, 50, 320, 130), ""},
            msTime_{Rect::XYWH(0, 180, 320, 40), ""}
         {
            g_.addChild(time_);
            time_.setFont(Font::fromROM<assets::OpenDyslexic128>());
            g_.addChild(msTime_);
            msTime_.setFont(Font::fromROM<assets::OpenDyslexic64>());
        }

        void update() override {
            ui::Form<void>::update();
            if (running_) {
                uint32_t now = uptimeUs();
                uint32_t delta = (now - lastTime_ + 500) / 1000;
                lastTime_ = now;
                ms_ += delta;
                if (ms_ >=1000) {
                    ms_ -= 1000;
                    if (++s_ == 60) {
                        s_ = 0;
                        if (++m_ == 60) {
                            m_ = 0;
                            ++h_;
                        }
                    }
                }
            }
            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                exit();
            if (btnPressed(Btn::A)) {
                btnClear(Btn::A);
                if (running_) {
                    running_ = false;
                } else {
                    running_ = true;
                    lastTime_ = uptimeUs();
                }
            }
            if (btnPressed(Btn::Start)) {
                btnClear(Btn::Start);
                running_ = false;
                ms_ = 0;
                s_ = 0;
                m_ = 0;
                h_ = 0;
            }
        }

        void draw() override {
            time_.setText(STR(h_ << ":" << fillLeft(m_, 2, '0') << ":" << fillLeft(s_, 2, '0')));
            msTime_.setText(STR("." << fillLeft(ms_, 3, '0')));
            ui::Form<void>::draw();
        }

    private:
        uint32_t ms_ = 0;
        uint32_t s_ = 0;
        uint32_t m_ = 0;
        uint32_t h_ = 0;

        ui::Label time_;
        ui::Label msTime_;

        bool running_ = false;
        uint32_t lastTime_ = 0;
    }; // rckid::Stopwatch

} // namespace rckid