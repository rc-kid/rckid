#pragma once

#include <string>
#include <functional>

/** A simple formatter for writing human readable (ASCII) text to various places, such as the display, or serial debugging port, etc.
 */
class Writer {
public:

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
        // TODO fix this
        return (*this) << (uint32_t)x;
    }

private:

    std::function<void(char)> putChar_;

}; // Writer
