#pragma once

#include "../graphics/bitmap.h"

namespace rckid {

    /** Header & status bar 
     
        
     */
    class Header {
    public:

        /** Draws the header on given bitmap. 
         */
        static void drawOn(Bitmap<ColorRGB> & surface, bool verbose = true);

    private:

        static std::pair<ColorRGB, char> getBatteryInfo(unsigned lebel);
        static std::pair<ColorRGB, char> getVolumeInfo(unsigned lebel);

    }; // rckid::Header


} // namespace rckid