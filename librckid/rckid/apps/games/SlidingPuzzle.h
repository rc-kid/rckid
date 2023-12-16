#pragma once

#include "rckid/app.h"
#include "rckid/graphics/framebuffer.h"
#include "rckid/graphics/png.h"

namespace rckid {

    /** A simple sliding puzzle game, silimar to FifteenPuzzle. 
     
        More of a demonstration of what can be done. 
     */
    class SlidingPuzzle : public App<Framebuffer<display_profile::RGB>> {
    public:

    protected:

        void onFocus() override {
            App::onFocus();
            PNG png = PNG::fromBuffer(defaultImage_, sizeof(defaultImage_));
            png.decode([&](ColorRGB * line, int lineNum, int lineWidth){
                Renderer & r = renderer();
                for (int i = 0; i < lineWidth; ++i)
                    r.pixel(i, lineNum, line[i]);
            });
        }

        void update() override {

        }

        void draw() override {

        }

    private:

        /** PNG image for the game. 
         
           from https://images6.fanpop.com/image/photos/43100000/Disney-Princesses-disney-princess-43157173-1500-1080.jpg 
         */
        static constexpr uint8_t defaultImage_[] = {
#include "SlidingPuzzle16.png.data"
        }; 

    }; 

} // namespace rckid