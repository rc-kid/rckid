#include <rckid/graphics/image_source.h>

namespace rckid {

    unique_ptr<RandomReadStream> ImageSource::toStream() {
        /*
        ASSERT(good());
        if (type() == Type::Memory) {
            auto dataPtr = data_.releasePtr();
            uint32_t dataSize = size_;
            size_ = 0;
            return make_unique<MemoryReadStream>(dataPtr, dataSize);
        } else {
            String path{reinterpret_cast<char const *>(data_.ptr())};
            return fs::openFileRead(path, (type() == Type::SD) ? fs::Drive::SD : fs::Drive::Cartridge);
        }
            */
        UNIMPLEMENTED;
    }



} // namespace rckid