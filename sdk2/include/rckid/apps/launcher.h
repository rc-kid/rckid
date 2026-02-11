#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/carousel.h>
#include <rckid/ui/menu.h>


namespace rckid {

    /** Generator of the default main menu. 
     */
    unique_ptr<ui::Menu> mainMenuGenerator();

    /** Generator for the games submenu.
     */
    unique_ptr<ui::Menu> gamesMenuGenerator();

    /** Utilities submenu generator. 
     */
    unique_ptr<ui::Menu> utilitiesMenuGenerator();

    /** App launcher (main menu)
     
        This is the first app that automatically runs when RCKid SDK built cartridges boot up. It is responsible for showing the main menu and launching the selected apps. 
     */
    class Launcher : public ui::App<void> {
    public:

        String name() const override { return "Launcher"; }

        Launcher() {
            ASSERT(instance_ == nullptr);
            instance_ = this;
            root_.setBackgroundImage(ui::Style::defaultStyle());
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
                // TODO move the coordinates & stuff
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
            if (moveDownOnFocus_) {
                carousel_->moveDown();
                moveDownOnFocus_ = false;
            }
        }

        void onBlur() override {
            if (! carouselBorrowed_) {
                carousel_->moveUp(nullptr);
                moveDownOnFocus_ = true;
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

        /** The launcher's home menu has no elements as exitting the app is not possible and launcher has no capabilities on its own.
         */
        unique_ptr<ui::Menu> homeMenu() {
            return std::make_unique<ui::Menu>();
        }

        void moveUp() {
            auto item = carousel_->currentItem();
            // nothing to process in empty menu
            if (item == nullptr)
                return;
            // if the item is action, we are launching new app (fly in & out animations handled by onBlur and onFocus events where it is clear what animation to perform, if any)
            if (item->isAction()) {
                item->action()();
            // if the item is generator, update current state index and start a new one according to the generator
            } else {
                carousel_->moveUp(item->generator());
            }
        }

        // the carousel itself
        ui::CarouselMenu * carousel_ = nullptr;

        // whether carousel has been borrowed by the next running app (this changes the animations and the way we reset the menu when the app exits)
        bool carouselBorrowed_ = false;
        bool moveDownOnFocus_ = false;

        // launcher instance
        static inline Launcher * instance_ = nullptr;

    }; // rckid::Launcher

} // namespace rckid