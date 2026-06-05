#include <rckid/graphics/image_source.h>
#include <rckid/graphics/image_decoder.h>
#include <rckid/graphics/png.h>

namespace rckid {

    unique_ptr<RandomReadStream> ImageSource::toStream() {
        if (empty())   
            return nullptr;
        if (type() == Type::Memory) {
            // create memory stream from this data
            unique_ptr<RandomReadStream> result{new MemoryReadStream{std::move(data_)}};
            // if the data is not in immutable memory, release the ownership. 
            if (data_ == nullptr)
                invalidate();
            return result;
        } else {
            // we know the data pointer actually holds the path to the file (null-terminated) and we need to create a string from it.
            Type t = type();
            String path{data_.reinterpretAs<char>()};
            // the reinterpretation may transfer ownership, so we must invalidate if that is the case            
            if (! hal::memory::isImmutableDataPtr(path.c_str()))
                invalidate();
            return fs::readFile(path, t == Type::SD ? fs::Drive::SD : fs::Drive::Cartridge);
        }
    }

    unique_ptr<ImageDecoder> ImageSource::toDecoder() {
        if (empty())   
            return nullptr;
        switch (getImageType()) {
            case ImageType::PNG:
                return std::make_unique<PNGImageDecoder>(std::move(*this));
                break;
            case ImageType::JPG:
            case ImageType::QOI:
                UNIMPLEMENTED;
                break;
            case ImageType::RawMemory:
                return std::make_unique<RawBitmapDecoder>(std::move(*this));
                break;
            case ImageType::Unknown:
                return nullptr;
            default:
                UNREACHABLE;
        }
    }

    ImageSource::ImageType ImageSource::getImageType() const {
        ASSERT(!empty());
        // data is in memory, this becomes simple
        if (type() == Type::Memory) {
            uint8_t const * d = data_.get();
            if (d[0] == 0x89 && d[1] == 0x50) {
                return ImageType::PNG;
            } else if (d[0] == 0xff && d[1] == 0xd8) {
                return ImageType::JPG;
            } else if (d[0] == 0x71 && d[1] == 0x6f) {
                return ImageType::QOI;
            } else if (RawBitmapDecoder::verify(d, data_.size())) {
                return ImageType::RawMemory;
            }
        } else {
            String ext = fs::ext(path());
            if (ext == "png" || ext == "PNG")
                return ImageType::PNG;
            if (ext == "jpg" || ext == "jpeg")
                return ImageType::JPG;
            if (ext == "qoi")
                return ImageType::QOI;
            if (ext == "raw")
                return ImageType::RawMemory;
        }
        return ImageType::Unknown;
    }

} // namespace rckid