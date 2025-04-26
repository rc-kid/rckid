#pragma once

#include <functional>
#include "../rckid.h"

namespace rckid {

    class ImageDecoder {
    public:
        using DecodeCallback16 = std::function<void(uint16_t * rgb, int lineNum, int lineWidth)>;

        virtual ~ImageDecoder() = default;

        virtual Coord width() const = 0;
        virtual Coord height() const = 0;

        virtual bool decode16(DecodeCallback16 cb) = 0;

    }; // rckid::Decoder

} // namespace rckid