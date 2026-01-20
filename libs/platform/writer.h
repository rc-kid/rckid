#pragma once

#include <cstring>
#include <string>
#include <functional>
#include <type_traits>

/** Abysmally simple and extensible formatter. 
 
    The writer is created with a closure that knows how to write a single character at a time, the task of the writer is then to break down various data types into series of characters. 
 */
class Writer {
public:
    using PutCharCallback = std::function<void(char)>;

    class Converter{};

    explicit Writer(PutCharCallback putChar): putChar_{putChar} {}

    void putChar(char c) { putChar_(c); }
    
    Writer & operator << (char c) { 
        putChar_(c);
        return *this; 
    }

    Writer & operator << (char const * str) {
        while (*str != 0)
            putChar_( *(str++) );
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
        uint64_t order = 1000000000000000000ull;
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

    Writer & operator << (int8_t value) { return *this << (int32_t)value; }
    Writer & operator << (int16_t value) { return *this << (int32_t)value; }
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

    template<typename T>
    std::enable_if_t<std::is_base_of<Converter, T>::value, Writer &> operator << (T && conv) {
        conv(*this);
        return *this;
    }

    template<typename T>
    std::enable_if_t<std::is_base_of<Converter, T>::value, Writer &> operator << (T & conv) {
        conv(*this);
        return *this;
    }

private:
    PutCharCallback putChar_;
}; 

/** Converter that displays numbers in hexadecimal. 
    
    The numbers are aligned based on their size and by default contain the `0x` prefix, which can be disabled for more compact output
 */
template<typename T>
class hex : public Writer::Converter {
public:
    hex(T what, bool header = true): what_{what}, header_{header} {}

    void operator () (Writer & writer) {
        if (header_)
            writer << '0' << 'x';
        unsigned bits = sizeof(T) * 8;
        uint64_t what = reinterpret_cast<uint64_t>(what_);
        for (int shift = bits - 4; shift >= 0; shift -= 4)
            writer << "0123456789abcdef"[(what >> shift) & 0xf];
    }

private:
    T what_;
    bool header_;
}; // hex

/** Converter that displays numbers in binary. 
    
    The numbers are aligned based on their size and by default contain the `0b` prefix, which can be disabled for more compact output
 */
template<typename T>
class bin : public Writer::Converter {
public:
    bin(T what, bool header = true): what_{what}, header_{header} {}

    void operator () (Writer & writer) {
        if (header_)
            writer << '0' << 'b';
        unsigned bits = sizeof(T) * 8;
        uint64_t what = static_cast<uint64_t>(what_);
        for (unsigned shift = bits - 1; shift >= 0; shift -= 1)
            writer << "01"[(what >> shift) & 0x1];
    }

private:
    T what_;
    bool header_;
}; // bin

inline Writer & operator << (Writer & w, void * ptr) {
    w << hex(ptr);
    return w;
}