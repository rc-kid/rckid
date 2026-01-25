#pragma once

#include <rckid/memory.h>
#include <rckid/graphics/color.h>

namespace rckid {

    /** Bitmap representation
     */
    class Bitmap {
    public:

        Bitmap(Coord w, Coord h, Color::Representation colorRep, mutable_ptr<uint16_t> pixels) :
            w_{w}, h_{h}, colorRepresentation_{colorRep}, pixels_{std::move(pixels)} {
        }

        Coord width() const { return w_; }

        Coord height() const { return h_;}

        Color::Representation colorRepresentation() const { return colorRepresentation_; }

        uint32_t bpp() const { return colorRepresentationBpp(colorRepresentation_); }

    private:
        Coord const w_;
        Coord const h_;
        Color::Representation const colorRepresentation_;
        mutable_ptr<uint16_t> pixels_;
    }; 


} // namespace rckid