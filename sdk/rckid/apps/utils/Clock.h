#pragma once

#include "../../app.h"
#include "../../ui/form.h"
#include "../../ui/label.h"
#include "../../ui/image.h"
#include "../../assets/fonts/OpenDyslexic128.h"
#include "../../assets/fonts/OpenDyslexic64.h"
#include "../../assets/icons_24.h"
#include "../dialogs/TimeDialog.h"
#include "../dialogs/DateDialog.h"

namespace rckid {
    class Clock : public ui::Form<void> {
    public:

        String name() const override { return "Clock"; }

        Clock(): 
            ui::Form<void>{},
            h_{Rect::XYWH(0, 30, 150, 130), ""},
            m_{Rect::XYWH(170, 30, 150, 130), ""},
            s_{Rect::XYWH(260, 42, 60, 130), ""},
            colon_{Rect::XYWH(150, 30, 20, 130), ":" },
            date_{Rect::XYWH(0, 140, 320, 40), ""},
            alarm_{Rect::XYWH(120, 200, 320, 40), "00:00"},
            alarmIcon_{90, 208, Icon{assets::icons_24::alarm_clock}}
         {
            g_.addChild(h_);
            h_.setFont(Font::fromROM<assets::OpenDyslexic128>());
            h_.setHAlign(HAlign::Right);
            g_.addChild(m_);
            m_.setFont(Font::fromROM<assets::OpenDyslexic128>());
            m_.setHAlign(HAlign::Left);
            g_.addChild(s_);
            s_.setFont(Font::fromROM<assets::OpenDyslexic64>());
            s_.setHAlign(HAlign::Left);
            s_.setColor(ui::Style::accentFg());
            g_.addChild(colon_);
            colon_.setFont(Font::fromROM<assets::OpenDyslexic128>());
            g_.addChild(date_);
            date_.setFont(Font::fromROM<assets::OpenDyslexic64>());
            date_.setHAlign(HAlign::Center);
            g_.addChild(alarm_);
            alarm_.setFont(Font::fromROM<assets::OpenDyslexic64>());
            alarm_.setHAlign(HAlign::Left);
            g_.addChild(alarmIcon_);
            contextMenu_.add(ui::ActionMenu::Item("Set time", [this]() {
                auto t = App::run<TimeDialog>("Select time");
                if (t.has_value()) {
                    TinyDateTime now = timeNow();
                    now.time = t.value();
                    setTimeNow(now);
                }
            }));
            contextMenu_.add(ui::ActionMenu::Item("Set date", [this]() {
                auto d = App::run<DateDialog>();
                if (d.has_value()) {
                    TinyDateTime now = timeNow();
                    now.date = d.value();
                    setTimeNow(now);
                }
            }));
            contextMenu_.add(ui::ActionMenu::Item("Set alarm"));
        }

        void update() override {
            ui::Form<void>::update();

            TinyAlarm a = timeAlarm();
            if (a.enabled()) {
                alarm_.setVisible(true);
                alarmIcon_.setVisible(true);
            } else {
                alarm_.setVisible(false);
                alarmIcon_.setVisible(false);
            }

            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                exit();

            if (btnPressed(Btn::Select)) {
                auto action = App::run<PopupMenu<ui::Action>>(&contextMenu_);
                if (action.has_value())
                    action.value()();
            }
        }

        void draw() override {
            TinyDateTime t = timeNow();
            TinyAlarm a = timeAlarm();
            h_.setText(STR(fillLeft(t.time.hour(), 2, '0')));
            m_.setText(STR(fillLeft(t.time.minute(), 2, '0')));
            s_.setText(STR(fillLeft(t.time.second(), 2, '0')));
            colon_.setVisible(t.time.second() & 1);
            date_.setText(STR(t.date.day() << "/" << t.date.month() << "/" << t.date.year()));
            alarm_.setText(STR(fillLeft(a.hour(), 2, '0') << ":" << fillLeft(a.minute(), 2, '0')));

            ui::Form<void>::draw();
        }

    private:
        ui::Label h_;
        ui::Label m_;
        ui::Label s_;
        ui::Label colon_;
        ui::Label date_;
        ui::Label alarm_;
        ui::Image alarmIcon_;
        ui::ActionMenu contextMenu_; 

    }; // rckid::Clock

} // namespace rckid