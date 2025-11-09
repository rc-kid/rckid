#pragma once

#include "bitmap.h"
#include "../filesystem.h"
#include "../assets/icons_64.h"

namespace rckid {

    /** Icon is a simple class that stores information necessary decoding bitmap at later time. 
      
        This is either a buffer in memory containing the bitmap data, or path to a file. The icon only takes up a pointer and uint32_t in memory and supports both immutable memory as well as mutable buffers and strings. When the toBitmap() method is called, the icon materializes the stored buffer or file path to a bitmap. 

        TODO support other bitmap depths. 
     */
    class Icon {
    public:

        Icon() = default;

        template<uint32_t SIZE>
        Icon(uint8_t const (&buffer)[SIZE]): buffer_{buffer}, bufferSize_{SIZE} {
            if (! memoryIsImmutable(buffer)) {
                uint8_t * newBuffer = new uint8_t[SIZE];
                memcpy(newBuffer, buffer, SIZE);
                buffer_ = newBuffer;
            }
        }

        Icon(char const * str): bufferSize_{BUFFER_FILE_PATH} {
            if (memoryIsImmutable(str)) {
                buffer_ = reinterpret_cast<uint8_t const *>(str);
            } else {
                buffer_ = new uint8_t[strlen(str) + 1];
                memcpy(const_cast<uint8_t *>(buffer_), str, strlen(str) + 1);
            }
        }

        Icon(String const & str): Icon{str.c_str()} {}

        Icon(String && str) noexcept: buffer_{reinterpret_cast<uint8_t const *>(str.release())}, bufferSize_{BUFFER_FILE_PATH} {}

        Icon(Icon const & other):
            buffer_{other.buffer_},
            bufferSize_{other.bufferSize_} {
            if (! memoryIsImmutable(buffer_)) {
                uint32_t bufSize = bufferSize_ == BUFFER_FILE_PATH ? strlen(reinterpret_cast<char const *>(buffer_)) + 1 : bufferSize_;
                uint8_t * newBuffer = new uint8_t[bufSize];
                memcpy(newBuffer, other.buffer_, bufSize);
                buffer_ = newBuffer;
            }
        }
        
        Icon(Icon && other):
            buffer_{std::exchange(other.buffer_, nullptr)},
            bufferSize_{std::exchange(other.bufferSize_, 0)} {
        }

        ~Icon() {
            if (! memoryIsImmutable(buffer_))
                delete [] buffer_;
        }

        Icon & operator = (Icon && other) {
            if (this != & other) {
                if (! memoryIsImmutable(buffer_))
                    delete [] buffer_;
                buffer_ = other.buffer_;
                bufferSize_ = other.bufferSize_;
                other.buffer_ = nullptr;
                other.bufferSize_ = 0;
            }
            return *this;
        }

        bool valid() const { return buffer_ != nullptr; }

        /** Materializes the icon into an RGB bitmap. 
         
            If the icon is not specified, materializes the default icon (poo:). 
         */
        Bitmap toBitmap() const {
           return Bitmap{getDecoder()};
        }

        /** Materializes the icon into given bitmap. If the bitmap is already the correct size, this makes the operation use less memory.
         */
        void intoBitmap(Bitmap & bitmap) const {
            PNG png{getDecoder()};
            /*
            if (png.width() > bitmap.width() || png.height() > bitmap.height()) {
                bitmap.resize(png.width(), png.height());
            }
            */
            bitmap.loadImage(std::move(png));
        }

        bool isFile() const { return bufferSize_ == BUFFER_FILE_PATH; }

        char const * filename() const {
            if (bufferSize_ == BUFFER_FILE_PATH)
                return reinterpret_cast<char const *>(buffer_);
            else
                return nullptr;
        }

    private:

        PNG getDecoder() const {
            if (bufferSize_ == BUFFER_FILE_PATH) {
                auto f = fs::fileRead(reinterpret_cast<char const *>(buffer_));
                if (f.good())
                    return PNG::fromStream(std::move(f));
            } else if (valid()) {
                return PNG::fromBuffer(buffer_, bufferSize_);
            }
            return PNG::fromBuffer(assets::icons_64::poo);
        }

        static constexpr uint32_t BUFFER_FILE_PATH = 0xffffffff; 
        uint8_t const * buffer_ = nullptr;
        uint32_t bufferSize_ = 0;

    }; // rckid::Icon

} // namespace rckid