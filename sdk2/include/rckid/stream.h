#pragma once

#include <optional>

#include <platform.h>
#include <platform/writer.h>

#include <rckid/error.h>
#include <rckid/memory.h>
#include <rckid/string.h>
#include <rckid/serialization.h>

namespace rckid {

    /** Minimal read stream. 
     
        The read stream provides sequential block oriented reading from a source and requires only 2 functions for its implementation (read() and eof()). The stream provides helpers reading single byte, string with custom terminator and a full line.
     */
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

        /** Reads single byte from the stream. 
         
            Returns std::nullopt if the stream is at its end (eof() == true).
         */
        std::optional<uint8_t> readByte() {
            uint8_t result;
            uint32_t numBytes = read(& result, 1);
            if (numBytes == 1)
                return result;
            else
                return std::nullopt;
        }

        /** Reads text from the stream until a delimiting character (null by default) is found. 
         */
        String readString(char until = '\0') {
            StringBuilder result;
            while (true) {
                auto c = readByte();
                if (!c || *c == until)
                    break;
                result.appendChar(static_cast<char>(*c));
            }
            return result.str();
        }

        /** Reads an entie line from the stream (deliming character is new line).
         */
        String readLine() { return readString('\n'); }
    }; 

    /** Buffered read stream. 
     
        On top of the basic functionality of the ReadStream, the buffered variant supports a single byte peek (non-advancing read). Adding this mentod is enough extra functionality to proviode Reader and BinaryReader interfaces that allow deserialization of values from the stream. 
     */
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

    /** Simplest write stream. 
     
        Counterpart to ReadStream, this is the simplest implementation of a writeable stream. Only a single method is required - tryWrite() w- which takes a sequence of bytes and writes them to the stream, returning the number of bytes written, or 0 when error. From this the stream provides the write() method, which asserts non-partial writes as those are not expected to be supported in the SDK). 

        The WriteStream also provides Writer and BinaryWriter interfaces for serialization and formatting. 
     */
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

    /** Extension of the  buffered stream that provides telling and seeking capabilities for the stream. 
     
        Since peek() can be impleented as read followed by a seek to previous position, the RandomReadStream also implements the peek() method from BufferedReadStream.
     */
    class RandomReadStream : public BufferedReadStream {
    public:

        std::optional<uint8_t> peek() override {
            if (eof())
                return std::nullopt;
            uint32_t currentPos = tell();
            auto c = readByte();
            seek(currentPos);
            return c;
        }

        /** Returns the size of the stream. 
         */
        virtual uint32_t size() const = 0;

        /** Seeks the stream to given position (from start). 
         
            seek(0) sets the stream to its beginning, seek(size() - 1) will set the stream to read the last byte in the next step.
         */
        virtual uint32_t seek(uint32_t position) = 0;

        /** Returns the current position of the stream read cursor.
         */
        virtual uint32_t tell() const = 0;
    };

    /** Random access stream with telling and seeking of the write cursor position. 
     
     */
    class RandomWriteStream : public WriteStream {
    public:
        virtual uint32_t size() const = 0;
        virtual uint32_t seek(uint32_t position) = 0;
        virtual uint32_t tell() const = 0;
    }; 

    class MemoryReadStream : public RandomReadStream {
    public:
    
        MemoryReadStream(immutable_ptr<uint8_t> buffer, uint32_t size) :
            buffer_{std::move(buffer)},
            size_{size},
            pos_{0} {
        }

        uint32_t read(uint8_t * buffer, uint32_t bufferSize) override {
            uint32_t toRead = std::min(bufferSize, size_ - pos_);
            std::memcpy(buffer, buffer_.get() + pos_, toRead);
            pos_ += toRead;
            return toRead;
        }

        bool eof() const override {
            return pos_ >= size_;
        }

        std::optional<uint8_t> peek() override {
            if (pos_ >= size_)
                return std::nullopt;
            return buffer_.get()[pos_];
        }

        uint32_t size() const override {
            return size_;
        }

        uint32_t seek(uint32_t position) override {
            if (position > size_)
                pos_ = size_;
            else 
                pos_ = position;
            return pos_;
        }

        uint32_t tell() const override {
            return pos_;
        }

    private:
        immutable_ptr<uint8_t> buffer_;
        uint32_t size_;
        uint32_t pos_;
    }; // rckid::MemoryReadStream

    class MemoryStream : public RandomReadStream, public RandomWriteStream {
    public:

        MemoryStream(mutable_ptr<uint8_t> buffer) :
            buffer_{std::move(buffer)},
            pos_{0} {
        }

        static MemoryStream withCapacity(uint32_t capacity) {
            uint8_t * buffer = new uint8_t[capacity];
            return MemoryStream{mutable_ptr<uint8_t>{buffer, capacity}};
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

        uint32_t tell() const override {
            return pos_;
        }

    private:
        mutable_ptr<uint8_t> buffer_;
        uint32_t pos_;
    }; 

    // TODO is there a need for ReaderStream, BinaryReaderStream, WriterStream, BinaryWriterStream ?

    class ReaderStream : public BufferedReadStream {
    public:
        explicit ReaderStream(Reader & from): from_{from} {}

        uint32_t read(uint8_t * buffer, uint32_t bufferSize) override {
            uint32_t bytesRead = 0;
            while (bytesRead < bufferSize) {
                int32_t c = from_.rawCallback(true);
                if (c == Reader::EOFMarker)
                    break;
                buffer[bytesRead++] = static_cast<uint8_t>(c);
            }
            return bytesRead;
        }

        bool eof() const override {
            int32_t c = from_.rawCallback(false);
            return c == Reader::EOFMarker;
        }

        std::optional<uint8_t> peek() override {
            int32_t c = from_.rawCallback(false);
            if (c == Reader::EOFMarker)
                return std::nullopt;
            return static_cast<uint8_t>(c);
        }

    private:
        Reader & from_;
    }; // rckid::ReaderStream

} // namespace rckid