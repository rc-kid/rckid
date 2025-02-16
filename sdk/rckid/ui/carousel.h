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

        void set(Image icon, std::string text) {
            set(std::move(icon), std::move(text), aImg_, aText_);
        }

        void moveLeft(Image && icon, std::string text) {

        }

        void moveRight(Image && icon, std::string text) {

        }

        void moveUp(Image && icon, std::string text) {

        }

        void moveDown(Image && icon, std::string text) {

        }



        void update() override {

        }

    protected:
        void renderColumn(Coord column, Pixel * buffer, Coord starty, Coord numPixels) override {
        }

        void set(Image && icon, std::string &&text, Image & imgInto, Label & labelInto) {
            imgInto = std::move(icon);
            labelInto.setText(text);
            // TODO position
        }
    

    private:
        Timer t_;
        Image aImg_;
        Label aText_;
        Image bImg_;
        Label bText_;

    }; // rckid::ui::Carousel

} // namespace rckid::ui