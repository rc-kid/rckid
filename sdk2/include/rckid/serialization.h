#pragma once

#include <platform.h>
#include <functional>

/** Serialization & Deserialization

    RCKid SDK provides simple and extensible serialization and deserialization framework via the Writer and Reader classes. Two basic serializers are provided - text based Writer and Reader and their binary based counterparts BinaryWriter and BinaryReader. Operators << and >> are used for serialization and deserialization respectively and types that allow themselves to be serialized should overload those for the applicable reader and writer classes.

    TODO add more documentation
 */

namespace rckid {

    // pull platform's writer to RCKid namespace
    using Writer = ::Writer;

    /** Simple text reader (deserializer)
     
        The reader is the counterpart to the Writer class, allowing reading characters from a source one at a time. The reader is created with a closure that knows how to read a single character at a time. Operator >> should be overloaded on Reader to provide deserialization for various types. 

        As the deserialization cannot be as simpe as serialization (lookahead is often necesary, such as to determine if next character belongs to the current token or not), the reader's callback is more complicated. It has a boolean flag that determines whether the read cursor should be advanced (read), or not (peek). Furthermore the callback does not return char, but int32_t, so that out of char bounds values can be interpreted as eof (EOFMarker).
        
        For convenience, the Reader class provides getChar(), peekChar() and eof() methods with common signatures that should be used instead. For getChar and peekChar, if eof is reached, 0 is returned (assuming 0 is not valid input character).
     */
    class Reader {
    public:

        static constexpr int32_t EOFMarker = 256;

        using GetCharCallback = std::function<int32_t(bool)>;

        explicit Reader(GetCharCallback getChar): 
            getChar_{getChar} {
        }

        char getChar() {
            int result = rawCallback(true);
            if (result == EOFMarker)
              result = 0;
            return static_cast<char>(result);
        }

        char peekChar() {
            int result = rawCallback(false);
            if (result == EOFMarker)
              result = 0;
            return static_cast<char>(result);
        }

        bool eof() {
            return rawCallback(false) == EOFMarker;
        }

        int32_t rawCallback(bool advance = true) { return getChar_(advance); }

    private:
        GetCharCallback getChar_;
    }; // Reader

    /** Allows serialization of various types in binary format. 
     
        The binary writer uses the same principle as the Writer class, only its `<<` overloads should format the data in binary, *not* textual format. Instead of characters works on unsigned bytes.
      */
    class BinaryWriter {
    public:
        using PutByteCallback = std::function<void(uint8_t)>;

        class Converter{};

        explicit BinaryWriter(PutByteCallback putByte): putByte_{putByte} {}

        void putByte(uint8_t c) { putByte_(c); }
        
    private:
        PutByteCallback putByte_;

    }; // BinaryWriter

    /** Allows deserialization of binary data. 
     
        Uses the same principle as the Reader class, only its `>>` overloads should parse binary data, *not* textual format. Instead of chars, works on unsigned bytes.
     */
    class BinaryReader {
    public:
        static constexpr int32_t EOFMarker = 256;

        using GetByteCallback = std::function<uint32_t(bool)>;

        explicit BinaryReader(GetByteCallback getByte): 
            getByte_{getByte} {
        }

        uint8_t getByte() {
            uint32_t result = rawCallback(true);
            if (result == EOFMarker)
              result = 0;
            return static_cast<uint8_t>(result);
        }

        uint8_t peekByte() {
            uint32_t result = rawCallback(false);
            if (result == EOFMarker)
              result = 0;
            return static_cast<uint8_t>(result);
        }

        bool eof() {
            return rawCallback(false) == EOFMarker;
        }

        int32_t rawCallback(bool advance = true) { return getByte_(advance); }

    private:
        GetByteCallback getByte_;

    }; // BinaryReader

    // below are various overloads for << and >> operators for the readers and writers specified above

    inline Reader operator >> (Reader r, char & c) {
        c = r.getChar();
        return r;
    }

    inline Reader operator >> (Reader r, bool & b) {
        char c = r.getChar();
        b = (c != 0 && c != '0' && c != 'F' && c != 'f');
        return r;
    }

    inline Reader operator >> (Reader r, uint32_t & into) {
        uint32_t result = 0;
        while (!r.eof()) {
            char c = r.peekChar();
            if (c >= '0' && c <= '9') {
                r.getChar();
                result = result * 10 + (c - '0');
            } else {
                break;
            }
        }
        into = result;
        return r;
    }


    // TODO do others


    inline BinaryWriter operator << (BinaryWriter w, uint8_t const & what) {
        w.putByte(what);
        return w;
    }

    inline BinaryWriter operator << (BinaryWriter w, uint16_t const & what) {
        w.putByte(static_cast<uint8_t>(what & 0xff));
        w.putByte(static_cast<uint8_t>((what >> 8) & 0xff));
        return w;
    }

    inline BinaryWriter operator << (BinaryWriter w, uint32_t const & what) {
        w.putByte(static_cast<uint8_t>(what & 0xff));
        w.putByte(static_cast<uint8_t>((what >> 8) & 0xff));
        w.putByte(static_cast<uint8_t>((what >> 16) & 0xff));
        w.putByte(static_cast<uint8_t>((what >> 24) & 0xff));
        return w;
    }

    inline BinaryWriter operator << (BinaryWriter w, uint64_t const & what) {
        w.putByte(static_cast<uint8_t>(what & 0xff));
        w.putByte(static_cast<uint8_t>((what >> 8) & 0xff));
        w.putByte(static_cast<uint8_t>((what >> 16) & 0xff));
        w.putByte(static_cast<uint8_t>((what >> 24) & 0xff));
        w.putByte(static_cast<uint8_t>((what >> 32) & 0xff));
        w.putByte(static_cast<uint8_t>((what >> 40) & 0xff));
        w.putByte(static_cast<uint8_t>((what >> 48) & 0xff));
        w.putByte(static_cast<uint8_t>((what >> 56) & 0xff));
        return w;
    }

    inline BinaryWriter operator << (BinaryWriter w, int8_t const & what) {
        w.putByte(static_cast<uint8_t>(what));
        return w;
    }

    inline BinaryWriter operator << (BinaryWriter w, int16_t const & what) {
        return w << static_cast<uint16_t>(what);
    }

    inline BinaryWriter operator << (BinaryWriter w, int32_t const & what) {
        return w << static_cast<uint32_t>(what);
    }

    inline BinaryWriter operator << (BinaryWriter w, int64_t const & what) {
        return w << static_cast<uint64_t>(what);
    }

    inline BinaryWriter operator << (BinaryWriter w, char const & what) {
        w.putByte(static_cast<uint8_t>(what));
        return w;
    }

    inline BinaryWriter operator << (BinaryWriter w, bool const & value) {
        return w << static_cast<uint8_t>(value ? 1 : 0);
    }

    inline BinaryReader operator >> (BinaryReader r, uint8_t & into) {
        into = r.getByte();
        return r;
    }

    inline BinaryReader operator >> (BinaryReader r, uint16_t & into) {
        uint16_t lo = r.getByte();
        uint16_t hi = r.getByte();
        into = lo + (hi << 8);
        return r;
    }   

    inline BinaryReader operator >> (BinaryReader r, uint32_t & into) {
        uint16_t lo, hi;;
        r >> lo;
        r >> hi;
        into = lo + (static_cast<uint32_t>(hi) << 16);
        return r;
    }

    inline BinaryReader operator >> (BinaryReader r, uint64_t & into) {
        uint32_t lo, hi;
        r >> lo;
        r >> hi;
        into = lo + (static_cast<uint64_t>(hi) << 32);
        return r;
    }

    inline BinaryReader operator >> (BinaryReader r, int8_t & into) {
        uint8_t x;
        r >> x;
        into = static_cast<int8_t>(x);
        return r;
    }

    inline BinaryReader operator >> (BinaryReader r, int16_t & into) {
        uint16_t x;
        r >> x;
        into = static_cast<int16_t>(x);
        return r;
    }

    inline BinaryReader operator >> (BinaryReader r, int32_t & into) {
        uint32_t x;
        r >> x;
        into = static_cast<int32_t>(x);
        return r;
    }

    inline BinaryReader operator >> (BinaryReader r, int64_t & into) {
        uint64_t x;
        r >> x;
        into = static_cast<int64_t>(x);
        return r;
    }

    inline BinaryReader operator >> (BinaryReader r, char & into) {
        into = static_cast<char>(r.getByte());
        return r;
    }

    inline BinaryReader operator >> (BinaryReader r, bool & into) {
        into = r.getByte() != 0;
        return r;
    }

} // namespace rckid


