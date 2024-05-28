#pragma once 

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "menu.h"
#include "header.h"
#include "carousel.h"
#include "assets/fonts/OpenDyslexic_48.h"

/*
#include "rckid/app.h"
#include "rckid/ui/header.h"
#include "rckid/ui/carousel.h"
*/


namespace rckid {

    /** Full screen menu.  

        The menu draws background, carousel in the bottom part of the screen and a header in the top part. The rest of the screen can be used for various other purposes, such as user information for the main menu, album information for music, etc. 
     */
    template<typename COLOR>
    class MenuApp : public App<FrameBuffer<COLOR>, void*> {
    public:

        /** Creates a menu app with given root menu. 
         */
        MenuApp(MenuStack & stack): carousel_{}, stack_{stack} {
        }

        /** Returns the current menu item.
         */
        MenuItem const & currentItem() {
            return stack_.top().menu->at(carousel_.current());
        }

    protected:

        using Super = App<FrameBuffer<COLOR>, void*>;
        using Super::exit;
        using Super::driver_;
        using typename Super::Color;

        void onFocus() override {
            Super::onFocus();
            // recreate the submenus
            stack_.reconstruct();
            MenuStack::Level & l = stack_.top();
            carousel_.setMenu(l.menu, l.index);
        }

        void onBlur() override {
            Super::onBlur();
            // to conserve memory, delete all non root menus (they can be recreated) - delete in the reverse order for easier last free block reclaim
            stack_.compress();
            leaveHeapArena();
        }

        void update() override {
            if (carousel_.idle() && carousel_.menu() == nullptr)
                exit();
            // left & right carousel movements
            if (down(Btn::Left))
                carousel_.prev();
            if (down(Btn::Right))
                carousel_.next();
            // select & up - if submenu, add to levels and enter, if not submenu, exit the app
            if (pressed(Btn::A) || pressed(Btn::Up)) {
                stack_.top().index = carousel_.current();
                MenuGenerator mg = currentItem().submenuGenerator();
                if (mg != nullptr) {
                    MenuStack::Level & l = stack_.push(MenuStack::Level{mg(), 0});
                    carousel_.setMenu(l.menu, l.index);
                } else {
                    exit(currentItem().payload());
                }
            }
            // back & down
            if (pressed(Btn::B) || pressed(Btn::Down)) {
                if (stack_.depth() > 0) {
                    MenuStack::Level & l = stack_.pop();
                    carousel_.setMenu(l.menu, l.index, /* reverse */ true);
                // exit the menu entirely, or do nothing if exitting is not allowed
                } else if (canGoBack_) {
                    // exit from the menu entirely
                    carousel_.setMenu(nullptr, 0, /* reverse */ true);
                }
            }
        }

        void draw() override {
            driver_.fill();
            header_.drawOn(driver_, Rect::WH(320, 20));
            driver_.setFont(OpenDyslexic_48);
            carousel_.drawOn(driver_, Rect::XYWH(0, 160, 320, 80));
        }

    private:

        Header<Color> header_;
        Carousel<Color> carousel_;

        MenuStack & stack_;

        bool canGoBack_ = false;

    }; // rckid::MenuApp

}