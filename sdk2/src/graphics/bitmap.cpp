#include <rckid/graphics/image_decoder.h>
#include <rckid/graphics/bitmap.h>

namespace rckid {

    Bitmap::Bitmap(ImageSource && src) {
        ASSERT(src.good());
        std::unique_ptr<ImageDecoder> decoder = src.toDecoder();
        ASSERT(decoder);
        *this = decoder->decode();
    }

} // namespace rckid