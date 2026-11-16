#include <kiddo/graphics/image_decoder.h>
#include <kiddo/graphics/bitmap.h>

namespace kiddo {

    Bitmap::Bitmap(ImageSource && src) {
        // if the source is empty, do nothing
        if (src.empty())
            return;
        std::unique_ptr<ImageDecoder> decoder = src.toDecoder();
        if (decoder)
            *this = decoder->decode();
    }

} // namespace kiddo