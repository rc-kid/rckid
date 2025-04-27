#pragma once

#include "../ui/form.h"
#include "../ui/label.h"
#include "../ui/image.h"
#include "../ui/carousel.h"
#include "../ui/menu.h"
#include "../assets/icons_default_64.h"
#include "../assets/fonts/OpenDyslexic64.h"


namespace rckid {

    /** Main menu application.
     
        The app is responsible for displaying the app launcher menu as well as header and basic user information.

        The app uses background image, carousel menu. can also show extra stuff, like clock, alarm, lives, messages, etc. So maybe just start with simple carousel with 
     */
    class MainMenu : public ui::App {
    public:
        MainMenu() : ui::App{} {
            using namespace ui;
            m_ = new ui::Menu{
                new ui::Menu::ActionItem{"Action 1", assets::icons_default_64::animal_1, []() {  }},
                new ui::Menu::ActionItem{"Action 2", assets::icons_default_64::animal_2, []() {  }},
                new ui::Menu::ActionItem{"Action 3", assets::icons_default_64::animal_3, []() {  }},
            };
    
            bg_ = new ui::Image{Bitmap<ColorRGB>{PNG::fromBuffer(assets::icons_default_64::game_controller)}};
            bg_->setRect(Rect::WH(320, 240));
            bg_->setRepeat(true);
            c_ = new ui::CarouselMenu{m_};
            g_.add(bg_);
            g_.add(c_);
            c_->setRect(Rect::XYWH(0, 160, 320, 80));
            c_->setFont(Font::fromROM<assets::OpenDyslexic64>());
            c_->setOnTransitionEvent([this](Carousel::TransitionState state, Carousel::Transition transition, Timer & t) {
                onCarouselTransition(state, transition, t);
            });
        }

    protected:
        void update() override {
            ui::App::update();
            c_->processEvents();
        }

        void onCarouselTransition(ui::Carousel::TransitionState state, ui::Carousel::Transition transition, Timer & t) {
            using namespace ui;
            switch (state) {
                case Carousel::TransitionState::Start:
                    imgX_ = bg_->imgX();
                    imgY_ = bg_->imgY();
                    break;
                default:
                    // either end or in progress
                    switch (transition) {
                        case Carousel::Transition::Left:
                            bg_->setImgX(imgX_ + interpolation::cosine(t, 0, 80).round());
                            break;
                        case Carousel::Transition::Right:
                            bg_->setImgX(imgX_ - interpolation::cosine(t, 0, 80).round());
                            break;
                        default:
                            UNIMPLEMENTED;
                            break;
                    }
                    break;
            }
        }

    private:
        ui::Menu * m_;
        ui::CarouselMenu * c_;
        ui::Image * bg_;
        Coord imgX_;
        Coord imgY_; 

    }; // rckid::MainMenu


} // namespace rckid