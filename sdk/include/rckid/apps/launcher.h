#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/carousel.h>
#include <rckid/ui/menu.h>


namespace rckid {

    struct MainMenuOptions {
        ui::MenuExtender gamesExtender = nullptr;
    };

    ui::MenuItem::GeneratorEvent mainMenuGenerator(MainMenuOptions options = {});

    /** Generator for the games submenu.
     */
    unique_ptr<ui::Menu> gamesMenuGenerator();

    /** Utilities submenu generator. 
     */
    unique_ptr<ui::Menu> utilitiesMenuGenerator();

    /** Settings menu generator. 
     */
    unique_ptr<ui::Menu> settingsMenuGenerator();

    /** App launcher (main menu)
     
        This is the first app that automatically runs when RCKid SDK built cartridges boot up. It is responsible for showing the main menu and launching the selected apps. 
     */
    class Launcher : public ui::App<void> {
    public:

        String name() const override { return "Launcher"; }

        Launcher(ui::MenuItem::GeneratorEvent rootMenuGenerator = mainMenuGenerator()) {
            ASSERT(instance_ == nullptr);
            instance_ = this;
            root_.setBackgroundImage(ui::Style::defaultStyle());
            carousel_ = addChild(new ui::CarouselMenu())
                << ui::SetRect(Rect::XYWH(0, 140, 320, 100))
                << ui::ResetMenu(rootMenuGenerator);
        }

        ~Launcher() override {
            // detach from the instance
            ASSERT(instance_ == this);
            instance_ = nullptr;
        }

        void releaseResources() override {
            root_.releaseResources();
            ui::App<void>::releaseResources();
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

            bool idle() const override {
                return ui::Widget::idle() && carousel_->idle();
            }

            ui::Menu * menu() const { return carousel_->menu(); }
            uint32_t index() const { return carousel_->index(); }
            uint32_t prevIndex() const { return carousel_->prevIndex(); }
            uint32_t nextIndex() const { return carousel_->nextIndex(); }
            ui::CarouselMenu::Context const * context() const { return carousel_->context() == root_ ? nullptr : carousel_->context(); }
            bool empty() const { return carousel_->empty(); }
            ui::MenuItem * currentItem() const { return carousel_->currentItem(); }
            void setItem(uint32_t index) { carousel_->setItem(index); }
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

            void processEvents() override {
                if (btnPressed(Btn::Down) || btnPressed(Btn::B)) {
                    if (atRoot())
                        return;
                }
                triggerProcessedEvents(carousel_);
            }

        private:
            ui::CarouselMenu * carousel_;
            ui::CarouselMenu::Context const * root_;
        }; // rckid::Launcher::BorrowedCarousel

        /** Updates the style used by the launcher. Useful for style changes previews, etc.
         */
        static void updateStyle(ui::Style * style);

    private:

        void onFocus() override {
            ui::App<void>::onFocus();
            carouselBorrowed_ = false;
            focusWidget(carousel_);
            if (!launch_)
                return;
            carousel_->moveDown();
        }

        void onBlur() override {
            if (!launch_)
                return;
            if (! carouselBorrowed_) {
                carousel_->moveUp(nullptr);
                waitUntilIdle(carousel_);
            }
        }

        void loop() override {
            ui::App<void>::loop();
            if (btnPressed(Btn::A) || btnPressed(Btn::Up)) {
                auto item = carousel_->currentItem();
                ASSERT(item->isAction());
                ASSERT(launch_ == false);
                launch_ = true;
                item->action()();
                launch_ = false;
            }
        }

        /** The launcher's home menu has no elements as exitting the app is not possible and launcher has no capabilities on its own.
         */
        unique_ptr<ui::Menu> homeMenu() {
            return std::make_unique<ui::Menu>();
        }

        // the carousel itself
        ui::CarouselMenu * carousel_ = nullptr;

        // whether carousel has been borrowed by the next running app (this changes the animations and the way we reset the menu when the app exits)
        bool carouselBorrowed_ = false;
        // true if we are launching an app as user action (so that we play animations), otherwise no focus & blur animations are played (such as when home menu is shown)
        bool launch_ = false;

        // launcher instance
        static inline Launcher * instance_ = nullptr;

    }; // rckid::Launcher

} // namespace rckid