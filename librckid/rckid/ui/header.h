#pragma once

#include "rckid/rckid.h"
#include "rckid/fs/sd.h"
#include "rckid/graphics/font.h"

#include "assets/fonts/Iosevka_16.h"
#include "assets/fonts/SymbolsNF_16.h"
#include "assets/fonts/SymbolsNF_20.h"



#include "rckid/audio/tone.h"
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
                bitmap.putChar(Point{x, y + 2}, SymbolsNF_16, glyph::DC, charging() ? COLOR::Blue() : COLOR::Gray());
            }
            // draw the battery gauge and optional level
            if (verbose_) {
                std::string pct = STR(batteryLevel() << "%");
                x -= Iosevka_16.textWidth(pct) + 8;
                bitmap.text(x, y + 2, Iosevka_16, COLOR::LightGray()) << pct;
            } else {
                x -= 4; // extra space from the flash
            }
            x -= SymbolsNF_20.glyphInfoFor(glyph::Battery100).advanceX + 4;
            auto b = getBatteryInfo();
            bitmap.putChar(Point{x, y + 1}, SymbolsNF_20, b.second, b.first);
            // see if we have headphones connected
            if (headphonesActive()) {
                x-= SymbolsNF_20.glyphInfoFor(glyph::Headphones).advanceX + 4;
                bitmap.putChar(Point{x, y}, SymbolsNF_20, glyph::Headphones, COLOR::Blue());
            }
            // draw the audio state and volume
            if (verbose_) {
                std::string pct = STR(audioVolume() << "%");
                x -= Iosevka_16.textWidth(pct) + 8;
                bitmap.text(x, y + 2, Iosevka_16, COLOR::LightGray()) << pct;
            }
            auto v = getVolumeInfo();
            x -= SymbolsNF_20.glyphInfoFor(v.second).advanceX + 4;
            bitmap.putChar(Point{x, y + 1}, SymbolsNF_20, v.second, v.first);
            // if SD card has not been found, show its error sign
            switch (SD::status()) {
                case SD::Status::NotPresent:
                    x -= SymbolsNF_20.glyphInfoFor(glyph::SDCard).advanceX + 4;
                    bitmap.putChar(Point{x, y + 1}, SymbolsNF_20, glyph::SDCard, COLOR::Red());
                    break;
                case SD::Status::Unrecognized:
                    x -= SymbolsNF_20.glyphInfoFor(glyph::SDCard).advanceX + 4;
                    bitmap.putChar(Point{x, y + 1}, SymbolsNF_20, glyph::SDCard, COLOR::Orange());
                    break;
                case SD::Status::USB:
                    x -= SymbolsNF_20.glyphInfoFor(glyph::USB).advanceX + 4;
                    bitmap.putChar(Point{x, y + 1}, SymbolsNF_20, glyph::USB, COLOR::White());
                    break;
                default:
                    break;
            }

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

        std::pair<COLOR, char> getVolumeInfo() {
            unsigned vol = audioVolume();
            if (vol >= 66)
                return std::make_pair(COLOR::Green(), glyph::VolumeHigh);
            else if (vol >= 33)
                return std::make_pair(COLOR::Green(), glyph::VolumeMid);
            else if (vol > 0)
                return std::make_pair(COLOR::DarkGreen(), glyph::VolumeLow);
            else 
                return std::make_pair(COLOR::Red(), glyph::SpeakerOff);
        }

        bool verbose_ = true;

    }; // rckid::Header

}; // namespace rckid