#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/carousel.h>
#include <rckid/ui/menu.h>


namespace rckid {

    unique_ptr<ui::Menu> mainMenuGenerator();

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

        class BorrowedCarousel : public ui::Widget {
        public:
            BorrowedCarousel() {
                ASSERT(Launcher::instance_ != nullptr);
                carousel_ = Launcher::instance_->carousel_;
                Launcher::instance_->carouselBorrowed_ = true;
                root_ = carousel_->context();
                setRect(carousel_->rect());
            }

            ui::Menu * menu() const { return carousel_->menu(); }
            uint32_t index() const { return carousel_->index(); }
            ui::CarouselMenu::Context const * context() const { return carousel_->context() == root_ ? nullptr : carousel_->context(); }
            bool empty() const { return carousel_->empty(); }
            ui::MenuItem * currentItem() const { return carousel_->currentItem(); }
            void resetMenu(ui::MenuItem::GeneratorEvent generator) {
                carousel_->clearContext(root_);
                carousel_->moveUp(std::move(generator));
                // TODO
            }
            void moveLeft() { 
                ASSERT(context() != nullptr);
                carousel_->moveLeft(); 
            }
            void moveRight() {
                ASSERT(context() != nullptr);
                carousel_->moveRight();
            }
            void moveUp(ui::MenuItem::GeneratorEvent generator) {
                ASSERT(context() != nullptr);
                carousel_->moveUp(std::move(generator));
            }

            void moveDown() {
                ASSERT(context() != nullptr);
                if (context()->previous == root_)
                    return;
                carousel_->moveDown();
            }

            bool atRoot() const {
                return carousel_->context()->previous == root_;
            }

            ui::CarouselMenu * borrowedCarousel() const { return carousel_; }

            void renderColumn(Coord column, Coord startRow, Color::RGB565 * buffer, Coord numPixels) override {
                // move the coordinates & stuff
                carousel_->renderColumn(column, startRow, buffer, numPixels);
            }

        protected:
            void onRender() override {
                Widget::onRender();
                triggerOnRender(carousel_);
            }

        private:
            ui::CarouselMenu * carousel_;
            ui::CarouselMenu::Context const * root_;
        }; // rckid::Launcher::BorrowedCarousel

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