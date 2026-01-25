#pragma once

#include <rckid/memory.h>
#include <rckid/graphics/color.h>

namespace rckid {

    /** Bitmap representation
     */
    class Bitmap {
    public:

        Coord width() const { return w_; }

        Coord height() const { return h_;}

        Color::Representation colorRepresentation() const { return colorRepresentation_; }

        uint32_t bpp() const { return colorRepresentationBpp(colorRepresentation_); }

    private:
        Coord const w_;
        Coord const h_;
        Color::Representation const colorRepresentation_;
        mutable_ptr<uint16_t> * pixels_;
    }; 


} // namespace rckid