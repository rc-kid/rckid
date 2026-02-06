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
            carousel_ = addChild(new ui::Carousel())
                << ui::SetRect(Rect::XYWH(0, 140, 320, 100));

            
            carousel_->set("Empty", assets::icons_64::empty_box, Direction::Up);

        }


        ~Launcher() override {
            // TODO delete states
        }
    


    private:

        void loop() override {
            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                exit();
        }

        class State {
        public:
            uint32_t index;
            ui::MenuItem::GeneratorEvent generator;
            State * previous;

            State(uint32_t index, ui::MenuItem::GeneratorEvent generator, State * previous):
                index{index}, generator{std::move(generator)}, previous{previous} {}
        }; 

        State * state_ = nullptr;

        ui::Carousel * carousel_ = nullptr;

    }; // rckid::Launcher

} // namespace rckid