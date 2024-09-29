#include "../assets/fonts/Symbols16.h"
#include "../assets/fonts/Iosevka16.h"
#include "../assets/glyphs.h"
#include "../app.h"

#include "header.h"

namespace rckid {

    void Header::drawOn(Surface<ColorRGB> & surface, bool verbose) {
        Font fSym = Font::fromROM<assets::font::Symbols16>();
        Font f = Font::fromROM<assets::font::Iosevka16>();
        surface.text(0,0, f, color::White) << App::fps() << " " << App::drawUs();

        int y = 0;
        int x = surface.width();
        x = 160;
        // AC if plugged in
        if (dcPower()) {
            //x -= fSym.glyphInfoFor(assets::glyph::DC).advanceX + 4;
            surface.text(x, y, fSym, charging() ? color::Blue : color::Gray) << " 1234567890" << assets::glyph::DC;
            //surface.putChar(Point{x, y}, fSym, assets::glyph::DC, charging() ? color::Blue : color::Gray);
        }
    }
}