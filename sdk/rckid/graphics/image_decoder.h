#pragma once

#include <functional>
#include "../rckid.h"

namespace rckid {

    /** Base class for image decoders. 
     
        Image decoder must support getting the image dimensions, bits per pixel, and palette (where applicable). 
     */
    class ImageDecoder {
    public:
        using DecodeCallbackRGB = std::function<void(uint16_t * rgb, int lineNum, int lineWidth)>;
        using DecodeCallback = std::function<void(uint16_t * pixels, int lineNum, int lineWidth)>;

        virtual ~ImageDecoder() = default;

        virtual Coord width() const = 0;
        virtual Coord height() const = 0;

        virtual uint32_t bpp() const = 0;

        virtual uint16_t * palette() const = 0;

        virtual bool decodeRGB(DecodeCallbackRGB cb) = 0;
        virtual bool decode(DecodeCallback cb) = 0;

    }; // rckid::Decoder

} // namespace rckid