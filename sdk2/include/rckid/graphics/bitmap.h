#pragma once

#include <rckid/memory.h>
#include <rckid/graphics/color.h>

#include <rckid/graphics/image_source.h>

namespace rckid {

    /** Bitmap representation
     */
    class Bitmap {
    public:

        Bitmap(Coord w, Coord h, Color::Representation colorRep, mutable_ptr<uint8_t> pixels, mutable_ptr<Color::RGB565> palette = nullptr) :
            w_{w}, h_{h}, colorRepresentation_{colorRep}, pixels_{std::move(pixels)}, palette_{std::move(palette)} {
        }

        Bitmap(Coord w, Coord h, Color::Representation colorRep) :
            w_{w}, h_{h}, colorRepresentation_{colorRep} {
            // TODO create new pixel data 
            // TODO use standard palette? 
        }

        Coord width() const { return w_; }

        Coord height() const { return h_;}

        Color::Representation colorRepresentation() const { return colorRepresentation_; }

        uint32_t bpp() const { return colorRepresentationBpp(colorRepresentation_); }

        Color::RGB565 const * palette() const { return palette_.ptr(); }

        void setPalette(mutable_ptr<Color::RGB565> palette) {
            palette_ = std::move(palette);
        }

        uint8_t * pixels() { return pixels_.mut(); }

        uint16_t getPixel(Coord x, Coord y) const {
            return Color::getPixel(colorRepresentation_, pixels_.ptr(), w_, h_, x, y);
        }

        void setPixel(Coord x, Coord y, uint16_t color) {
            Color::setPixel(colorRepresentation_, pixels_.mut(), w_, h_, x, y, color);
        }

    private:
        Coord const w_;
        Coord const h_;
        Color::Representation const colorRepresentation_;
        mutable_ptr<uint8_t> pixels_;
        mutable_ptr<Color::RGB565> palette_;
    }; 


} // namespace rckid