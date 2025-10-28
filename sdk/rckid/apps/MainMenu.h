#pragma once

#include <variant>

#include "../ui/form.h"
#include "../ui/label.h"
#include "../ui/image.h"
#include "../ui/carousel.h"
#include "../ui/menu.h"
#include "../ui/header.h"
#include "../assets/icons_64.h"
#include "../assets/icons_24.h"
#include "../assets/fonts/OpenDyslexic64.h"
#include "../assets/images.h"


namespace rckid {

    /** Main menu displays the apps available on the system and also shows kid's centered dashboard with next birthdays and events, etc.  
     */
    class MainMenu : public ui::Form<ui::Action> {
    public:

        String name() const override { return "MainMenu"; }

        String title() const override { 
            TinyDateTime t = timeNow();
            return STR(fillLeft(t.time.hour(), 2, '0') << (t.time.second() % 2 ? ':' : ' ') << fillLeft(t.time.minute(), 2, '0'));
        }

        MainMenu(ui::ActionMenu::MenuGenerator initialGenerator):
            ui::Form<ui::Action>{} {
            g_.addChild(c_);
            g_.addChild(bdayImg_);
            bdayImg_.setTransparentColor(ColorRGB::Black());
            c_.setRect(Rect::XYWH(0, 160, 320, 80));
            c_.setFont(Font::fromROM<assets::OpenDyslexic64>());
            if (history_ == nullptr)
                history_ = new ui::ActionMenu::HistoryItem{0, initialGenerator, nullptr};
            c_.attachHistory(history_);
        }

    protected:
        void focus() override {
            ui::Form<ui::Action>::focus();
        }

        void update() override {
            ui::Form<ui::Action>::update();
            if (!c_.processEvents()) {
                if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                    auto action = c_.currentItem();
                    ASSERT(action->isAction());
                    exit(action->action());
                    history_ = c_.detachHistory();
                }
            }
            if (btnPressed(Btn::Start)) {
                RAMHeap::traceChunks();
            }
        }

        /** Main menu does not support any extra home menu actions (including the exit). 
         */
        ui::ActionMenu::MenuGenerator homeMenuGenerator() override {
            return [](){ return new ui::ActionMenu{}; };
        }

    private:
       
        ui::CarouselMenu<ui::Action> c_;
        ui::Image bdayImg_{8, 18, Icon{assets::icons_24::birthday_cake}};

        static inline ui::ActionMenu::HistoryItem * history_ = nullptr;

    }; // MainMenu

} // namespace rckid