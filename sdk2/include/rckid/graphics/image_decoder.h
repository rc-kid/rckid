#pragma once

#include <rckid/graphics/image_source.h>
#include <rckid/graphics/bitmap.h>

namespace rckid {

    class ImageDecoder {
    public:
        virtual Coord width() const = 0;
        virtual Coord height() const = 0;
        virtual Color::Representation colorRepresentation() const = 0;
        virtual Bitmap decode() = 0;
    }; 

} // namespace rckid