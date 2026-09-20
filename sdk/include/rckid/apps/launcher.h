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

    class Launcher : public ui::App<void> {
    public:

        String name() const override { return "Launcher"; }

        Launcher(ui::MenuItem::GeneratorEvent rootMenuGenerator = mainMenuGenerator()) {
            root_.applyStyle();
            overlay_ = addChild(new ui::NonOwningWrapper())
                << ui::SetRect(Rect::XYWH(0, 0, 320, 240));
            carousel_ = addChild(new ui::CarouselMenu())
                << ui::SetRect(Rect::XYWH(0, 140, 320, 100));


            carousel_->onMenuChange = [this] {
                updateOverlayWidget();
            };
            carousel_->onItemSelected = [this](ui::MenuItem const & mi) {
                carousel_->animate()
                    << ui::FlyOut(carousel_, Point{0, 100});
                waitUntilIdle(carousel_);
                mi.action()();
                carousel_->animate()
                    << ui::FlyOut(carousel_, Point{0, -100});
                waitUntilIdle(carousel_);
            };
            carousel_->enterMenu(std::move(rootMenuGenerator));
        }

        void releaseResources() override {
            root_.releaseResources();
            ui::App<void>::releaseResources();
            carousel_->menu().releaseResources();
        }

    protected:

        void onFocus() override {
            ui::App<void>::onFocus();
            if (! carousel_->menu().populated())
                carousel_->menu().populate();
        }

        void onBlur() override {
            ui::App<void>::onBlur();
        }

        void loop() override {
            ui::App<void>::loop();
            carousel_->processEvents();
            if (overlay_->contents() != nullptr)
                overlay_->contents()->processEvents();
        }

        void updateOverlayWidget() {
            ASSERT(carousel_->menu().populated());
            overlay_->setContents(nullptr);
            ui::Menu::Context * ctx = & carousel_->menu();
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

        ui::CarouselMenu * carousel_ = nullptr;
        ui::NonOwningWrapper * overlay_ = nullptr;

    }; // rckid::Launcher

} // namespace rckid