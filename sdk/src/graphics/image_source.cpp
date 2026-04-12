#include <rckid/graphics/image_source.h>
#include <rckid/graphics/image_decoder.h>
#include <rckid/graphics/png.h>

namespace rckid {

    unique_ptr<RandomReadStream> ImageSource::toStream() {
        if (empty())   
            return nullptr;
        if (type() == Type::Memory) {
            // create memory stream from this data
            unique_ptr<RandomReadStream> result{new MemoryReadStream{std::move(data_), size_}};
            // if the data is not in immutable memory, release the ownership. 
            if (data_ == nullptr)
                invalidate();
            return result;
        } else {
            // one more hack here - we know the data pointer actually holds the path to the file (null-terminated) and we need to create a string from it.
            Type t = type();
            char const * path = reinterpret_cast<char const *>(data_.get());
            // if the path is not in immutable memory, invalidate the ImageSource after creating the stream
            // NOTE this is not needed, but makes the path based image sources behave in the same way the memory based ones do, i.e. single use            
            if (! hal::memory::isImmutableDataPtr(path))
                invalidate();
            String sourceFile{immutable_ptr<char>{path}};
            return fs::readFile(sourceFile, t == Type::SD ? fs::Drive::SD : fs::Drive::Cartridge);
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
            } else if (RawBitmapDecoder::verify(d, size_)) {
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
        }
        return ImageType::Unknown;
    }

} // namespace rckid