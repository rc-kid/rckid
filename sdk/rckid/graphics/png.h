#pragma once

#include "../utils/stream.h"
#include "../memory.h"
#include "color.h"

// forward declarations of the png's internal structures
struct png_image_tag;
struct png_draw_tag;

namespace rckid {

    /** PNG image wrapper. 
     
        Reading the image is done in two phases - first the PNG instance with a stream containing the image data is created and enough information from the image is parsed so that we know the size of the image. Then when the actual decoding should hppen, the decode() method with actual decode callback should eb called. 

        The decode callback is called every time new row of the image is decoded and thus gives the caller the ability to process the image.
     */
    class PNG {
    public:
        using DecodeCallback = std::function<void(ColorRGB * rgb, int lineNum, int lineWidth)>;

        static PNG fromStream(RandomReadStream & stream, Allocator & a = Heap::allocator());

        static PNG fromBuffer(uint8_t const * buffer, uint32_t numBytes, Allocator & a = Heap::allocator()); 

        template<uint32_t SIZE>
        static PNG fromBuffer(uint8_t const (&buffer)[SIZE], Allocator & a = Heap::allocator()) { return fromBuffer(buffer, SIZE, a); }

        int width() const;

        int height() const;

        int decode(DecodeCallback cb);

        PNG & operator == (PNG const &) = delete;
        PNG & operator == (PNG &&) = delete;

        ~PNG() {
            Heap::tryFree(img_);
            Heap::tryFree(line_);
        }

    private:

        PNG(Allocator & a);

        static void decodeLine_(png_draw_tag *pDraw);

        png_image_tag * img_;
        DecodeCallback cb_;
        uint16_t * line_;
    }; 
}