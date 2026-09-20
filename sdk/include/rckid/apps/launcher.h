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

        /** Item payload forcing a close of the current submenu. Useful when a submenu is a list of actions and once action is applied, the submenu should be closed.
         */
        static constexpr uint32_t CloseSubmenu = 0xffff0000;
        
        /** Triggers a menu refresh *after* the action is performed. Normally menus are refreshed only when coming back (context pop).
         */
        static constexpr uint32_t RefreshMenu = 0xffff0001;

        /** Disables animations for the crrent action. Useful for non-visual actions, such as decorator changes, etc. where no visible app is launched by the action. */
        static constexpr uint32_t NoAnimation = 0xffff0002;

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
                if (mi.payload != NoAnimation) {
                    carousel_->animate()
                        << ui::FlyOut(carousel_, Point{0, 100});
                    waitUntilIdle(carousel_);
                }
                mi.action()();
                switch (mi.payload) {
                    case CloseSubmenu:
                        carousel_->moveDown(false);
                        break;
                    case RefreshMenu:
                        carousel_->refresh();
                        break;
                }
                if (mi.payload != NoAnimation) {
                    carousel_->animate()
                        << ui::FlyOut(carousel_, Point{0, -100});
                    waitUntilIdle(carousel_);
                }
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