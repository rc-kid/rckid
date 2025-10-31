#pragma once

#include <variant>

#include "../ui/form.h"
#include "../ui/label.h"
#include "../ui/image.h"
#include "../ui/carousel.h"
#include "../ui/menu.h"
#include "../ui/style.h"
#include "../ui/header.h"
#include "../assets/icons_64.h"
#include "../assets/icons_24.h"
#include "../assets/fonts/OpenDyslexic64.h"
#include "../assets/fonts/OpenDyslexic24.h"
#include "../assets/images.h"
#include "../pim.h"


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
            ui::Form<ui::Action>{}
        {
            g_.addChild(c_);
            for (uint32_t i = 0; i < 2; ++i) {
                g_.addChild(anniversaryIcons_[i]);
                g_.addChild(anniversaryLabels_[i]);
                anniversaryIcons_[i].setTransparentColor(ColorRGB::Black());
                anniversaryLabels_[i].setFont(Font::fromROM<assets::OpenDyslexic24>());
                anniversaryLabels_[i].setColor(ui::Style::fg());
            }

            c_.setRect(Rect::XYWH(0, 160, 320, 80));
            c_.setFont(Font::fromROM<assets::OpenDyslexic64>());
            if (history_ == nullptr)
                history_ = new ui::ActionMenu::HistoryItem{0, initialGenerator, nullptr};
            c_.attachHistory(history_);

            uint32_t bdayDays = 366;
            uint32_t holidayDays = 366;
            String bdayName;
            String holidayName;
            Icon holidayIcon;

            Contact::forEach([&](Contact c) {
                uint32_t days = c.daysTillBirthday();
                if (days < bdayDays) {
                    bdayDays = days;
                    bdayName = c.name;
                } else if (days == bdayDays) {
                    bdayName = STR(bdayName << ", " << c.name);
                }
            });

            Holiday::forEach([&](Holiday h) {
                uint32_t days = h.daysTillHoliday();
                if (days < holidayDays) {
                    holidayDays = days;
                    holidayName = h.name;
                    if (h.image != "")
                        holidayIcon = Icon{h.image};
                    else
                        holidayIcon = Icon{assets::icons_24::poo};
                } else if (days == holidayDays) {
                    holidayName = STR(holidayName << ", " << h.name);
                    holidayIcon = Icon{assets::icons_24::poo};
                }
            });

            if (bdayDays <= holidayDays) {
                if (bdayDays < 366) {
                    anniversaryIcons_[0] = Icon{assets::icons_24::birthday_cake};
                    anniversaryLabels_[0].setText(STR(bdayName << " (" << bdayDays << " days)"));
                } else {
                    anniversaryIcons_[0].setVisible(false);
                    anniversaryLabels_[0].setVisible(false);
                }
                if (holidayDays < 366) {
                    anniversaryIcons_[1] = holidayIcon;
                    anniversaryLabels_[1].setText(STR(holidayName << " (" << holidayDays << " days)"));
                } else {
                    anniversaryIcons_[1].setVisible(false);
                    anniversaryLabels_[1].setVisible(false);
                }
            } else {
                if (holidayDays < 366) {
                    anniversaryIcons_[0] = holidayIcon;
                    anniversaryLabels_[0].setText(STR(holidayName << " (" << holidayDays << " days)"));
                } else {
                    anniversaryIcons_[0].setVisible(false);
                    anniversaryLabels_[0].setVisible(false);
                }
                if (bdayDays < 366) {
                    anniversaryIcons_[1] = Icon{assets::icons_24::birthday_cake};
                    anniversaryLabels_[1].setText(STR(bdayName << " (" << bdayDays << " days)"));
                } else {
                    anniversaryIcons_[1].setVisible(false);
                    anniversaryLabels_[1].setVisible(false);
                }
            }
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

        ui::Image anniversaryIcons_[2]{ 
            ui::Image{8, 18, Icon{assets::icons_24::birthday_cake}},
            ui::Image{8, 50, Icon{assets::icons_24::birthday_cake}},
        };
        ui::Label anniversaryLabels_[2]{
            ui::Label{40, 18, ""},
            ui::Label{40, 50, ""},
        };
        //ui::Image bdayImg_{8, 18, Icon{assets::icons_24::birthday_cake}};
        //ui::Label bdayLabel_{40, 18, ""};

        static inline ui::ActionMenu::HistoryItem * history_ = nullptr;

    }; // MainMenu

} // namespace rckid