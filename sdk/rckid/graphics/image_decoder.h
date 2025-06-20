#pragma once

#include <functional>
#include "../rckid.h"

namespace rckid {

    /** Base class for image decoders. 
     
        Image decoder must support getting the image dimensions, bits per pixel, and palette (where applicable). 
     */
    class ImageDecoder {
    public:
        using DecodeCallback16 = std::function<void(uint16_t * rgb, int lineNum, int lineWidth)>;

        virtual ~ImageDecoder() = default;

        virtual Coord width() const = 0;
        virtual Coord height() const = 0;

        virtual uint32_t bpp() const = 0;

        virtual ColorRGB * palette() const = 0;

        virtual bool decode16(DecodeCallback16 cb) = 0;

    }; // rckid::Decoder

} // namespace rckid