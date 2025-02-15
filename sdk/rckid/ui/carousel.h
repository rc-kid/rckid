#pragma once

#include "../graphics/bitmap.h"

namespace rckid::ui {

    /** The carousel is used to display a rolling menu.
     
        Has two sprites and two element texts, and uses the renderColumn rendering to display them at appropriate parts of the screen. 
     */
    class Carousel {
    public:


    protected:
    
        /** Renders the specified carousel column onto given buffer. 
         */
        void renderColumn(Coord x, ColorRGB * buffer) const {

            //icon_.renderColumn(x, buffer, palette, paletteOffset);
            //iconNext_.renderColumn(x + icon_.width(), buffer, palette, paletteOffset);
        }

    private:

    Bitmap<ColorRGB> icon_;
        Bitmap<ColorRGB> iconNext_;

    }; // rckid::ui::Carousel

} // namespace rckid::ui