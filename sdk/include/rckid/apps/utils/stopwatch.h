#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/label.h>
#include <rckid/ui/image.h>
#include <rckid/ui/style.h>

#include <assets/OpenDyslexic128.h>
#include <assets/OpenDyslexic64.h>

namespace rckid {

    /** Simple clock app. 
     
        Shows date & time.

        TODO add alarm as well.
     */
    class Stopwatch : public ui::App<void> {
    public:

        String name() const override { return "Stopwatch"; }

        Stopwatch() {
            using namespace ui;
            time_ = addChild(new ui::Label{})
                << SetFont(assets::OpenDyslexic128)
                << SetHAlign(HAlign::Center)
                << SetRect(Rect::XYWH(0, 50, 320, 130));
            msTime_ = addChild(new ui::Label{})
                << SetFont(assets::OpenDyslexic64)
                << SetHAlign(HAlign::Center)
                << SetRect(Rect::XYWH(0, 180, 320, 40));
        }

    protected:
        void onLoopStart() override {
            ui::App<void>::onLoopStart();
            root_.flyIn();
        }

        void loop() {
            ui::App<void>::loop();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                exit();
                // wait for idle to make sure we are exiting from known state
                waitUntilIdle();
                root_.flyOut();
                waitUntilIdle();
            }
            if (btnPressed(Btn::A)) {
                btnClear(Btn::A);
                if (running_) {
                    running_ = false;
                } else {
                    running_ = true;
                    last_ = time::uptimeUs();
                }
            }
            if (btnPressed(Btn::Start)) {
                running_ = false;
                last_ = 0;
                h_ = 0;
                m_ = 0;
                s_ = 0;
                ms_ = 0;
            }
            if (running_) {
                uint32_t now = time::uptimeUs();
                uint32_t delta = (now - last_ + 500) / 1000;
                last_ = now;
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
            time_->setText(STR(h_ << ":" << fillLeft(m_, 2, '0') << ":" << fillLeft(s_, 2, '0')));
            msTime_->setText(STR("." << fillLeft(ms_, 3, '0')));
        }

    private:
        ui::Label * time_;
        ui::Label * msTime_;

        bool running_ = false;
        uint32_t h_ = 0;
        uint32_t m_ = 0;
        uint32_t s_ = 0;
        uint32_t ms_ = 0;
        uint64_t last_ = 0; 

    }; // rckid::Clock

} // namespace rckid