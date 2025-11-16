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
    class PiggyBank : public ui::Form<void> {
    public:

        String name() const override { return "PiggyBank"; }

        PiggyBank(): 
            ui::Form<void>{},
            cash_{Rect::XYWH(0, 30, 320, 130), ""}
         {
            g_.addChild(cash_);
            cash_.setFont(Font::fromROM<assets::OpenDyslexic128>());
            cash_.setHAlign(HAlign::Center);
            value_ = 200;
            cash_.setText(STR(value_));
            contextMenu_.add(ui::ActionMenu::Item("Add", [this]() {
                /*
                auto t = App::run<TimeDialog>("Select time");
                if (t.has_value()) {
                    TinyDateTime now = timeNow();
                    now.time = t.value();
                    setTimeNow(now);
                }
                */
            }));
            contextMenu_.add(ui::ActionMenu::Item("Remove", [this]() {
                /*
                auto d = App::run<DateDialog>();
                if (d.has_value()) {
                    TinyDateTime now = timeNow();
                    now.date = d.value();
                    setTimeNow(now);
                }
                    */
            }));
            contextMenu_.add(ui::ActionMenu::Item("Set monthly value"));
        }

        void update() override {
            ui::Form<void>::update();

            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                exit();

            if (btnPressed(Btn::Select)) {
                auto action = App::run<PopupMenu<ui::Action>>(&contextMenu_);
                if (action.has_value())
                    action.value()();
            }
        }

        void draw() override {
            ui::Form<void>::draw();
        }

    private:
        ui::Label cash_;
        ui::ActionMenu contextMenu_; 

        int32_t value_ = 0;

    }; // rckid::Clock

} // namespace rckid