#pragma once

#include <cstring>
#include <string>
#include <functional>

#if (! defined STR)
#define STR(...) (StringWriter{} << __VA_ARGS__).str()
#endif


/** A very simple formatter for both human-readable texts and binary representations. 
 
    To make sure the writer is available even on very low level platforms such as ATTiny chips, it does not use C++'s lambda captures, but rely on passing a void * context poninter to the PutChar function. 
 */
class Writer {
public:

    explicit Writer(std::function<void(char)> putChar):putChar_{putChar} {}

    Writer & operator << (char const * str) {
        while (*str != 0)
            putChar_(*(str++));
        return *this;
    }

    Writer & operator << (std::string const & str) {
        return *this << str.c_str();
    }

    Writer & operator << (char c) { 
        putChar_(c); 
        return *this; 
    }

    Writer & operator << (bool b) {
        putChar_(b ? 'T' : 'F');
        return *this;
    }

    Writer & operator << (uint8_t x) { return *this << (uint32_t) x; }
    Writer & operator << (uint16_t x) { return *this << (uint32_t) x; }
    Writer & operator << (uint32_t x) {
        unsigned order = 1000000000;
        while (x < order && order > 1)
            order = order / 10;
        while (order >= 10) {
            putChar_(static_cast<char>((x / order)) + '0');
            x = x % order;
            order = order / 10;
        }
        putChar_(static_cast<char>(x) + '0');
        return *this;
    }
    Writer & operator << (uint64_t x) {
        uint64_t order = 10000000000000000000ul;
        while (x < order && order > 1)
            order = order / 10;
        while (order >= 10) {
            putChar_(static_cast<char>((x / order)) + '0');
            x = x % order;
            order = order / 10;
        }
        putChar_(static_cast<char>(x) + '0');
        return *this;
        return *this;
    }

private:
    std::function<void(char)> putChar_;
}; // Writer




#ifdef FOO

template<typename T>
class Binary {
public:
    T const & value;
}; 

template<typename T>
Binary<T> serialize(T value) { return Binary<T>{value}; }

/** A simple formatter for writing human readable (ASCII) text to various places, such as the display, or serial debugging port, etc.
 */
class Writer {
public:

    class hex {
    public:
        hex(uint8_t const * buffer, size_t size):
            buffer_{buffer}, bufferSize_{size} {
        }

        hex(uint8_t const & c):
            buffer_{&c}, bufferSize_{1} {
        }

        void convert(Writer & w) const {
            for (size_t i = 0; i < bufferSize_; ++i) {
                uint8_t c = buffer_[i] >> 4;
                if (c > 9)
                    w.putChar_('a' + (c - 10));
                else
                    w.putChar_('0' + c);
                c = buffer_[i] & 0xf;
                if (c > 9)
                    w.putChar_('a' + (c - 10));
                else
                    w.putChar_('0' + c);
            }
        }

    public:
        uint8_t const * buffer_;
        size_t bufferSize_;
    }; // Writer::hex


    Writer(std::function<void(char)> putChar):putChar_{putChar} {}

    static Writer toString(std::string & str) {
        return Writer([& str](char c) {
            str = str + c;
        });
    }
    
    Writer & operator << (char const * str) {
        while (*str != 0)
            putChar_(*(str++));
        return *this;
    }

    Writer & operator << (std::string const & str) { 
        for (size_t i = 0, e = str.size(); i != e; ++i)
            putChar_(str[i]);
        return *this;
    }

#if (defined ARCH_ARDUINO)
    Writer & operator << (String s) { return *this << s.c_str(); }
#endif

    Writer & operator << (char c) { 
        putChar_(c); 
        return *this; 
    }

    Writer & operator << (bool b) {
        putChar_(b ? 'T' : 'F');
        return *this;
    }

    // TODO only works for real HW
#if (defined ARCH_RP2040)
    Writer & operator << (unsigned int x) { return *this << (uint32_t) x; } 
#endif

    Writer & operator << (uint8_t x) { return *this << (uint32_t) x; }
    Writer & operator << (uint16_t x) { return *this << (uint32_t) x; }

    Writer & operator << (uint32_t x) {
        unsigned order = 1000000000;
        while (x < order && order > 1)
            order = order / 10;
        while (order >= 10) {
            putChar_((x / order) + '0');
            x = x % order;
            order = order / 10;
        }
        putChar_(x + '0');
        return *this;
    }

    Writer & operator << (int16_t x) {
        if (x < 0) {
            putChar_('-');
            x = -x;
        }
        return (*this) << static_cast<uint16_t>(x);
    }

    Writer & operator << (int32_t x) {
        if (x < 0) {
            putChar_('-');
            x = -x;
        }
        return (*this) << static_cast<uint32_t>(x);
    }

    Writer & operator << (uint64_t x) {
        uint64_t order = 10000000000000000000_u64;
        while (x < order && order > 1)
            order = order / 10;
        while (order >= 10) {
            putChar_((x / order) + '0');
            x = x % order;
            order = order / 10;
        }
        putChar_(x + '0');
        return *this;
    }

    Writer & operator << (hex const & converter) {
        converter.convert(*this);
        return *this;
    }

    template<typename T>
    Writer & operator << (Binary<T> const & value) {
        serialize(value.value);
        return *this;
    }

    void writeByte(uint8_t byte) { putChar_(byte); }

    void writeBuffer(uint8_t const * buffer, size_t size) {
        while (size-- != 0)
            putChar_(*buffer++);
    }

    template<typename T>
    void serialize(T what);

private:


    std::function<void(char)> putChar_;

}; // Writer

class Reader {
public:
    Reader(std::function<uint8_t()> getByte): getByte_{getByte} {}
    
    // TODO override >> for string to type conversion
    
    uint8_t readByte() {
        return getByte_();
    }

    void readBuffer(uint8_t * buffer, size_t size) {
        while (size-- != 0) 
            *(buffer++) = getByte_();
    }

    template<typename T>
    T deserialize(); 

private:
    std::function<uint8_t()> getByte_;

}; // Reader


class StringWriter {
public:
    StringWriter():
        writer_{[this](char c) { str_ += c; }} {
    }

    template<typename T>
    StringWriter & operator << (T x) {
        writer_ << x;
        return *this; 
    }

    std::string str() {
        return std::move(str_);
    }

private:
    std::string str_;
    Writer writer_;

}; // StringWriter


template<>
inline void Writer::serialize<uint8_t>(uint8_t value) {
    writeByte(value);
}

template<>
inline void Writer::serialize<uint16_t>(uint16_t value) {
    writeByte(value & 0xff);
    writeByte(value >> 8);
}

template<>
inline void Writer::serialize<char const *>(char const * value) {
    size_t len = strlen(value);
    serialize<uint16_t>(len);
    writeBuffer(reinterpret_cast<uint8_t const *>(value), len);
}

template<>
inline uint8_t Reader::deserialize<uint8_t>() { 
    return getByte_(); 
}

template<>
inline uint16_t Reader::deserialize<uint16_t>() {
    uint16_t result = getByte_();
    return (getByte_() << 8) | result;
}

template<>
inline std::string Reader::deserialize<std::string>() {
    size_t len = deserialize<uint16_t>();
    std::string result(len, '\0');
    readBuffer(reinterpret_cast<uint8_t*>(result.data()), len);
    return result;
}

#endif