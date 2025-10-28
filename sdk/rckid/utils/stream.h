#pragma once


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

        virtual bool eof() const = 0;

        uint8_t read() {
            uint8_t result;
            bool res = read(& result, 1);
            ASSERT(res == 1);
            return result;
        }

        /** Reads entire line from the stream.
         
            The returned line does not contain the line end character. End of file reads will return empty line (and so will empty lines).
         */
        String readLine() {
            String result;
            while (!eof()) {
                char c = static_cast<char>(read());
                if (c == '\n')
                    break;
                if (c != '\r')
                    result += c;
            }
            return result;
        }

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
        bool writeByte(uint8_t data) {
            return write(&data, 1) == 1;
        }

        /** Returns a writer for formatter and serialized writes to the stream.
         */
        Writer writer() {
            return Writer([](char c, void * self) {
                bool result = reinterpret_cast<WriteStream*>(self)->writeByte(static_cast<uint8_t>(c));
                ASSERT(result);
            }, this);
        }

    }; // rckid::WriteStream

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

        /** Create memory read strem from given null terminaed string.
         */
        MemoryReadStream(char const * buffer):
            MemoryReadStream(reinterpret_cast<uint8_t const *>(buffer), static_cast<uint32_t>(strlen(buffer))) {
        }

        /** Creates new stream from given buffer and sets the reading cusror to the beginning. 
         */
        template<uint32_t SIZE>
        MemoryReadStream(uint8_t const (&buffer)[SIZE]): MemoryReadStream(buffer, SIZE) {}

        /** Reads up to bufferSize bytes into the buffer and returns the number of bytes read. This is 0 if at the end of the time. Also advances the read cursor by the actal bytes read. 
         */
        uint32_t read(uint8_t * buffer, uint32_t bufferSize) override {
            uint32_t available = bufferSize < (bufferSize_ - pos_) ? bufferSize : (bufferSize_ - pos_);
            if (available != 0) {
                memcpy(buffer, buffer_ + pos_, available);
                pos_ += available;
            }
            return available;
        }

        bool eof() const {
            return pos_ >= bufferSize_;
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

    // serialization functions

    inline void serialize(WriteStream & into, uint8_t const * what, uint32_t size) {
        into.write(what, size);
    }

    inline void serialize(WriteStream & into, uint8_t const & what) {
        into.writeByte(what);
    }

    inline void serialize(WriteStream & into, int8_t const & what) {
        into.writeByte(static_cast<uint8_t>(what));
    }

    inline void serialize(WriteStream & into, uint16_t const & what) {
        serialize(into, static_cast<uint8_t>(what & 0xff));
        serialize(into, static_cast<uint8_t>(what >> 8));
    }

    inline void serialize(WriteStream & into, int16_t const & what) {
        serialize(into, static_cast<uint16_t>(what));
    }

    inline void serialize(WriteStream & into, uint32_t const & what) {
        serialize(into, static_cast<uint16_t>(what & 0xffff));
        serialize(into, static_cast<uint16_t>(what >> 16));
    }

    inline void serialize(WriteStream & into, int32_t const & what) {
        serialize(into, static_cast<uint32_t>(what));
    }

    inline void serialize(WriteStream & into, uint64_t const & what) {
        serialize(into, static_cast<uint32_t>(what & 0xffffffff));
        serialize(into, static_cast<uint32_t>(what >> 32));
    }

    inline void serialize(WriteStream & into, int64_t const & what) {
        serialize(into, static_cast<uint64_t>(what));
    }

    inline void serialize(WriteStream & into, bool const & value) {
        value ? serialize(into, static_cast<uint8_t>(1)) : serialize(into, static_cast<uint8_t>(0));
    }

    inline void serialize(WriteStream & into, String const & what) {
        serialize(into, static_cast<uint32_t>(what.size()));
        into.write(reinterpret_cast<uint8_t const *>(what.data()), what.size());
    }

    inline void serialize(WriteStream & into, TinyTime const & time) {
        uint8_t const * x = reinterpret_cast<uint8_t const *>(& time);
        into.write(x, sizeof(TinyTime));
    }

    inline void serialize(WriteStream & into, TinyDate const & time) {
        uint8_t const * x = reinterpret_cast<uint8_t const *>(& time);
        into.write(x, sizeof(TinyDate));
    }

    inline void serialize(WriteStream & into, TinyDateTime const & time) {
        serialize(into, time.time);
        serialize(into, time.date);
    }

    inline void serialize(WriteStream & into, TinyAlarm const & time) {
        static_assert(sizeof(TinyAlarm) == 3);
        uint8_t const * x = reinterpret_cast<uint8_t const *>(& time);
        serialize(into, x[0]);
        serialize(into, x[1]);
        serialize(into, x[2]);
    }

    // deserialization functions

    template<typename T>
    inline T deserialize(ReadStream & from) {
        T result;
        deserialize(from, result);
        return result;
    }

    inline void deserialize(ReadStream & from, uint8_t * data, uint32_t size) {
        from.read(data, size);
    }

    inline void deserialize(ReadStream & from, uint8_t & into) {
        into = from.read();
    }

    inline void deserialize(ReadStream & from, int8_t & into) {
        uint8_t x = from.read();
        into = static_cast<int8_t>(x);
    }

    inline void deserialize(ReadStream & from, uint16_t & into) {
        uint8_t lo = from.read();
        uint8_t hi = from.read();
        into = lo + (hi << 8);
    }

    inline void deserialize(ReadStream & from, int16_t & into) {
        uint16_t x;
        deserialize(from, x);
        into = static_cast<int16_t>(x);
    }

    inline void deserialize(ReadStream & from, uint32_t & into) {
        uint16_t lo, hi;
        deserialize(from, lo);
        deserialize(from, hi);
        into = lo + (static_cast<uint32_t>(hi) << 16);
    }

    inline void deserialize(ReadStream & from, int32_t & into) {
        uint32_t x;
        deserialize(from, x);
        into = static_cast<int32_t>(x);
    }

    inline void deserialize(ReadStream & from, uint64_t & into) {
        uint32_t lo, hi;
        deserialize(from, lo);
        deserialize(from, hi);
        into = lo + (static_cast<uint64_t>(hi) << 32);
    }

    inline void deserialize(ReadStream & from, int64_t & into) {
        uint64_t x;
        deserialize(from, x);
        into = static_cast<int64_t>(x);
    }

    inline void deserialize(ReadStream & from, bool & into) {
        uint8_t x;
        deserialize(from, x);
        into = (x == 1);
    }

    inline void deserialize(ReadStream & from, String & into) {
        uint32_t size;
        deserialize(from, size);
        into = String(' ', size);
        from.read(reinterpret_cast<uint8_t*>(into.data()), size);
    }

    inline void deserialize(ReadStream & from, TinyTime & into) {
        uint8_t * x = reinterpret_cast<uint8_t *>(&into);
        from.read(x, sizeof(TinyTime));
    }

    inline void deserialize(ReadStream & from, TinyDate & into) {
        uint8_t * x = reinterpret_cast<uint8_t *>(&into);
        from.read(x, sizeof(TinyDate));
    }

    inline void deserialize(ReadStream & from, TinyDateTime & into) {
        deserialize(from, into.time);
        deserialize(from, into.date);
    }

    inline void deserialize(ReadStream & from, TinyAlarm & into) {
        static_assert(sizeof(TinyAlarm) == 3);
        uint8_t * x = reinterpret_cast<uint8_t *>(&into);
        deserialize(from, x[0]);
        deserialize(from, x[1]);
        deserialize(from, x[2]);
    }

} // namespace rckid