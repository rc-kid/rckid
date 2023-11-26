#pragma once

#include <functional>

namespace rckid {

    class Writer {
    public:

        Writer(std::function<void(char)> putChar):putChar_{putChar} {}
        
        Writer & operator << (char const * str) {
            while (*str != 0)
                putChar_(*(str++));
            return *this;
        }

        Writer & operator << (unsigned x) {
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

        std::function<void(char)> putChar_;

    }; // rckid::Writer

} // namespace rckid