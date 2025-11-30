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
                ui::Form<void>{}
            {
                cash_ = g_.addChild(new ui::Label{Rect::XYWH(0, 30, 320, 130), ""});
                topupIcon_ = g_.addChild(new ui::Image{Rect::XYWH(130, 180, 24, 24), Icon{assets::icons_24::money_bag}});
                topUp_ = g_.addChild(new ui::Label{});
                cash_->setFont(Font::fromROM<assets::OpenDyslexic128>());
                cash_->setHAlign(HAlign::Center);
                topUp_->setFont(Font::fromROM<assets::OpenDyslexic32>());
            // topup countdown
            TinyDate now = timeNow().date;
            TinyDate topUp{1, static_cast<uint8_t>((now.month() == 12) ? 1 : (now.month() + 1)), now.year()};
            uint32_t daysTillTopUp = now.daysTillNextAnnual(topUp);
                topUp_->setText(STR("+" << monthlyAllowance_ << " in " << daysTillTopUp << " days"));
                Coord w = topUp_->textWidth() + 24;
                topupIcon_->setPos(160 - (w / 2), 144);
                topUp_->setPos(160 - (w / 2) + 24, 140);

            value_ = 200;
                cash_->setText(STR(value_));
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

        ~PiggyBank() override {
            saveSettings();
        }

    protected:

        void update() override {
            ui::Form<void>::update();

            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                exit();

            // piggy bank actions only work in parent mode
            if (btnPressed(Btn::Select) && parentMode()) {
                auto action = App::run<PopupMenu<ui::Action>>(&contextMenu_);
                if (action.has_value())
                    action.value()();
            }
        }

        void draw() override {
            ui::Form<void>::draw();
        }

        void saveSettings() {

        }

    private:
        ui::Label * cash_;
        ui::Label * topUp_;
        ui::Image * topupIcon_;
        ui::ActionMenu contextMenu_; 

        int32_t value_ = 0;
        int32_t monthlyAllowance_ = 0;
        TinyDate lastTopUp_;

    }; // rckid::Clock

} // namespace rckid