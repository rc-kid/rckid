#pragma once

#include <rckid/graphics/image_decoder.h>

// forward declarations of the png's internal structures
struct png_image_tag;
struct png_draw_tag;

namespace rckid {

    class PNGImageDecoder : public ImageDecoder {
    public:

        /** Creates PNG image decoder from ImageSource. 
         */
        PNGImageDecoder(ImageSource && src):
            PNGImageDecoder(src.toStream()) {
        }

        /** Creates PNG image decoder from stream.
         */
        PNGImageDecoder(unique_ptr<RandomReadStream> && stream);

        ~PNGImageDecoder() override;

        Coord width() const override;

        Coord height() const override;

        Color::Representation colorRepresentation() const override;

        Bitmap decode() override;

    private:

        static void decodeLine_(png_draw_tag * pDraw);
        
        png_image_tag * img_;

    }; // PNGImageDecoder

} // namespace rckid