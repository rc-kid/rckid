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
        // AC if plugged in
        if (dcPower() || true) {
            x -= fSym.glyphInfoFor(assets::glyph::PowerCord).advanceX + 4;
            surface.text(x, y - 7, fSym, charging() ? color::Blue : color::Gray) << assets::glyph::PowerCord;
        }

        // draw the battery gauge and optional level
        if (verbose) {
            std::string pct = STR(batteryLevel() << "%");
            x -= f.textWidth(pct);
            surface.text(x, y + 1, f, color::LightGray) << pct;
        } 
        x -= fSym.glyphInfoFor(assets::glyph::Battery100).advanceX + 6;
        auto b = getBatteryInfo(batteryLevel());
        surface.text(x, y - 7, fSym, b.first) << b.second;

        // see if we have headphones connected
        if (audioHeadphones() || true) {
            x-= fSym.glyphInfoFor(assets::glyph::Headphones).advanceX + 6;
            surface.text(x, y - 6, fSym, color::Blue) << assets::glyph::Headphones;
        }
        // draw the audio state and volume
        if (verbose) {
            std::string pct = STR(audioVolume << "%");
            x -= f.textWidth(pct);
            surface.text(x, y + 1, f, color::LightGray) << pct; 
        }
        auto v = getVolumeInfo(audioVolume());
        x -= fSym.glyphInfoFor(v.second).advanceX;
        surface.text(x, y - 7, fSym, v.first) << v.second;
        // if SD card has not been found, show its error sign
        if (sdCapacity() == 0 || true) {
            x -= fSym.glyphInfoFor(assets::glyph::SDCard).advanceX;
            surface.text(x, y - 7, fSym, color::Red) << assets::glyph::SDCard;
        }
        // TODO custom system tray icons
    }

    std::pair<ColorRGB, char> Header::getBatteryInfo(unsigned level) {
        if (level >= 80)
            return std::make_pair(color::Green, assets::glyph::Battery100);
        else if (level >=60)
            return std::make_pair(color::Green, assets::glyph::Battery75);
        else if (level >=40)
            return std::make_pair(color::Orange, assets::glyph::Battery50);
        else if (level >=20)
            return std::make_pair(color::Orange, assets::glyph::Battery25);
        else
            return std::make_pair(color::Red, assets::glyph::Battery0);
    }

    std::pair<ColorRGB, char> Header::getVolumeInfo(unsigned vol) {
        if (vol >= 66)
            return std::make_pair(color::Red, assets::glyph::VolumeHigh);
        else if (vol >= 33)
            return std::make_pair(color::Green, assets::glyph::VolumeMid);
        else if (vol > 0)
            return std::make_pair(color::DarkGreen, assets::glyph::VolumeLow);
        else 
            return std::make_pair(color::DarkGray, assets::glyph::SpeakerOff);
    }

}