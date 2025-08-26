#pragma once

#include "../app.h"
#include "../ui/form.h"
#include "../ui/label.h"
#include "../assets/fonts/OpenDyslexic128.h"


namespace rckid {
    class Clock : public ui::App<void> {
    public:

        String name() const override { return "Clock"; }

        Clock() : ui::App<void>{} {
            h_ = g_.addChild(new ui::Label{Rect::XYWH(0, 30, 150, 130), ""});
            h_->setFont(Font::fromROM<assets::OpenDyslexic128>());
            h_->setHAlign(HAlign::Right);
            m_ = g_.addChild(new ui::Label{Rect::XYWH(170, 30, 150, 130), ""});
            m_->setFont(Font::fromROM<assets::OpenDyslexic128>());
            m_->setHAlign(HAlign::Left);
            colon_ = g_.addChild(new ui::Label{Rect::XYWH(150, 30, 20, 130), ":"});
            colon_->setFont(Font::fromROM<assets::OpenDyslexic128>());
            contextMenu_.add(ui::ActionMenu::Item("Set time"));
            contextMenu_.add(ui::ActionMenu::Item("Set date"));
            contextMenu_.add(ui::ActionMenu::Item("Set alarm"));
        }

        void update() override {
            ui::App<void>::update();
            
            TinyDateTime t = timeNow();
            h_->setText(STR(fillLeft(t.hour(), 2, '0')));
            m_->setText(STR(fillLeft(t.minute(), 2, '0')));
            colon_->setVisible(t.second() & 1);

            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                exit();

            if (btnPressed(Btn::Select)) {
                App::run<PopupMenu<ui::Action>>(&contextMenu_);
                /*
                auto action = PopupMenu::show(&contextMenu_);
                if (action.has_value()) {
                    switch (action.value()) {
                        case 0: // Set time
                            UNIMPLEMENTED;
                            break;
                        case 1: // Set date
                            UNIMPLEMENTED;
                            break;
                        case 2: // Set alarm
                            UNIMPLEMENTED;
                            break;
                        default:
                            UNREACHABLE;
                    }
                }
                */
            }
        }

        /** There is no harm checking the time.
         */
        bool isBudgeted() const override { return false; }

    private:
        ui::Label * h_;
        ui::Label * m_;
        ui::Label * colon_;
        ui::ActionMenu contextMenu_; 

    }; // rckid::Clock

} // namespace rckid