#pragma once

#include <variant>

#include "../ui/form.h"
#include "../ui/label.h"
#include "../ui/image.h"
#include "../ui/carousel.h"
#include "../ui/menu.h"
#include "../ui/header.h"
#include "../assets/icons_64.h"
#include "../assets/fonts/OpenDyslexic64.h"
#include "../assets/images.h"


namespace rckid {

    struct MainMenuGameLauncher {
        String file;
    };

    using MainMenuPayload = std::variant<
        ui::Menu::Generator, 
        ui::Menu::Action,
        MainMenuGameLauncher
    >;


    /** Main menu application.
     
        The app is responsible for displaying the app launcher menu as well as header and basic user information.

        The app uses background image, carousel menu. can also show extra stuff, like clock, alarm, lives, messages, etc. So maybe just start with simple carousel with 
     */
    class MainMenu : public ui::App<MainMenuPayload> {
    public:

        class Item : public ui::Menu::Item {
        public:

            Item(String text, Icon icon, ui::Menu::Action action) :
                ui::Menu::Item{std::move(text), std::move(icon)},
                payload{std::move(action)} {
            }

            Item(String text, Icon icon, ui::Menu::Generator generator) :
                ui::Menu::Item{std::move(text), std::move(icon)},
                payload{std::move(generator)} {
            }

            Item(String text, Icon icon, MainMenuGameLauncher gameLauncher) :
                ui::Menu::Item{std::move(text), std::move(icon)},
                payload{std::move(gameLauncher)} {
            }

            MainMenuPayload payload;
        }; // rckid::MainMenu::Item

        static Item * Action(String text, Icon icon, ui::Menu::Action action) {
            return new Item{text, std::move(icon), action};
        }

        static Item * Submenu(String text, Icon icon, ui::Menu::Generator generator) {
            return new Item{text, std::move(icon), generator};
        }

        static Item * GameLauncher(String text, Icon icon, String gameFile) {
            return new Item{text, std::move(icon), MainMenuGameLauncher{std::move(gameFile)}};
        }

        MainMenu(ui::Menu::Generator generator) : ui::App<MainMenuPayload>{} {
            using namespace ui;
            c_ = g_.addChild(new ui::CarouselMenu{});
            c_->setRect(Rect::XYWH(0, 160, 320, 80));
            c_->setFont(Font::fromROM<assets::OpenDyslexic64>());
            if (history_ == nullptr) {
                generator_ = generator;
                c_->setMenu(generator_());
                c_->setItem(0, Direction::Up);
                //c_->setMenu(generator_(), Direction::Up);
            } else {
                historyPop(Direction::Up);
            }
        }

        static std::optional<MainMenuPayload> run(ui::Menu::Generator generator) {
            MainMenu menu{generator};
            return menu.run();
        }

        using ui::App<MainMenuPayload>::run;

    protected:
        void update() override {
            ui::App<MainMenuPayload>::update();
            c_->processEvents();
            // see if an item has been selected
            if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                Item * item = reinterpret_cast<Item *>(c_->currentItem());
                if (item == nullptr)
                    return;
                if (std::holds_alternative<ui::Menu::Generator>(item->payload)) {
                    historyPush();
                    generator_ = std::get<ui::Menu::Generator>(item->payload);;
                    c_->setMenu(generator_());
                    c_->setItem(0, Direction::Up);
                } else {
                    // record where we are
                    historyPush();
                    // and exit, if item selected
                    select(item->payload);
                }
                /*
                switch (item->kind()) {
                    case ui::Menu::SubmenuItem::KIND: {
                        historyPush();
                        auto m = item->as<ui::Menu::SubmenuItem>();
                        generator_ = m->generator();
                        c_->setMenu(generator_());
                        c_->setItem(0, Direction::Up);
                        break;
                    }
                    case ui::Menu::ActionItem::KIND:
                        // record where we are
                        historyPush();
                        // and exit, if item selected
                        select(item->as<ui::Menu::ActionItem>()->action());
                        break;
                    default:
                        UNREACHABLE;
                        break;
                } */
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
            history_ = new (Arena::alloc<PreviousMenu>()) PreviousMenu{c_->currentIndex(), generator_, history_};
        }

        void historyPop(Direction transition = Direction::Down) {
            if (history_ == nullptr)
                return;
            auto * h = history_;
            history_ = h->previous;
            generator_ = h->generator;
            c_->setMenu(generator_());
            c_->setItem(h->index, transition);
            // since we are the only ones in the arena and the menu histories are the only thing being stored there, we must be able to free the latest history
            bool ok = Arena::tryFree(h);
            ASSERT(ok);
        }

    private:

        ui::Menu::Generator generator_;
        ui::CarouselMenu * c_;

        // action item to be returned
        //Payload result_;

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