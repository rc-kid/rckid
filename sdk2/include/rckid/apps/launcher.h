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
            ASSERT(instance_ == nullptr);
            instance_ = this;
            carousel_ = addChild(new ui::CarouselMenu())
                << ui::SetRect(Rect::XYWH(0, 140, 320, 100))
                << ui::ResetMenu(mainMenuGenerator);
        }

        ~Launcher() override {
            // detach from the instance
            ASSERT(instance_ == this);
            instance_ = nullptr;
        }

        ui::CarouselMenu * borrowCarousel() {
            ASSERT(carouselBorrowed_ == false); // can be borrowed only once
            return carousel_;
        }

    private:

        void onFocus() override {
            ui::App<void>::onFocus();
            carouselBorrowed_ = false;
        }

        void onBlur() override {
            if (! carouselBorrowed_) {
                carousel_->moveUp(emptyMenuGenerator);
                waitUntilIdle(carousel_);
            }
        }

        void loop() override {
            ui::App<void>::loop();
            if (btnPressed(Btn::Left))
                carousel_->moveLeft();
            if (btnPressed(Btn::Right))
                carousel_->moveRight();
            if (btnPressed(Btn::A) || btnPressed(Btn::Up))
                moveUp();
            if (btnPressed(Btn::B) || btnPressed(Btn::Down))
                carousel_->moveDown();
        }

        void moveUp() {
            auto item = carousel_->currentItem();
            // nothing to process in empty menu
            if (item == nullptr)
                return;
            // if the item is action, we are launching new app 
            if (item->isAction()) {
                //state_->index = carousel_->index();
                // TODO fly out desktop animations
                item->action()();
                carousel_->moveDown();
            // if the item is generator, update current state index and start a new one according to the generator
            } else {
                carousel_->moveUp(item->generator());
            }
        }

        ui::CarouselMenu * carousel_ = nullptr;
        bool carouselBorrowed_ = false;

        static inline Launcher * instance_ = nullptr;

        static unique_ptr<ui::Menu> emptyMenuGenerator() {
            auto result = std::make_unique<ui::Menu>();
            (*result)
                << ui::MenuItem{"", []() {
                    UNREACHABLE;
                }};
            return result;
        }

    }; // rckid::Launcher

} // namespace rckid