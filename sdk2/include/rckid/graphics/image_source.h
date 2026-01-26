#pragma once

#include <rckid/rckid.h>
#include <rckid/memory.h>
#include <rckid/stream.h>
#include <rckid/filesystem.h>

namespace rckid {

    /** Describes the source of an image and provides its type decoding.
     
        Image source can be either pointer to region of memory containing the image data directly, or a path to the image file on either SD card, or cartridge.
     */
    class ImageSource {
    public:

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

        uint32_t size() const {
            ASSERT(type() == Type::Memory);
            return size_;
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

        /** Detects the image type of given stream. 
         
            The stream is reset to the beginning after the detection takes place.
         */
        static ImageType getImageType(RandomReadStream & stream);
        
    private:
        static constexpr uint32_t FILE_SD = 0xffffffff;
        static constexpr uint32_t FILE_CARTRIDGE = 0xfffffffe;

        void invalidate() {
            data_.release();
            size_ = 0;
        }

        // size of the raw data in bytes, or magic value indicating file source
        uint32_t size_;

        // either pointer to the data itself, or path to the file
        immutable_ptr<uint8_t> data_;

    }; // rckid::ImageSource

} // namespace rckid