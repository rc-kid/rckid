#pragma once

#include <optional>

#include <platform.h>

#include <string.h>

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

        std::optional<uint8_t> read() {
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
                auto c = read();
                if (!c || *c == '\n')
                    break;
                if (*c != '\r')
                    result.appendChar(*c);
            }
            return result.str();
        }
    }; 

    class BufferedReadStream : public ReadStream {
    public:
        virtual std::optional<uint8_t> peek() = 0;

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

        void write(uint8_t value) {
            write(&value, 1);
        }

        Writer writer() {
            return Writer([this](char c) { write(c); });
        }
    };

    class ReadWriteStream : public ReadStream, public WriteStream {

    };

    class RandomReadStream : public ReadStream {

    };

    class RandomWriteStream : public WriteStream {

    }; 

} // namespace rckid