#pragma once

#include "../graphics/bitmap.h"

namespace rckid {

    /** Header & status bar 
     
        
     */
    class Header {
    public:

        /** Draws the header on given surface. 
         */
        static void drawOn(Surface<ColorRGB> & surface, bool verbose = true);

    }; // rckid::Header


} // namespace rckid