#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/carousel.h>
#include <rckid/ui/menu.h>

namespace rckid {

    inline unique_ptr<ui::Menu> mainMenuGenerator() {
        auto result = std::make_unique<ui::Menu>();
        (*result)
            << ui::MenuItem{"Steps Counter", assets::icons_64::footprint, []() {
                Steps{}.run();
            }}
            << ui::MenuItem::Generator("More Apps", assets::icons_64::footprint, mainMenuGenerator)
            << ui::MenuItem{"Steps Two", assets::icons_64::footprint, []() {
                Steps{}.run();
            }};
        return result;
     }


    /** App launchaer (main menu)
     
        This is the first app that automatically runs when RCKid SDK built cartridges boot up. It is responsible for showing the main menu and launching the selected apps. 
     */
    class Launcher : public ui::App<void> {
    public:

        Launcher() {
            carousel_ = addChild(new ui::CarouselMenu())
                << ui::SetRect(Rect::XYWH(0, 140, 320, 100))
                << ui::SetMenu(mainMenuGenerator(), Direction::Up);
            state_ = new State(mainMenuGenerator, nullptr);
        }

        ~Launcher() override {
            // TODO delete states
        }

    private:

        void loop() override {
            ui::App<void>::loop();
            if (btnPressed(Btn::Left))
                carousel_->moveLeft();
            if (btnPressed(Btn::Right))
                carousel_->moveRight();
            if (btnPressed(Btn::A) || btnPressed(Btn::Up))
                moveUp();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                moveDown();
        }

        void moveUp() {
            auto item = carousel_->currentItem();
            // nothing to process in empty menu
            if (item == nullptr)
                return;
            // if the item is action, do it
            if (item->isAction()) {
                item->action()();
            // if the item is generator, update current state index and start a new one according to the generator
            } else {
                ASSERT(state_ != nullptr);
                state_->index = carousel_->index();
                state_ = new State(item->generator(), state_);
                carousel_->setMenu(item->generator()(), Direction::Up);
            }
        }

        void moveDown() {
            ASSERT(state_ != nullptr);
            // do not do anything if there is nowhere to go
            if (state_->previous == nullptr)
                return; 
            // move to next state and delete the current one
            State * prevState = state_;
            state_ = state_->previous;
            delete prevState;
            // update the menu according to the new state
            carousel_->setMenu(state_->generator(), Direction::Down, state_->index);
        }

        class State {
        public:
            uint32_t index = 0;
            ui::MenuItem::GeneratorEvent generator;
            State * previous;

            State(ui::MenuItem::GeneratorEvent generator, State * previous): 
                generator{std::move(generator)}, previous{previous} {}
        }; 

        State * state_ = nullptr;

        ui::CarouselMenu * carousel_ = nullptr;

    }; // rckid::Launcher

} // namespace rckid