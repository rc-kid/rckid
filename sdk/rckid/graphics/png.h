#pragma once

#include <type_traits>

#include "../utils/stream.h"
#include "../memory.h"
#include "color.h"
#include "image_decoder.h"

// forward declarations of the png's internal structures
struct png_image_tag;
struct png_draw_tag;
struct pngenc_image_tag;


namespace rckid {

    /** PNG image wrapper. 
     
        Reading the image is done in two phases - first the PNG instance with a stream containing the image data is created and enough information from the image is parsed so that we know the size of the image. Then when the actual decoding should hppen, the decode() method with actual decode callback should eb called. 

        The decode callback is called every time new row of the image is decoded and thus gives the caller the ability to process the image.
     */
    class PNG : public ImageDecoder {
    public:

        PNG & operator = (PNG const &) = delete;
        PNG & operator = (PNG &&) = delete;

        static PNG fromStream(RandomReadStream * stream);

        template<typename STREAM>
        static typename std::enable_if<std::is_base_of<RandomReadStream, STREAM>::value, PNG>::type fromStream(STREAM && stream) {
            return fromStream(new STREAM{std::move(stream)});
        }

        static PNG fromBuffer(uint8_t const * buffer, uint32_t numBytes); 

        template<uint32_t SIZE>
        static PNG fromBuffer(uint8_t const (&buffer)[SIZE]) { return fromBuffer(buffer, SIZE); }

        Coord width() const override;

        Coord height() const override;

        uint32_t bpp() const override;

        uint16_t * palette() const override;

        bool decodeRGB(DecodeCallbackRGB cb) override;
        bool decode(DecodeCallback cb) override;


        ~PNG() override;

        PNG(PNG const &) = delete;
        
        PNG(PNG && other) noexcept: img_{other.img_}, line_{other.line_} {
            other.img_ = nullptr;
            other.line_ = nullptr;
        }

    private:

        struct DecodeRGB {
            DecodeCallbackRGB cb;
            PNG * png;

            DecodeRGB(DecodeCallbackRGB cb, PNG * png): 
                cb{cb}, png{png} { }
        };

        struct Decode {
            DecodeCallback cb;
            PNG * png;

            Decode(DecodeCallback cb, PNG * png):
                cb{cb}, png{png} { }
        };

        PNG();

        static void decodeLineRGB_(png_draw_tag *pDraw);
        static void decodeLine_(png_draw_tag *pDraw);

        png_image_tag * img_;
        uint16_t * line_;
    }; // rckid::PNG

    class PNGEncoder {
    public:
    
        PNGEncoder & operator = (PNGEncoder const &) = delete;
        PNGEncoder & operator = (PNGEncoder &&) = delete;

        static PNGEncoder fromStream(RandomReadWriteStream * stream);

        template<typename STREAM>
        static typename std::enable_if<std::is_base_of<RandomReadWriteStream, STREAM>::value, PNGEncoder>::type fromStream(STREAM && stream) {
            return fromStream(new STREAM{std::move(stream)});
        }

        /** Begins encoding of an image. Always uses 16bpp and hence no palette. 
         */
        bool beginEncode(Coord width, Coord height, uint8_t compressionLevel = 6);

        bool addLine(uint16_t * pixels);

        bool setTransparentColor(uint32_t color);

        ~PNGEncoder();

        PNGEncoder(PNGEncoder const &) = delete;

        PNGEncoder(PNGEncoder && other) noexcept: img_{other.img_} {
            other.img_ = nullptr;
        }

    private:
        PNGEncoder();

        pngenc_image_tag * img_;
        int linesEncoded_ = 0;

    }; // rckid::PNGEncoder

} // namespace rckid