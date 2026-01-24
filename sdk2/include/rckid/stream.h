#pragma once

#include <optional>

#include <platform.h>
#include <platform/writer.h>

#include <rckid/error.h>
#include <rckid/memory.h>
#include <rckid/string.h>
#include <rckid/serialization.h>

namespace rckid {

    class ReadStream {
    public:

        /** Destroys the stream and releases resources.
         */
        virtual ~ReadStream() = default;

        /** Reads up to bufferSize bytes from the stream. Returns the actual number of bytes read. 
         */
        virtual uint32_t read(uint8_t * buffer, uint32_t bufferSize) = 0;

        /** Returns true if the stream is at the end, i.e. no more data can be read from it.
         */
        virtual bool eof() const = 0;

        std::optional<uint8_t> readByte() {
            uint8_t result;
            uint32_t numBytes = read(& result, 1);
            if (numBytes == 1)
                return result;
            else
                return std::nullopt;
        }

        String readLine() {
            StringBuilder result;
            while (true) {
                auto c = readByte();
                if (!c || *c == '\n')
                    break;
                if (*c != '\r')
                    result.appendChar(static_cast<char>(*c));
            }
            return result.str();
        }
    }; 

    class BufferedReadStream : public ReadStream {
    public:
        virtual std::optional<uint8_t> peek() = 0;

        /** Returns reader that will read from the stream.
         
            The reader starts reading at the current position of the stream.
         */
        Reader reader() {
            return Reader([this](bool advance) -> int32_t {
                if (eof())
                    return Reader::EOFMarker;
                auto c = advance ? readByte() : peek();
                if (c)
                    return static_cast<int32_t>(*c);
                else
                    return Reader::EOFMarker;
            });
        }

        /** Returns binary reader for the stream. 
         
            The binary reader starts reading at the current position of the stream.
         */
        BinaryReader binaryReader() {
            return BinaryReader([this](bool advance) -> uint32_t {
                if (eof())
                    return BinaryReader::EOFMarker;
                auto c = advance ? readByte() : peek();
                if (c)
                    return static_cast<uint32_t>(*c);
                else
                    return BinaryReader::EOFMarker;
            });
        }
    }; 

    class WriteStream {
    public:
        virtual ~WriteStream() = default;

        virtual uint32_t tryWrite(uint8_t const * buffer, uint32_t bufferSize) = 0;

        void write(uint8_t const * buffer, uint32_t bufferSize) {
            uint32_t written = tryWrite(buffer, bufferSize);
            if (written != bufferSize)
                FATAL_ERROR("Partial Write", written);
        }

        void writeByte(uint8_t value) {
            write(&value, 1);
        }

        /** Returns text writer that will output to the stream. 
         */
        Writer writer() {
            return Writer([this](char c) { writeByte(static_cast<uint8_t>(c)); });
        }

        /** Returns binary writer that will output to the stream. 
         */
        BinaryWriter binaryWriter() {
            return BinaryWriter([this](uint8_t c) { writeByte(c); });
        }
    };

    class RandomReadStream : public BufferedReadStream {
    public:
        virtual uint32_t size() const = 0;
        virtual uint32_t seek(uint32_t position) = 0;
    };

    class RandomWriteStream : public WriteStream {
    public:
        virtual uint32_t seek(uint32_t position) = 0;
    }; 

    class MemoryStream : public RandomReadStream, public RandomWriteStream {
    public:
        MemoryStream(uint8_t * buffer, uint32_t bufferSize) :
            buffer_{buffer, bufferSize},
            pos_{0} {
        }

        MemoryStream(uint8_t const * buffer, uint32_t bufferSize) :
            buffer_{buffer, bufferSize},
            pos_{0} {
        }

        static MemoryStream withCapacity(uint32_t capacity) {
            uint8_t * buffer = new uint8_t[capacity];
            return MemoryStream{buffer, capacity};
        }

        uint32_t read(uint8_t * buffer, uint32_t bufferSize) override {
            uint32_t toRead = std::min(bufferSize, buffer_.count() - pos_);
            std::memcpy(buffer, buffer_.ptr() + pos_, toRead);
            pos_ += toRead;
            return toRead;
        }

        bool eof() const override {
            return pos_ >= buffer_.count();
        }

        std::optional<uint8_t> peek() override {
            if (pos_ >= buffer_.count())
                return std::nullopt;
            return buffer_.ptr()[pos_];
        }

        uint32_t tryWrite(uint8_t const * buffer, uint32_t bufferSize) override {
            uint32_t toWrite = std::min(bufferSize, buffer_.count() - pos_);
            std::memcpy(buffer_.mut() + pos_, buffer, toWrite);
            pos_ += toWrite;
            return toWrite;
        }

        uint32_t size() const override {
            return buffer_.count();
        }

        uint32_t seek(uint32_t position) override {
            if (position > buffer_.count())
                pos_ = buffer_.count();
            else 
                pos_ = position;
            return pos_;
        }
    private:
        mutable_ptr<uint8_t> buffer_;
        uint32_t pos_;
    }; 

} // namespace rckid