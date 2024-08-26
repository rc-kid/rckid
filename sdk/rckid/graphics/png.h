#pragma once

#include <PNGdec/src/PNGdec.h>

#include "../utils/stream.h"
#include "color.h"

namespace rckid {

    /** PNG image wrapper. 
     
        Reading the image is done in two phases - first the PNG instance with a stream containing the image data is created and enough information from the image is parsed so that we know the size of the image. Then when the actual decoding should hppen, the decode() method with actual decode callback should eb called. 

        The decode callback is called every time new row of the image is decoded and thus gives the caller the ability to process the image.
     */
    class PNG : private PNGIMAGE  {
    public:

        using DecodeCallback = std::function<void(ColorRGB * rgb, int lineNum, int lineWidth)>;

        static PNG fromStream(RandomReadStream & stream);

        static PNG fromBuffer(uint8_t const * buffer, uint32_t numBytes); 

        template<uint32_t SIZE>
        static PNG fromBuffer(uint8_t const (&buffer)[SIZE]) { return fromBuffer(buffer, SIZE); }


        int width() const { return iWidth; }

        int height() const { return iHeight; }

        int decode(DecodeCallback cb);

        PNG & operator == (PNG const &) = delete;
        PNG & operator == (PNG &&) = delete;

    private:

        PNG();

        static void decodeLine_(PNGDRAW *pDraw);

        DecodeCallback cb_;

    }; // rckid::PNG


} // namespace rckid