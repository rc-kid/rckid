#pragma once

#include "../ui/form.h"
#include "../ui/label.h"
#include "../ui/image.h"
#include "../ui/carousel.h"
#include "../ui/menu.h"
#include "../ui/header.h"
#include "../assets/icons_default_64.h"
#include "../assets/fonts/OpenDyslexic64.h"
#include "../assets/images.h"


namespace rckid {

    /** Main menu application.
     
        The app is responsible for displaying the app launcher menu as well as header and basic user information.

        The app uses background image, carousel menu. can also show extra stuff, like clock, alarm, lives, messages, etc. So maybe just start with simple carousel with 
     */
    class MainMenu : public ui::App<ui::Menu::Action> {
    public:
        MainMenu(ui::Menu::Generator generator) : ui::App<ui::Menu::Action>{} {
            using namespace ui;
            c_ = g_.addChild(new ui::CarouselMenu{});
            c_->setRect(Rect::XYWH(0, 160, 320, 80));
            c_->setFont(Font::fromROM<assets::OpenDyslexic64>());
            if (history_ == nullptr) {
                generator_ = generator;
                c_->setMenu(generator_(), Direction::Up);
            } else {
                historyPop(Direction::Up);
            }
        }

        static std::optional<ui::Menu::Action> run(ui::Menu::Generator generator) {
            MainMenu menu{generator};
            return menu.run();
        }

        using ui::App<ui::Menu::Action>::run;

    protected:
        void update() override {
            ui::App<ui::Menu::Action>::update();
            c_->processEvents();
            // see if an item has been selected
            if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                ui::Menu::Item * item = c_->currentItem();
                if (item == nullptr)
                    return;
                switch (item->kind()) {
                    case ui::Menu::SubmenuItem::KIND: {
                        historyPush();
                        auto m = item->as<ui::Menu::SubmenuItem>();
                        generator_ = m->generator();
                        c_->setMenu(generator_(), Direction::Up);
                        break;
                    }
                    case ui::Menu::ActionItem::KIND:
                        // record where we are
                        historyPush();
                        // and exit, if item selected
                        exit(item->as<ui::Menu::ActionItem>()->action());
                        break;
                    default:
                        UNREACHABLE;
                        break;
                }
            }
            if (btnPressed(Btn::B) || btnPressed(Btn::Down)) {
                if (history_ != nullptr) {
                    historyPop();
                } else {
                    // TODO some error would be nice 
                }
            }
        }

        void historyPush() {
            history_ = new (Arena::alloc<PreviousMenu>()) PreviousMenu{c_->index(), generator_, history_};
        }

        void historyPop(Direction transition = Direction::Down) {
            if (history_ == nullptr)
                return;
            auto * h = history_;
            history_ = h->previous;
            generator_ = h->generator;
            c_->setMenu(generator_(), transition, h->index);
            // since we are the only ones in the arena and the menu histories are the only thing being stored there, we must be able to free the latest history
            bool ok = Arena::tryFree(h);
            ASSERT(ok);
        }

    private:
        ui::Menu::Generator generator_;
        ui::CarouselMenu * c_;

        // action item to be returned
        ui::Menu::Action result_;

        class PreviousMenu {
        public:
            PreviousMenu(uint32_t index, ui::Menu::Generator generator, PreviousMenu * previous) : index{index}, generator{generator}, previous{previous} {}

            uint32_t index;
            ui::Menu::Generator generator;
            PreviousMenu * previous;
        };

        static inline PreviousMenu * history_ = nullptr;

    }; // rckid::MainMenu


} // namespace rckid