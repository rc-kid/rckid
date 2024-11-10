#pragma once

#include <algorithm>

#include "../rckid.h"

namespace rckid {

    /** Simplest read stream interface.  
     
        Defines the very basic read stream interface that consists of a method hat reads up to N bytes from the stream. Assumes no other information about the stream, and does not allow seeking. 
     */
    class ReadStream {
    public:
        virtual ~ReadStream() = default;
        /** Reads up to bufferSize bytes from the stream. Returns the actual number of bytes read. 
         */
        virtual uint32_t read(uint8_t * buffer, uint32_t bufferSize) = 0;
    }; // rckid::InStream

    /** Simplest write stream interface. 
     
        Simply allows arrays of bytes to be written to the stream. Assumes no other knowledge of the stream. 
     */
    class WriteStream {
    public:
        virtual ~WriteStream() = default;
        
        /** Writes the given buffer to the stream. Returns the numbed of bytes written, which should be identical to bufferSize on success. 
         */
        virtual uint32_t write(uint8_t const * buffer, uint32_t bufferSize) = 0;

        /** Writes single byte to the stream. 
         */
        bool write(uint8_t data) {
            return write(&data, 1) == 1;
        }

        /** Returns a writer for formatter and serialized writes to the stream.
         */
        Writer write() {
            return Writer([this](char c) {
                bool result = write(static_cast<uint8_t>(c));
                ASSERT(result);
            });
        }

    }; // rckid::OutStream

    /** Random read stream. 
     
        Extends the basic ReadStream with the ability to seek and to know the size of the stream. 
     */
    class RandomReadStream : public ReadStream {
    public:
        /** Returns the size of the stream in bytes. 
         */
        virtual uint32_t size() const = 0;
        /** Seeks the read cursor to the given position.
         
            Returns the actual position, which is identical to the argument on success, but can be the end of the stream if the position specified was outside of the stream's size. 
         */
        virtual uint32_t seek(uint32_t position) = 0;
    }; // rckid::RandomReadStream

    /** Random read stream from memory buffer. 
     
        Provides the RandomReadStream interface for memory buffers. Does not own the buffer it reads from, so it must be kept alive by the user. 
     */
    class MemoryReadStream : public RandomReadStream {
    public:

        /** Creates new stream from given buffer and sets the reading cursor to the beginning. 
         */
        MemoryReadStream(uint8_t const * buffer, uint32_t bufferSize):
            buffer_{buffer},
            bufferSize_{bufferSize},
            pos_{0} {
        }

        /** Creates new stream from given buffer and sets the reading cusror to the beginning. 
         */
        template<uint32_t SIZE>
        MemoryReadStream(uint8_t const (&buffer)[SIZE]): MemoryReadStream(buffer, SIZE) {}

        /** Reads up to bufferSize bytes into the buffer and returns the number of bytes read. This is 0 if at the end of the time. Also advances the read cursor by the actal bytes read. 
         */
        uint32_t read(uint8_t * buffer, uint32_t bufferSize) override {
            uint32_t available = std::min(bufferSize, bufferSize_ - pos_);
            if (available != 0) {
                memcpy(buffer, buffer_ + pos_, available);
                pos_ += available;
            }
            return available;
        }

        /** Returns the size of the underlying memory buffer. 
         */
        uint32_t size() const override { return bufferSize_; }

        /** Seeks the read cursor to given position.
         
            0 for the beginning. Setting the position larger than size of the memory buffer will set the position to the end of the buffer. Returns the actual position after the call. 
         */
        uint32_t seek(uint32_t position) override {
            if (pos_ > bufferSize_)
                pos_ = bufferSize_;
            else 
            pos_ = position;
            return pos_;
        }
        
    private:
        uint8_t const * buffer_;
        uint32_t bufferSize_;
        uint32_t pos_;
    }; // rckid::MemoryReadStream

} // namespace rckid