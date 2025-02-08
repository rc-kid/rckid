#pragma once

#include "../graphics/bitmap.h"

namespace rckid {

    /** The carousel is used to display a rolling menu.
     
        Has two sprites and two element texts, and uses the renderColumn rendering to display them at appropriate parts of the screen. 
     */
    class Carousel {
    public:

        /** Renders the specified carousel column onto given buffer. 
         */
        void renderColumn(Coord x, ColorRGB * buffer) const {

            //icon_.renderColumn(x, buffer, palette, paletteOffset);
            //iconNext_.renderColumn(x + icon_.width(), buffer, palette, paletteOffset);
        }

    private:
        Bitmap<ColorRGB> icon_;
        Bitmap<ColorRGB> iconNext_;

    }; // rckid::Carousel

} // namespace rckid