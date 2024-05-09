#pragma once

#include "rckid/rckid.h"
#include "rckid/graphics/font.h"

#include "assets/fonts/Iosevka_16.h"
#include "assets/fonts/Iosevka_20.h"
#include "assets/fonts/SymbolsNF_20.h"

namespace rckid {

    /** A very basic RCKid header that can render itself into given bitmap. 
     
        The header has the following format:

          87%
     */
    template<typename COLOR>
    class Header {
    public:

        /** Draws the header to the given bitmap. 
         */
        void drawOn(Bitmap<COLOR> & bitmap, Rect where) {
            int y = where.top();
            int x = where.right();
            // AC if plugged in
            if (dcPower()) {
                x -= SymbolsNF_20.glyphInfoFor(glyph::DC).advanceX + 4;
                bitmap.putChar(Point{x, y}, SymbolsNF_20, glyph::DC, COLOR::White());
            }
            // draw the battery gauge and optional level
            x -= SymbolsNF_20.glyphInfoFor(glyph::Battery100).advanceX + 4;
            auto b = getBatteryInfo();
            bitmap.putChar(Point{x, y}, SymbolsNF_20, b.second, b.first);
                

            bitmap.text(where.left(), where.top() + 2, Iosevka_16, COLOR::White()) <<
                "fps: " << stats::fps() << "d: " << stats::drawUs();
        }

    private:

        
        std::pair<COLOR, char> getBatteryInfo() {
            unsigned batt = batteryLevel();
            if (batt >= 80)
                return std::make_pair(COLOR::Green(), glyph::Battery100);
            else if (batt >=60)
                return std::make_pair(COLOR::Green(), glyph::Battery75);
            else if (batt >=40)
                return std::make_pair(COLOR::Green(), glyph::Battery50);
            else if (batt >=60)
                return std::make_pair(COLOR::Red(), glyph::Battery25);
            else
                return std::make_pair(COLOR::Red(), glyph::Battery0);
        }

        bool verbose_ = true;




    }; // rckid::Header

}; // namespace rckid