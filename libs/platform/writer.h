#pragma once

#include <cstring>
#include <string>
#include <functional>

#if (! defined STR)
#define STR(...) (StringWriter{} << __VA_ARGS__).str()
#endif

/** A very simple formatter for both human-readable texts and binary representations. 
 
    To make sure the writer is available even on very low level platforms such as ATTiny chips, it does not use C++'s lambda captures, but rely on passing a void * context pointer to the PutChar function. 
 */
class Writer {
public:

#if (defined PLATFORM_NOSTDCPP)
    typedef void (*CharWriter)(char);
    typedef void (*Converter)(Writer &);
#else
    using CharWriter = std::function<void(char)>;
    using Converter = std::function<void(Writer &)>;
#endif

    static constexpr char endl = '\n';

    explicit Writer(CharWriter putChar):putChar_{putChar} {}

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
    Writer & operator << (uint16_t x) { return *this << (uint32_t)x; }
    Writer & operator << (uint32_t x) {
        uint32_t order = 1000000000;
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

    Writer & operator << (void * address) {
        static_assert(sizeof(void*) <= 8);
        if constexpr (sizeof(void*) > 4)
            return (*this) << reinterpret_cast<uint64_t>(address);
        else
            return (*this) << static_cast<uint32_t>(reinterpret_cast<uint64_t>(address));
    }

    Writer & operator << (int8_t value) { 
        if (value < 0) {
            putChar_('-');
            value *= -1;
        }
        return (*this) << static_cast<uint8_t>(value);
    }

    Writer & operator << (int16_t value) { 
        if (value < 0) {
            putChar_('-');
            value *= -1;
        }
        return (*this) << static_cast<uint16_t>(value);
    }

    Writer & operator << (int32_t value) { 
        if (value < 0) {
            putChar_('-');
            value *= -1;
        }
        return (*this) << static_cast<uint32_t>(value);
    }

    Writer & operator << (int64_t value) { 
        if (value < 0) {
            putChar_('-');
            value *= -1;
        }
        return (*this) << static_cast<uint64_t>(value);
    }

private:

    CharWriter putChar_;
}; // Writer


#if (! defined PLATFORM_NOSTDCPP)
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
inline Writer::Converter fillRight(T const & what, uint32_t width, char fill = ' ') {
    return [what, width, fill](Writer & writer) mutable {
        std::string x{STR(what)};
        writer << x;
        while (x.size() < width) {
            writer << fill;
            --width;
        }
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

#endif 
