#include <rckid/graphics/image_source.h>

namespace rckid {

    unique_ptr<RandomReadStream> ImageSource::toStream() {
        ASSERT(good());
        if (type() == Type::Memory) {
            uint8_t * ptr = const_cast<uint8_t*>(reinterpret_cast<uint8_t const *>(data_.get()));
            // create memory stream from this data
            unique_ptr<RandomReadStream> result{new MemoryStream{mutable_ptr<uint8_t>{ptr, size_}}};
            // if the data is not in immutable memory, release the ownership. 
            if (! hal::memory::isImmutableDataPtr(ptr))
                invalidate();
            return result;
        } else {
            // one more hack here - we know the data pointer actually holds the path to the file (null-terminated) and we need to create a string from it.
            char const * path = reinterpret_cast<char const *>(data_.get());
            // if the path is not in immutable memory, invalidate the ImageSource after creating the stream
            if (! hal::memory::isImmutableDataPtr(path))
                invalidate();
            String sourceFile{immutable_ptr<char>{path}};
            return fs::readFile(sourceFile, type() == Type::SD ? fs::Drive::SD : fs::Drive::Cartridge);
        }
    }

} // namespace rckid