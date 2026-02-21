#pragma once

#include <cstring>
#include <cstdint>
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
    
private:
    PutCharCallback putChar_;
}; 

inline Writer operator << (Writer w, unsigned long long x) {
    uint64_t order = 1000000000000000000ull;
    while (x < order && order > 1)
        order = order / 10;
    while (order >= 10) {
        w.putChar(static_cast<char>((x / order)) + '0');
        x = x % order;
        order = order / 10;
    }
    w.putChar(static_cast<char>(x) + '0');
    return w;
}

inline Writer operator << (Writer w, unsigned x) {
    uint32_t order = 1000000000;
    while (x < order && order > 1)
        order = order / 10;
    while (order >= 10) {
        w.putChar(static_cast<char>((x / order)) + '0');
        x = x % order;
        order = order / 10;
    }
    w.putChar(static_cast<char>(x) + '0');
    return w;
}

inline Writer operator << (Writer w, unsigned long value) { 
    if (sizeof(unsigned long) == 4)
        return w << static_cast<unsigned>(value);
    else 
        return w << static_cast<unsigned long long>(value);
}

inline Writer operator << (Writer w, unsigned short value) {
    return w << static_cast<unsigned>(value);
}

inline Writer operator << (Writer w, unsigned char value) {
    return w << static_cast<unsigned>(value);
}

inline Writer operator << (Writer w, long long value) { 
    if (value < 0) {
        w.putChar('-');
        value *= -1;
    }
    return w << static_cast<unsigned long long>(value);
}   

inline Writer operator << (Writer w, int value) { 
    if (value < 0) {
        w.putChar('-');
        value *= -1;
    }
    return w << static_cast<unsigned>(value);
}

inline Writer operator << (Writer w, long value) {
    if (sizeof(long) == 4)
        return w << static_cast<int>(value);
    else
        return w << static_cast<long long>(value);
}

inline Writer operator << (Writer w, short value) {
    return w << static_cast<int>(value);
}

inline Writer operator << (Writer w, signed char value) {
    return w << static_cast<int>(value);
}

inline Writer operator << (Writer w, char c) {
    w.putChar(c);
    return w;
}

inline Writer operator << (Writer w, char const * str) {
    while (*str != 0)
        w.putChar( *(str++) );
    return w;
}

inline Writer operator << (Writer w, bool b) {
    w.putChar(b ? 'T' : 'F');
    return w;
}

template<typename T>
std::enable_if_t<std::is_base_of<Writer::Converter, T>::value, Writer> operator << (Writer w, T && conv) {
    conv(w);
    return w;
}

template<typename T>
std::enable_if_t<std::is_base_of<Writer::Converter, T>::value, Writer> operator << (Writer w, T & conv) {
    conv(w);
    return w;
}

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
        uint64_t what;
        if constexpr (std::is_pointer_v<T>)
            what = static_cast<int64_t>(reinterpret_cast<std::uintptr_t>(what_));
        else if constexpr (std::is_integral_v<T>)
            what = static_cast<int64_t>(what_);
        else
            static_assert(false, "T must be a pointer or an integral type");
        for (int shift = bits - 4; shift >= 0; shift -= 4)
            writer << "0123456789abcdef"[(what >> shift) & 0xf];
    }

private:
    T what_;
    bool header_;
}; // hex

/** Hex writer for a single byte that always uses 2 digits and emits no header. 
 */
class hex2 : public Writer::Converter {
public:
    hex2(uint8_t what): what_{what} {}

    void operator () (Writer & writer) {
        writer << "0123456789abcdef"[(what_ >> 4) & 0xf];
        writer << "0123456789abcdef"[what_ & 0xf];
    }
private:
    uint8_t what_;
}; // hex2

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
        uint64_t what;
        if constexpr (std::is_pointer_v<T>)
            what = static_cast<int64_t>(reinterpret_cast<std::uintptr_t>(what_));
        else if constexpr (std::is_integral_v<T>)
            what = static_cast<int64_t>(what_);
        else
            static_assert(false, "T must be a pointer or an integral type");
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
template<typename T>
class alignRight : public Writer::Converter {
public:
    alignRight(T value, size_t width, char pad = ' ') : value_{value}, width_{width}, pad_{pad} {}

    void operator () (Writer & writer) {
        switch (width_) {
            default:
            // TODO more & signed
                [[fallthrough]];
            case 5: 
                if (value_ < 10000)
                    writer << pad_;
                [[fallthrough]];
            case 4: 
                if (value_ < 1000)
                    writer << pad_;
                [[fallthrough]];
            case 3: 
                if (value_ < 100)
                    writer << pad_;
                [[fallthrough]];
            case 2:
                if (value_ < 10)
                    writer << pad_;
                [[fallthrough]];
            case 1:
                break;
        }
        writer << value_;
    }

private:
    T value_;
    size_t width_;
    char pad_;
};

template<typename T>
class nonZero : public Writer::Converter {
public:
    nonZero(T value, char const * separator = ""): value_{value}, separator_{separator} {}

    void operator () (Writer & writer) {
        if (value_ != 0)
            writer << value_ << separator_;
    }

private:
    T value_;
    char const * separator_;
};