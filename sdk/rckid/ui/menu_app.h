#pragma once

#include "../app.h"
#include "../graphics/canvas.h"

#include "../assets/fonts/OpenDyslexic48.h"

#include "header.h"
#include "menu.h"
#include "carousel.h"
#include "../filesystem.h"

#include "text_input.h"

namespace rckid {

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
            void * x = reinterpret_cast<void*>(generator);
            TRACE_MENU_APP("Created submenu " << text << ", payload " << x);
            return new StaticMenuItem(text, buffer, SIZE, ITEM_KIND_SUBMENU, x);
        }

        template<uint32_t SIZE>
        static MenuItem * Item(char const * text, uint8_t const (&buffer)[SIZE], Action action) {
            void * x = reinterpret_cast<void*>(action);
            TRACE_MENU_APP("Created menu item " << text << ", payload " << x);
            return new StaticMenuItem(text, buffer, SIZE, ITEM_KIND_ACTION, x);
        }

        MenuApp(): 
            GraphicsApp{Canvas<ColorRGB>{320, 240}} {
            carousel_ = new Carousel{Font::fromROM<assets::font::OpenDyslexic48>()}; 
            rckid::ledSetEffects(
                RGBEffect::Rainbow(0, 1, 1, 32), 
                RGBEffect::Rainbow(51, 1, 1, 32), 
                RGBEffect::Rainbow(102, 1, 1, 32), 
                RGBEffect::Rainbow(153, 1, 1, 32), 
                RGBEffect::Rainbow(204, 1, 1, 32)
            );
        }

        static void run(MenuGenerator menuGenerator) {
            if (singleton_ == nullptr)
                singleton_ = ARENA(new MenuApp{});
            i_ = 0;
            menu_ = menuGenerator();
            TRACE_MENU_APP("Generated menu - items: " << menu_->size());
            carousel_->moveUp((*menu_)[i_]);
            while (true) {
                TRACE_MENU_APP("Entering app loop");
                singleton_->loop();
                // based on current index, we either return from the function, call submenu, or perform the action
                if (i_ == -1) {
                    TRACE_MENU_APP("Leaving menu level");
                    delete menu_;
                    return;
                }
                TRACE_MENU_APP("Selected submenu " << i_ << ", payload " << (*menu_)[i_].payload());
                uint32_t itemKind = (*menu_)[i_].payload();
                void * payloadPtr = ((*menu_)[i_].payloadPtr());
                // if the LSB is set, we are dealing with a submenu, remember current index, delete the menu and call itself with the generator 
                if (itemKind == ITEM_KIND_SUBMENU) {
                    TRACE_MENU_APP("Entering submenu");
                    delete menu_;
                    int oldIndex = i_;
                    run(reinterpret_cast<MenuGenerator>(payloadPtr));
                    TRACE_MENU_APP("Submenu done");
                    // when we come back, regenerate the menu and set index accordingly
                    i_ = oldIndex;
                    menu_ = menuGenerator();
                    carousel_->moveDown((*menu_)[i_]);
                // it's an action - run the action, but leave the memory area first
                } else {
                    TRACE_MENU_APP("Executing app");
                    int oldIndex = i_;
                    singleton_->onBlur();
                    delete menu_; // menu is heap allocated
                    Arena::leave();
                    Arena::enter();
                    TRACE_MEMORY("Heap allocated before app: " << Heap::allocated());
                    TRACE_MEMORY("Heap used befor eapp:      " << Heap::used());
                    reinterpret_cast<Action>(payloadPtr)();
                    TRACE_MEMORY("Heap allocated after app:  " << Heap::allocated());
                    TRACE_MEMORY("Heap used after eapp:      " << Heap::used());
                    TRACE_MENU_APP("App done");
                    rumbleNudge();
                    Arena::leave();
                    Arena::enter();
                    singleton_ = ARENA(new MenuApp{});
                    singleton_->onFocus();
                    i_ = oldIndex;
                    menu_ = menuGenerator();
                    carousel_->moveUp((*menu_)[i_]);
                }
            }
        }

    protected:


        void update() override {
            if (btnPressed(Btn::VolumeUp))
                audioSetVolume(audioVolume() + 1);
            if (btnPressed(Btn::VolumeDown))
                audioSetVolume(audioVolume() - 1);
            if (btnPressed(Btn::Select)) {
                //using namespace filesystem;
                //format(Drive::Cartridge);
                memoryFree();
                /*
                if (displayBrightness() == 0)
                    displaySetBrightness(128);
                else if (displayBrightness() == 128)
                    displaySetBrightness(255);
                else displaySetBrightness(0);
                */
            }
            if (carousel_->idle()) {
                if (btnDown(Btn::Left))
                    moveLeft();
                if (btnDown(Btn::Right))
                    moveRight();
                if (btnDown(Btn::Up) || btnDown(Btn::A)) {
                    rumbleNudge();
                    exit();
                }
                if (btnDown(Btn::Down) || btnDown(Btn::B)) {
                    i_ = -1; // to signify we are leaving the menu
                    exit();
                    rumbleNudge();                    
                }
                // TODO this should be handled by the base app in future versions
                if (btnPressed(Btn::Select))
                    ledSetEffect(Btn::Left, RGBEffect::Solid(16, 0, 0, 1));
                if (btnPressed(Btn::Start))
                    ledSetEffect(Btn::Left, RGBEffect::Solid(0, 16, 0, 1));
            }
        }

        void draw() {
            NewArenaScope _{};
            g_.fill();
            carousel_->drawOn(g_, Rect::XYWH(0, 160, 320, 80));
            Header::drawOn(g_);
        }

    private:

        // an app to be executed (with trasition effects)
        static constexpr uint32_t ITEM_KIND_ACTION = 0;
        // submenu with generator
        static constexpr uint32_t ITEM_KIND_SUBMENU = 1;

        void moveLeft() {
            if (--i_ < 0)
                i_ = menu_->size() - 1;
            carousel_->moveLeft((*menu_)[i_]);
            rumbleNudge();
        }

        void moveRight() {
            if (++i_ >= static_cast<int>(menu_->size()))
                i_ = 0;
            carousel_->moveRight((*menu_)[i_]);
            rumbleNudge();
        }

        static inline MenuApp * singleton_ = nullptr;
        static inline Carousel * carousel_;
        static inline Menu * menu_;
        static inline int i_;


    }; // MenuApp

}