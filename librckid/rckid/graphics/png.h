#pragma once

#include <functional>

#include <PNGdec.h>

#include "color.h"

namespace rckid {

    /** PNG image wrapper. 
     */
    class PNG : private PNGIMAGE  {
    public:

        using DecodeCallback = std::function<void(ColorRGB * rgb, int lineNum, int lineWidth)>;

        static PNG fromBuffer(uint8_t const * buffer, size_t numBytes);

        template<size_t SIZE>
        static PNG fromBuffer(uint8_t const (&buffer)[SIZE]) { return fromBuffer(buffer, SIZE); }

        /*
        static PNG fromFile(char const * file) {

        }
        */

        int width() const { return iWidth; }

        int height() const { return iHeight; }

        int decode(DecodeCallback cb);

        PNG & operator == (PNG const &) = delete;
        PNG & operator == (PNG &&) = delete;

    private:

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
        PNG() {
            memset(this, 0, sizeof(PNGIMAGE));
        }
#pragma GCC diagnostic pop

        static void decodeLine_(PNGDRAW *pDraw);

        DecodeCallback cb_;

    }; // rckid::PNG

} // namespace rckid