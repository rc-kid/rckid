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
        using DecodeCallback16 = std::function<void(uint16_t * rgb, int lineNum, int lineWidth)>;

        static PNG fromStream(RandomReadStream & stream, Allocator & a = Heap::allocator());

        static PNG fromBuffer(uint8_t const * buffer, uint32_t numBytes, Allocator & a = Heap::allocator()); 

        template<uint32_t SIZE>
        static PNG fromBuffer(uint8_t const (&buffer)[SIZE], Allocator & a = Heap::allocator()) { return fromBuffer(buffer, SIZE, a); }

        int width() const;

        int height() const;

        int decode16(DecodeCallback16 cb, Allocator & a = Heap::allocator());

        PNG & operator = (PNG const &) = delete;
        PNG & operator = (PNG &&) = delete;

        ~PNG() {
            Heap::tryFree(img_);
        }

        PNG(PNG const &) = delete;
        
        PNG(PNG && other) noexcept: img_{other.img_} {
            other.img_ = nullptr;
        }

    private:

        struct Decode16 {
            DecodeCallback16 cb;
            PNG * png;
            uint16_t * line;

            Decode16(DecodeCallback16 cb, PNG * png, Allocator & a);
            ~Decode16();
        };

        PNG(Allocator & a);

        static void decodeLine16_(png_draw_tag *pDraw);

        png_image_tag * img_;


    }; 
}