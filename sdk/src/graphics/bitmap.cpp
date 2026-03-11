#include <rckid/graphics/image_decoder.h>
#include <rckid/graphics/bitmap.h>

namespace rckid {

    Bitmap::Bitmap(ImageSource && src) {
        // if the source is empty, do nothing
        if (src.empty())
            return;
        std::unique_ptr<ImageDecoder> decoder = src.toDecoder();
        if (decoder)
            *this = decoder->decode();
    }

} // namespace rckid