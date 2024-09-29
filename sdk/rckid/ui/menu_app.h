#pragma once

#include "../app.h"
#include "../graphics/canvas.h"

#include "../assets/fonts/OpenDyslexic48.h"

#include "header.h"
#include "menu.h"
#include "carousel.h"

namespace rckid {


    template<Menu (*GENERATOR)()>
    void menu() {

    }


    /* Menu application. 

       TODO should really be carousel app

       - carousel for menus 
       - when not enough memory, either swap to SD card or use some simple overhead menu
       
     */
    class MenuApp : public GraphicsApp<Canvas<ColorRGB>> {
    public:

        typedef Menu * (*MenuGenerator)();
        typedef void (*Action)();

        template<uint32_t SIZE>
        static MenuItem * Submenu(char const * text, uint8_t const (&buffer)[SIZE], MenuGenerator generator) {
            uintptr_t x = reinterpret_cast<uintptr_t>(generator) + 1;
            return new StaticMenuItem(text, buffer, SIZE, reinterpret_cast<void*>(x));
        }

        template<uint32_t SIZE>
        static MenuItem * Item(char const * text, uint8_t const (&buffer)[SIZE], Action action) {
            return new StaticMenuItem(text, buffer, SIZE, reinterpret_cast<void*>(action));
        }

        MenuApp(): 
            GraphicsApp{Canvas<ColorRGB>{320, 240}} {
            carousel_ = new Carousel{Font::fromROM<assets::font::OpenDyslexic48>()}; 
        }

        static void run(MenuGenerator menuGenerator) {
            if (singleton_ == nullptr)
                singleton_ = new MenuApp{};
            i_ = 0;
            menu_ = menuGenerator();
            carousel_->moveUp((*menu_)[i_]);
            while (true) {
                singleton_->loop();
                // based on current index, we either return from the function, call submenu, or perform the action
                if (i_ == -1) {
                    delete menu_;
                    return;
                }
                uintptr_t x = reinterpret_cast<uintptr_t>((*menu_)[i_].payload());
                // if the LSB is set, we are dealing with a submenu, remember current index, delete the menu and call itself with the generator 
                if (x & 1) {
                    x = x - 1;
                    delete menu_;
                    int oldIndex = i_;
                    run(reinterpret_cast<MenuGenerator>(x));
                    // when we come back, regenerate the menu and set index accordingly
                    i_ = oldIndex;
                    menu_ = menuGenerator();
                    carousel_->moveDown((*menu_)[i_]);
                // it's an action - run the action, but leave the memory area first
                } else {
                    int oldIndex = i_;
                    memoryLeaveArena();
                    memoryEnterArena();
                    reinterpret_cast<Action>(x)();
                    memoryLeaveArena();
                    memoryEnterArena();
                    singleton_ = new MenuApp{};
                    i_ = oldIndex;
                    menu_ = menuGenerator();
                    carousel_->moveUp((*menu_)[i_]);
                }
            }
        }

    protected:


        void onFocus() override {
        }

        void update() override {
            if (carousel_->idle()) {
                if (btnDown(Btn::Left))
                    moveLeft();
                if (btnDown(Btn::Right))
                    moveRight();
                if (btnDown(Btn::Up) || btnDown(Btn::A))
                    exit();
                if (btnDown(Btn::Down) || btnDown(Btn::B)) {
                    i_ = -1; // to signify we are leaving the menu
                    exit();
                }
            }
        }

        void draw() {
            g_.fill();
            carousel_->drawOn(g_, Rect::XYWH(0, 160, 320, 80));
            Header::drawOn(g_);
        }

    private:

        void moveLeft() {
            if (--i_ < 0)
                i_ = menu_->size() - 1;
            carousel_->moveLeft((*menu_)[i_]);
        }

        void moveRight() {
            if (++i_ >= static_cast<int>(menu_->size()))
                i_ = 0;
            carousel_->moveRight((*menu_)[i_]);
        }

        static inline MenuApp * singleton_ = nullptr;
        static inline Carousel * carousel_;
        static inline Menu * menu_;
        static inline int i_;


    }; // MenuApp

}