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
            bg_ = new ui::Image{Bitmap<ColorRGB>{PNG::fromBuffer(assets::star)}};
            bg_->setRect(Rect::WH(320, 240));
            bg_->setRepeat(true);
            c_ = new ui::CarouselMenu{};
            g_.add(bg_);
            g_.add(c_);
            c_->setRect(Rect::XYWH(0, 160, 320, 80));
            c_->setFont(Font::fromROM<assets::OpenDyslexic64>());
            c_->setOnTransitionEvent([this](Carousel::TransitionState state, Carousel::Transition transition, Timer & t) {
                if (initialTransition_) {
                    if (state == Carousel::TransitionState::End)
                        initialTransition_ = false;
                } else {
                    onCarouselTransition(state, transition, t);
                }
            });
            hdr_ = new ui::Header{};
            g_.add(hdr_);
            if (history_ == nullptr) {
                generator_ = generator;
                c_->setMenu(generator_(), ui::Carousel::Transition::Up);
            } else {
                historyPop(ui::Carousel::Transition::Up);
            }
        }

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
                        c_->setMenu(generator_(), ui::Carousel::Transition::Up);
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
            history_ = new (Arena::alloc<PreviousMenu>()) PreviousMenu{c_->index(), generator_, history_};
        }

        void historyPop(ui::Carousel::Transition transition = ui::Carousel::Transition::Down) {
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
        ui::Image * bg_;
        ui::Header * hdr_;
        Coord imgX_;
        Coord imgY_; 
        // when 
        bool initialTransition_ = true;

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