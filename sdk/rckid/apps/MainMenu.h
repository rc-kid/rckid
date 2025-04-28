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
    class MainMenu : public ui::App {
    public:
        MainMenu(ui::Menu::SubmenuItem::Generator generator) : ui::App{} {
            using namespace ui;
            // TODO when called again, but with active history, ignore the generator and go from history instead
            m_ = generator();
            generator_ = generator;
    
            bg_ = new ui::Image{Bitmap<ColorRGB>{PNG::fromBuffer(assets::star)}};
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
            hdr_ = new ui::Header{};
            g_.add(hdr_);
        }

    protected:
        void update() override {
            ui::App::update();
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
                        c_->setMenu(m->generator()(), ui::Carousel::Transition::Up);
                        break;
                    }
                    case ui::Menu::ActionItem::KIND:
                        // do nothing for now, exit the menu app in the future
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
                        case Carousel::Transition::Up:
                            bg_->setImgY(imgY_ + interpolation::cosine(t, 0, 60).round());
                            break;
                        case Carousel::Transition::Down:
                            bg_->setImgY(imgY_ - interpolation::cosine(t, 0, 60).round());
                            break;
                        default:
                            UNIMPLEMENTED;
                            break;
                    }
                    break;
            }
        }

        void historyPush() {
            history_ = new PreviousMenu{c_->index(), generator_, history_};
        }

        void historyPop() {
            if (history_ == nullptr)
                return;
            auto * h = history_;
            history_ = h->previous;
            c_->setMenu(h->generator(), ui::Carousel::Transition::Down, h->index);
            Heap::tryFree(h);
        }

    private:
        ui::Menu * m_;
        ui::Menu::SubmenuItem::Generator generator_;
        ui::CarouselMenu * c_;
        ui::Image * bg_;
        ui::Header * hdr_;
        Coord imgX_;
        Coord imgY_; 


        class PreviousMenu {
        public:
            PreviousMenu(uint32_t index, ui::Menu::SubmenuItem::Generator generator, PreviousMenu * previous) : index{index}, generator{generator}, previous{previous} {}

            uint32_t index;
            ui::Menu::SubmenuItem::Generator generator;
            PreviousMenu * previous;
        };

        static inline PreviousMenu * history_ = nullptr;

    }; // rckid::MainMenu


} // namespace rckid