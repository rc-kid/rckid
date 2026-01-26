#pragma once

#include <rckid/graphics/image_source.h>
#include <rckid/graphics/bitmap.h>

namespace rckid {

    /** Interface for image decoders. 
     
        An image decoder is expected to be initialized with ImageSource containing the image data in its format. The decoder then provides methods to query the basic image properties as well as generating the corresponding Bitmap object. To determine which ImageDecoder implementation to use, the ImageSource::getImageType() static method can be used for the image types supported by the SDK, but custom decoders can be implemented as well.
     */
    class ImageDecoder {
    public:
        virtual ~ImageDecoder() = default;

        virtual Coord width() const = 0;
        virtual Coord height() const = 0;
        virtual Color::Representation colorRepresentation() const = 0;
        virtual Bitmap decode() = 0;
    }; 

    /** Simple & resource efficient raw bitmap image decoder.
     
        The raw bitmap decoder is extremely simple image decoder that expects the image source to point directly to pixel data in RGB565 format followed by two uint16_t values corresponding to the width and height of the bitmap data respectively.

        This layout means that when creating the Bitmap, the decode can simply decode the width & height and pass the pixel data pointer directly to the bitmap without any copying orr transformation. This is extremely useful if the iage data is stored in flash memory as a binary asset, since the memory cost for those images at runtime is virtually nonexistent.
     
        TODO can this use other color representations that RGB565? 
     */
    class RawBitmapDecoder : public ImageDecoder {
    public:

        RawBitmapDecoder(ImageSource && src) {
            ASSERT(src.good());
            if (src.type() == ImageSource::Type::Memory) {
                uint32_t dataSize = src.size();
                data_ = mutable_ptr<uint8_t>{(src.releaseData().release()), dataSize};
                ASSERT(dataSize == (width() * height() + 2) * sizeof(uint16_t));
            } else {
                // TODO do we want to support file based raw bitmaps?
                UNIMPLEMENTED;
            }
        }

        Coord width() const override { 
            ASSERT(good());
            return static_cast<Coord>(reinterpret_cast<uint16_t const *>(data_.ptr())[data_.count() / sizeof(uint16_t) - 2]);
        }

        Coord height() const override { 
            ASSERT(good());
            return static_cast<Coord>(reinterpret_cast<uint16_t const *>(data_.ptr())[data_.count() / sizeof(uint16_t) - 1]);
        }

        Color::Representation colorRepresentation() const override {
            return Color::Representation::RGB565;
        }

        Bitmap decode() override {
            ASSERT(good());
            return Bitmap{width(), height(), colorRepresentation(), std::move(data_)};
        }

        bool good() const {
            return data_.ptr() != nullptr;
        }

    private:
        mutable_ptr<uint8_t> data_;
    }; // RawBitmapDecoder

} // namespace rckid