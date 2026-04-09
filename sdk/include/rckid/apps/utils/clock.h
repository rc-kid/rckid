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
    class Clock : public ui::App<void> {
    public:

        String name() const override { return "Clock"; }

        Clock() {
            using namespace ui;
            h_ = addChild(new ui::Label{})
                << SetFont(assets::OpenDyslexic128)
                << SetHAlign(HAlign::Right)
                << SetRect(Rect::XYWH(0, 30, 130, 130));
            m_ = addChild(new ui::Label{})
                << SetFont(assets::OpenDyslexic128)
                << SetHAlign(HAlign::Left)
                << SetRect(Rect::XYWH(160, 30, 150, 130));
            s_ = addChild(new ui::Label{})
                << SetFont(assets::OpenDyslexic64)
                << SetHAlign(HAlign::Left)
                //<< SetColor(Style::::accentFg())
                << SetRect(Rect::XYWH(260, 42, 60, 130));
            colon_ = addChild(new ui::Label{})
                << SetFont(assets::OpenDyslexic128)
                << SetHAlign(HAlign::Left)
                << SetText(":")
                << SetRect(Rect::XYWH(135, 30, 20, 130));
            date_ = addChild(new ui::Label{})
                << SetFont(assets::OpenDyslexic64)
                << SetHAlign(HAlign::Center)
                << SetRect(Rect::XYWH(0, 140, 320, 40));
            //alarm_ = addChild(new ui::Label{})
            //    << SetFont(assets::OpenDyslexic64)
            //    << SetHAlign(HAlign::Left)
            //    << SetRect(Rect::XYWH(120, 200, 220, 40));
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
            TinyDateTime t = time::now();

            h_->setText(STR(fillLeft(t.time.hour(), 2, '0')));
            m_->setText(STR(fillLeft(t.time.minute(), 2, '0')));
            s_->setText(STR(fillLeft(t.time.second(), 2, '0')));
            colon_->setVisibility(t.time.second() & 1);
            date_->setText(STR(t.date.day() << "/" << t.date.month() << "/" << t.date.year()));

        }

    private:
        ui::Label * h_;
        ui::Label * m_;
        ui::Label * s_;
        ui::Label * colon_;
        ui::Label * date_;
        //ui::Label * alarm_;
        //ui::Image * alarmIcon_;

    }; // rckid::Clock

} // namespace rckid