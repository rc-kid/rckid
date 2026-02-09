#pragma once

#include <platform/writer.h>

#include <rckid/rckid.h>
#include <rckid/memory.h>
#include <rckid/stream.h>
#include <rckid/filesystem.h>

namespace rckid {

    class ImageDecoder;

    /** Describes the source of an image and provides its type decoding.
     
        Image source can be either pointer to region of memory containing the image data directly, or a path to the image file on either SD card, or cartridge.
     */
    class ImageSource {
    public:

        /** Empty image source (no image).
         */
        ImageSource() = default;

        /** Creates image source pointing to the given file and drive.
         
            This version should be used for loading images from files on either the SD card, or the cartridge filesystems.
         */
        ImageSource(String path, fs::Drive drive = fs::Drive::SD):
            size_{(drive == fs::Drive::SD) ? FILE_SD : FILE_CARTRIDGE} 
        {
            uint8_t const * pathData = reinterpret_cast<uint8_t const *>(path.release());
            data_ = immutable_ptr<uint8_t>{pathData};
        }

        /** Creates image source pointing to given data buffer in flash memory.
         
            The data buffer must contain the actual image data in the specified format and is asserted to exist in the flash memory (i.e. be immutable and not in main device RAM). This is the version that should be used for binary assets compiled into the firmware.
         */
        template<size_t SIZE>
        ImageSource(uint8_t const (& data)[SIZE]) :
            size_{static_cast<uint32_t>(SIZE)},
            data_{data} 
        {
            ASSERT(hal::memory::isImmutableDataPtr(data));
        }

        /** Creates  */
        template<uint32_t SIZE>
        ImageSource(uint16_t const (&data)[SIZE]) :
            size_{static_cast<uint32_t>(sizeof(data))},
            data_{reinterpret_cast<uint8_t const *>(data)} 
        {
            ASSERT(hal::memory::isImmutableDataPtr(data));
            ASSERT(SIZE == data[SIZE-1] * data[SIZE-2] + 2);
        }

        /** Creates image source pointing to given data buffer.
         
            The data buffer is owned by the ImageSource after this call and must contain the actual image data in the specified format. This is the least useable version that allows even dynamic data to be creates as the source of the image. 
         */
        ImageSource(mutable_ptr<uint8_t> data):
            size_{data.count()},
            data_{data.releasePtr()} {
        }

        /** Creates copy of the image source. 
         
            This is a trivial operation of the image source points to immutable data (either string, or data buffer). If the data is mutable, it is cloned first.
         */
        ImageSource(ImageSource const & from) {
            clone(from);
        }

        ImageSource & operator = (ImageSource const & other) {
            if (this == & other)
                return *this;
            clone(other);            
            return *this;
        }

        /** Type of the image source. 
         
            Can be either memory buffer (flash or RAM), or a file (SD card or cartridge stored).
         */
        enum class Type {
            Memory,
            SD,
            Cartridge,
        };

        /** Returns the type of the image source. 
         */
        Type type() const {
            ASSERT(good());
            if (size_ == FILE_SD)
                return Type::SD;
            else if (size_ == FILE_CARTRIDGE)
                return Type::Cartridge;
            else
                return Type::Memory;
        }


        /** Returns true if the image source is valid. 
         
            ImageSource is valid from its construction until it decays to a stream via the toStream() method. 
         */        
        bool good() const {
            return data_.get() != nullptr;
        }

        /** Decays the image source into a stream from which the image can be read.
         
            No futher operations on the ImageSource should be performed after this call. 
         */
        unique_ptr<RandomReadStream> toStream();

        /** Decays the image source to a corresponding decoder. 
         
            Returns nullptr if the image type is not supported.
         */
        unique_ptr<ImageDecoder> toDecoder();

        uint32_t size() const {
            ASSERT(type() == Type::Memory);
            return size_;
        }

        /** Returns pointer to the stored data.
         
            This is either the actual image data if kind is memory, or a string (null ternimated) path to the file on either SD card or cartridge.
         */
        uint8_t const * data() const {
            return data_.get();
        }

        /** Releases the data buffer held by the image source.
         */
        immutable_ptr<uint8_t> releaseData() {
            ASSERT(type() == Type::Memory);
            size_ = 0;
            return std::move(data_);
        }

        enum class ImageType {
            PNG,
            JPG,
            QOI,
            RawMemory,
            Unknown,
        };

        /** Returns the image type. 
         
            This is a very primitive check that looks at the magic header of the image data to determine its type:

            - 0x89 0x50 for PNG (although there is more to the PNG header)
            - 0xff 0xd8 for JPG
            - 0x71 0x6f for QOI (again there is more)

            If none of the above is true, but the data correspond to the raw memory format, then raw memory is returned. Otherwise unknown is returned.  
         */
        ImageType getImageType() const;

        
    private:
        static constexpr uint32_t FILE_SD = 0xffffffff;
        static constexpr uint32_t FILE_CARTRIDGE = 0xfffffffe;

        void clone(ImageSource const & from) {
            size_ = from.size_;
            if (hal::memory::isImmutableDataPtr(from.data_.get())) {
                data_ = immutable_ptr<uint8_t>{from.data_.get()};
            } else {
                uint32_t memSize = from.size_;
                if (memSize == FILE_SD || memSize == FILE_CARTRIDGE)
                    memSize = std::strlen(reinterpret_cast<char const *>(from.data_.get())) + 1; // null terminated
                // for memory sources we need to make a copy of the data
                uint8_t * newData = (memSize == 0) ? nullptr : new uint8_t[memSize];
                std::memcpy(newData, from.data_.get(), memSize);
                data_ = immutable_ptr<uint8_t>{newData};
            }
        }

        void invalidate() {
            data_.release();
            size_ = 0;
        }

        // size of the raw data in bytes, or magic value indicating file source
        uint32_t size_;

        // either pointer to the data itself, or path to the file
        immutable_ptr<uint8_t> data_;

    }; // rckid::ImageSource

    inline Writer operator << (Writer w, ImageSource const & img) {
        if (! img.good()) {
            w << "<invalid>";
            return w;
        }
        switch (img.type()) {
            case ImageSource::Type::Memory:
                w << "<memory>";
                break;
            case ImageSource::Type::SD:
                w << "sd:" << reinterpret_cast<char const *>(img.data());
                break;
            case ImageSource::Type::Cartridge:
                w << "cartridge:" << reinterpret_cast<char const *>(img.data());
                break;
            default:
                UNREACHABLE;
        }
        return w;
    }

    inline Reader operator >> (Reader r, ImageSource & img) {
        String path;
        // TODO this is brittle and only works till end of line
        r >> path;
        if (path.size() > 0) {
            if (path.startsWith("sd:")) {
                img = ImageSource{path.substr(3), fs::Drive::SD};
            } else if (path.startsWith("cartridge:")) {
                img = ImageSource{path.substr(10), fs::Drive::Cartridge};
            } else {
                // for memory sources we cannot do much, so we just create an empty image source
                img = ImageSource{};
            }
        } else {
            // likely was memory
            img = ImageSource{};
        }
        return r;
    }

} // namespace rckid