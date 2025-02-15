#pragma once

#include "../graphics/bitmap.h"

#include "timer.h"
#include "image.h"
#include "label.h"

namespace rckid::ui {

    /** The carousel is used to display a rolling menu.
     
        Has two sprites and two element texts, and uses the renderColumn rendering to display them at appropriate parts of the screen. 
     */
    class Carousel : public Widget {
    public:

        void moveLeft() {

        }

        void moveRight() {

        }

        void moveUp() {

        }

        void moveDown() {
            
        }



        void update() override {

        }

    protected:
        void renderColumn(Coord column, Pixel * buffer, Coord starty, Coord numPixels) override {
        }
    

    private:
        Timer t_;

    }; // rckid::ui::Carousel

} // namespace rckid::ui