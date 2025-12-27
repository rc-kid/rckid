#pragma once

#include "../../app.h"
#include "../../ui/form.h"
#include "../../ui/label.h"
#include "../../ui/image.h"
#include "../../assets/fonts/OpenDyslexic128.h"
#include "../../assets/fonts/OpenDyslexic64.h"
#include "../../assets/icons_24.h"
#include "../dialogs/InfoDialog.h"
#include "../dialogs/TimeDialog.h"
#include "../dialogs/DateDialog.h"
#include "../dialogs/FileDialog.h"
#include "../dialogs/PopupMenu.h"
#include "../utils/Alarm.h"

namespace rckid {
    class Clock : public ui::Form<void> {
    public:

        String name() const override { return "Clock"; }

          Clock(): 
                ui::Form<void>{}
            {
                h_ = g_.addChild(new ui::Label{Rect::XYWH(0, 30, 150, 130), ""});
                h_->setFont(Font::fromROM<assets::OpenDyslexic128>());
                h_->setHAlign(HAlign::Right);
                m_ = g_.addChild(new ui::Label{Rect::XYWH(170, 30, 150, 130), ""});
                m_->setFont(Font::fromROM<assets::OpenDyslexic128>());
                m_->setHAlign(HAlign::Left);
                s_ = g_.addChild(new ui::Label{Rect::XYWH(260, 42, 60, 130), ""});
                s_->setFont(Font::fromROM<assets::OpenDyslexic64>());
                s_->setHAlign(HAlign::Left);
                s_->setColor(ui::Style::accentFg());
                colon_ = g_.addChild(new ui::Label{Rect::XYWH(150, 30, 20, 130), ":"});
                colon_->setFont(Font::fromROM<assets::OpenDyslexic128>());
                date_ = g_.addChild(new ui::Label{Rect::XYWH(0, 140, 320, 40), ""});
                date_->setFont(Font::fromROM<assets::OpenDyslexic64>());
                date_->setHAlign(HAlign::Center);
                alarm_ = g_.addChild(new ui::Label{Rect::XYWH(120, 200, 320, 40), "00:00"});
                alarm_->setFont(Font::fromROM<assets::OpenDyslexic64>());
                alarm_->setHAlign(HAlign::Left);
                alarmIcon_ = g_.addChild(new ui::Image{90, 208, Icon{assets::icons_24::alarm_clock}});
            if (parentMode()) {
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
                contextMenu_.add(ui::ActionMenu::Item("Set silent period", [this]() {
                    Alarm::Settings settings = Alarm::Settings::load();
                    auto start = App::run<TimeDialog>("Start", settings.silentStart());
                    if (!start.has_value())
                        return;
                    auto end = App::run<TimeDialog>("End", settings.silentEnd());
                    if (!end.has_value())
                        return;
                    settings.setSilentPeriod(start.value(), end.value()).save();
                }));
            }
            contextMenu_.add(ui::ActionMenu::Item("Set Alarm", [this]() {
                TinyAlarm ta = timeAlarm();
                auto t = App::run<TimeDialog>("Select alarm time", TinyTime{ta.hour(), ta.minute()});
                if (t.has_value()) {
                    if (Alarm::Settings::load().isSilentPeriod(t.value()))
                        InfoDialog::error("Invalid time", "Alarm time is within silent period!");
                    else
                        setTimeAlarm(t.value());
                }
            }));
            contextMenu_.add(ui::ActionMenu::Item("Disable Alarm", []() {
                TinyAlarm ta = timeAlarm();
                ta.setEnabled(false);
                setTimeAlarm(ta);
            }));
            contextMenu_.add(ui::ActionMenu::Item("Set alarm sound", []() {
                auto result = App::run<FileDialog>("Select alarm sound", "/files/music/");
                if (result.has_value()) {
                    Alarm::Settings::load().setSound(result.value()).save();
                }
            }));
        }

        void update() override {
            ui::Form<void>::update();

            TinyAlarm a = timeAlarm();
            if (a.enabled()) {
                alarm_->setVisible(true);
                alarmIcon_->setVisible(true);
            } else {
                alarm_->setVisible(false);
                alarmIcon_->setVisible(false);
            }

            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                exit();

            if (btnPressed(Btn::Select) && contextMenu_.size() != 0) {
                auto action = App::run<PopupMenu<ui::Action>>(&contextMenu_);
                if (action.has_value())
                    action.value()();
            }
        }

        void draw() override {
            TinyDateTime t = timeNow();
            TinyAlarm a = timeAlarm();
            h_->setText(STR(fillLeft(t.time.hour(), 2, '0')));
            m_->setText(STR(fillLeft(t.time.minute(), 2, '0')));
            s_->setText(STR(fillLeft(t.time.second(), 2, '0')));
            colon_->setVisible(t.time.second() & 1);
            date_->setText(STR(t.date.day() << "/" << t.date.month() << "/" << t.date.year()));
            alarm_->setText(STR(fillLeft(a.hour(), 2, '0') << ":" << fillLeft(a.minute(), 2, '0')));

            ui::Form<void>::draw();
        }

    private:
        ui::Label * h_;
        ui::Label * m_;
        ui::Label * s_;
        ui::Label * colon_;
        ui::Label * date_;
        ui::Label * alarm_;
        ui::Image * alarmIcon_;
        ui::ActionMenu contextMenu_; 

    }; // rckid::Clock

} // namespace rckid