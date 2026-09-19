#pragma once

#include <rckid/ui/app.h>
#include <rckid/ui/carousel.h>
#include <rckid/ui/menu.h>


namespace rckid {

    /** Launcher menu. 
     
        Launcher menus are normal menus (collections of menu items), but enhanced with an ability to generate menu specific overlays. The overlay is a ui::Widget that is displayed on top of the menu and can be used to show additional information.
     */
    class LauncherMenu : public ui::Menu {
    public:
        using Extender = std::function<unique_ptr<LauncherMenu>(unique_ptr<LauncherMenu>)>;
        
        ui::Widget * overlay() const { return overlay_.get(); }

        void setOverlay(unique_ptr<ui::Widget> overlay) {
            overlay_ = std::move(overlay);
        }

    protected:
        unique_ptr<ui::Widget> overlay_ = nullptr;
    };

    struct MainMenuOptions {
        LauncherMenu::Extender gamesExtender = nullptr;
        LauncherMenu::Extender cartridgeExtender = nullptr;
    };


    ui::MenuItem::GeneratorEvent mainMenuGenerator(MainMenuOptions options = {});

    /** Generator for the games submenu.
     */
    unique_ptr<LauncherMenu> gamesMenuGenerator();

    /** Utilities submenu generator. 
     */
    unique_ptr<LauncherMenu> utilitiesMenuGenerator();

    /** Settings menu generator. 
     */
    unique_ptr<LauncherMenu> settingsMenuGenerator();

    /** Settings menu generator. 
     */
    unique_ptr<LauncherMenu> debugMenuGenerator();

    class Launcher2 : public ui::App<void> {
    public:

        String name() const override { return "Launcher"; }

        Launcher2(ui::MenuItem::GeneratorEvent rootMenuGenerator = mainMenuGenerator()) {
            root_.applyStyle();
            overlay_ = addChild(new ui::NonOwningWrapper())
                << ui::SetRect(Rect::XYWH(0, 0, 320, 240));
            carousel_ = addChild(new ui::Carousel())
                << ui::SetRect(Rect::XYWH(0, 140, 320, 100));
            enterMenu(std::move(rootMenuGenerator));
        }

        void releaseResources() override {
            root_.releaseResources();
            ui::App<void>::releaseResources();
            menu_.releaseResources();
        }

    protected:

        void onFocus() override {
            ui::App<void>::onFocus();
            if (! menu_.populated())
                menu_.populate();
        }

        void onBlur() override {
            ui::App<void>::onBlur();
        }

        void loop() override {
            ui::App<void>::loop();
            if (btnPressed(Btn::Left))
                moveLeft();
            if (btnPressed(Btn::Right))
                moveRight();  
            if (btnPressed(Btn::Up))
                moveUp();
            if (btnPressed(Btn::Down))
                moveDown();
            if (overlay_->contents() != nullptr)
                overlay_->contents()->processEvents();
        }

        void enterMenu(ui::MenuItem::GeneratorEvent generator) {
            if (!carousel_->idle())
                carousel_->cancelAnimations();
            menu_.openNew(std::move(generator));
            menu_.populate();
            if (menu_.menu()->empty()) {
                carousel_->setEmpty(Direction::Up);
            } else {
                ui::MenuItem const * mi = menu_.currentItem();
                carousel_->set(mi->text,mi->icon, Direction::Up);
            }
        }

        void moveLeft() {
            if (menu_.empty() || menu_.menu()->empty())
                return;
            if (!carousel_->idle())
                carousel_->cancelAnimations();
            menu_.setIndex((menu_.index() + menu_.menu()->size() - 1) % menu_.menu()->size());
            ui::MenuItem const * mi = menu_.currentItem();
            carousel_->set(mi->text,mi->icon, Direction::Left);            
        }

        void moveRight() {
            if (menu_.empty() || menu_.menu()->empty())
                return;
            if (!carousel_->idle())
                carousel_->cancelAnimations();
            menu_.setIndex((menu_.index() + 1) % menu_.menu()->size());
            ui::MenuItem const * mi = menu_.currentItem();
            carousel_->set(mi->text,mi->icon, Direction::Right);            
        }

        void moveUp() {
            if (menu_.empty() || menu_.menu()->empty())
                return;
            if (!carousel_->idle())
                carousel_->cancelAnimations();
            ui::MenuItem const * mi = menu_.currentItem();
            if (mi->isAction()) {
                carousel_->animate()
                    << ui::FlyOut(carousel_, Point{0, 100});
                waitUntilIdle(carousel_);
                mi->action()();
                carousel_->animate()
                    << ui::FlyOut(carousel_, Point{0, -100});
                waitUntilIdle(carousel_);
            } else {
                enterMenu(mi->generator());
                updateOverlayWidget();
            }
        }

        void moveDown() {
            if (menu_.parent() != nullptr) {
                if (!carousel_->idle())
                    carousel_->cancelAnimations();
                menu_.pop();
                updateOverlayWidget();
                ui::MenuItem const * mi = menu_.currentItem();
                carousel_->set(mi->text,mi->icon, Direction::Down);
            }
        }

        void updateOverlayWidget() {
            ASSERT(menu_.populated());
            overlay_->setContents(nullptr);
            ui::Menu::Context * ctx = & menu_;
            while (ctx != nullptr && overlay_->contents() == nullptr) {
                // TODO this should be checked cast
                LauncherMenu * lm = static_cast<LauncherMenu *>(ctx->menu());
                ASSERT(lm != nullptr);
                overlay_->setContents(lm->overlay());
                ctx = ctx->parent();
            }
        }

        /** The launcher's home menu has no elements as exitting the app is not possible and launcher has no capabilities on its own.
         */
        unique_ptr<ui::Menu> homeMenu() {
            return std::make_unique<ui::Menu>();
        }

        ui::Menu::Context menu_;

        ui::Carousel * carousel_ = nullptr;
        ui::NonOwningWrapper * overlay_ = nullptr;

    }; // rckid::Launcher

    /** App launcher (main menu)
     
        This is the first app that automatically runs when RCKid SDK built cartridges boot up. It is responsible for showing the main menu and launching the selected apps. 
     */
    class Launcher : public ui::App<void> {
    public:

        /** Tells the launcher that given action should also result in moving the menu down one level.
         
            This is useful for menu items that are just selections from a set of options.
         */
        static constexpr uint32_t PAYLOAD_MOVE_DOWN = 12345678;

        String name() const override { return "Launcher"; }

        Launcher(ui::MenuItem::GeneratorEvent rootMenuGenerator = mainMenuGenerator()) {
            ASSERT(instance_ == nullptr);
            instance_ = this;
            root_.applyStyle();
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

        ui::CarouselMenu * carousel() const { return carousel_; }

        static Launcher * instance() { return instance_; }
        
        class BorrowedCarousel : public ui::Widget {
        public:
            BorrowedCarousel() {
                ASSERT(Launcher::instance_ != nullptr);
                // if the carousel is already borrowed, we can't simply take it - instead create new carousel that we'll use temporarily
                if (Launcher::instance_->carouselBorrowed_) {
                    ownedCarousel_ = unique_ptr<ui::CarouselMenu>{new ui::CarouselMenu{}};
                    ownedCarousel_->setRect(Rect::XYWH(0, 140, 320, 100));
                    carousel_ = ownedCarousel_.get();
                } else {
                    carousel_ = Launcher::instance_->carousel_;
                    Launcher::instance_->carouselBorrowed_ = true;
                }
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
            void setEmpty() { carousel_->setEmpty(); }
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
                carousel_->processEvents();
            }

        private:
            ui::CarouselMenu * carousel_;
            ui::CarouselMenu::Context const * root_;
            unique_ptr<ui::CarouselMenu> ownedCarousel_;
        }; // rckid::Launcher::BorrowedCarousel

    protected:

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
                if (item->payload == PAYLOAD_MOVE_DOWN)
                    carousel_->moveDown();
                // redecorate the current item in case there has been no menu / app animations
                else
                    carousel_->setItem(carousel_->index());
            }
            if (btnPressed(Btn::Start))
                ASSERT(false);
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