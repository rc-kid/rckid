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

    using Converter = std::function<void(Writer &)>;

    static constexpr char endl = '\n';

    explicit Writer(std::function<void(char)> putChar):putChar_{putChar} {}

    Writer & operator << (Converter & conv) {
        conv(*this);
        return *this;
    }

    Writer & operator << (Converter && conv) {
        conv(*this);
        return *this;
    }

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

#if (! defined ARCH_PC)
    Writer & operator << (size_t x) {
        if constexpr(sizeof(size_t) == 4)
            return (*this) << static_cast<uint32_t>(x);
        else 
            return (*this) << static_cast<uint64_t>(x);
    }
#endif

    Writer & operator << (void * address) {
        static_assert(sizeof(void*) <= 8);
        if constexpr (sizeof(void*) > 4)
            return (*this) << reinterpret_cast<uint64_t>(address);
        else
            return (*this) << static_cast<uint32_t>(reinterpret_cast<uint64_t>(address));
    }

    //std::enable_if<sizeof(int) == 4, Writer &>::type
    Writer & operator << (int value) {
        if (value < 0) {
            putChar_('-');
            value *= -1;
        }
        if constexpr (sizeof(int) == 4)
            return (*this) << static_cast<uint32_t>(value);
        else
            return (*this) << static_cast<uint64_t>(value);
    }

/*
    typename std::enable_if<sizeof(int) != 4, Writer &>::type
    operator << (int32_t value) {
        if (value < 0) {
            putChar_('-');
            value *= -1;
        }
        return (*this) << static_cast<uint32_t>(value);
    }
*/

    //std::enable_if<sizeof(int) == 4, Writer &>::type
    //operator << (int value) { return (*this) << (uint32_t)}

private:

#ifdef foobar

    Writer & format(uint8_t value) {

    }

    Writer & format(uint16_t value) {

    }

    Writer & format(uint32_t value) {
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

    Writer & format(uint64_t value) {
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

    #endif

    std::function<void(char)> putChar_;
}; // Writer

/** Very simple string buffer that uses the Writer API to serialize values into a string. 
 */
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

/** Writer formatter that ensures width of given value of at least N characters with an optional fill character (defaults to space). 
 */
template<typename T>
inline Writer::Converter fillLeft(T const & what, uint32_t width, char fill = ' ') {
    return [what, width, fill](Writer & writer) mutable {
        std::string x{STR(what)};
        while (x.size() < width) {
            writer << fill;
            --width;
        }
        writer << x;
    };
}

template<typename T>
Writer::Converter hex(T what, bool header = true);

/** Displays the unsigned 8bit number in hex format (aligned). 
 */
template<>
inline Writer::Converter hex(uint8_t what, bool header) {
    return [what, header](Writer & writer) {
        if (header)
            writer << '0' << 'x';
        writer << "0123456789abcdef"[(what >> 4) & 0xf];
        writer << "0123456789abcdef"[what & 0xf];
    };
}

/** Displays the unsigned 16bit number in hex format (aligned). 
 */
template<>
inline Writer::Converter hex(uint16_t what, bool header) {
    return [what, header](Writer & writer) {
        if (header)
            writer << '0' << 'x';
        writer << "0123456789abcdef"[what >> 12];
        writer << "0123456789abcdef"[(what >> 8) & 0xf];
        writer << "0123456789abcdef"[(what >> 4) & 0xf];
        writer << "0123456789abcdef"[what & 0xf];
    };
}

/** Displays the unsigned 32bit number in hex format (aligned). 
 */
template<>
inline Writer::Converter hex(uint32_t what, bool header) {
    return [what, header](Writer & writer) {
        if (header)
            writer << '0' << 'x';
        writer << "0123456789abcdef"[what >> 28];
        writer << "0123456789abcdef"[(what >> 24) & 0xf];
        writer << "0123456789abcdef"[(what >> 20) & 0xf];
        writer << "0123456789abcdef"[(what >> 16) & 0xf];
        writer << "0123456789abcdef"[(what >> 12) & 0xf];
        writer << "0123456789abcdef"[(what >> 8) & 0xf];
        writer << "0123456789abcdef"[(what >> 4) & 0xf];
        writer << "0123456789abcdef"[what & 0xf];
    };
}

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


#endif