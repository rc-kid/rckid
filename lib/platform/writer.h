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

/** Provides the << operator for all possible writers (&, &&). The operator simply translates to calling the write function, which data types that can write themselves must override.
 */
template<typename W, typename T>
std::enable_if_t<std::is_same_v<std::decay_t<W>, Writer>, W&&>
operator << (W&& w, T const & arg) {
    write(w, arg);
    return std::forward<W>(w);
}

inline void write(Writer & w, unsigned long long x) {
    uint64_t order = 1000000000000000000ull;
    while (x < order && order > 1)
        order = order / 10;
    while (order >= 10) {
        w.putChar(static_cast<char>((x / order)) + '0');
        x = x % order;
        order = order / 10;
    }
    w.putChar(static_cast<char>(x) + '0');
}

inline void write(Writer & w, unsigned x) {
    uint32_t order = 1000000000;
    while (x < order && order > 1)
        order = order / 10;
    while (order >= 10) {
        w.putChar(static_cast<char>((x / order)) + '0');
        x = x % order;
        order = order / 10;
    }
    w.putChar(static_cast<char>(x) + '0');
}

inline void write(Writer & w, unsigned long value) { 
    if (sizeof(unsigned long) == 4)
        w << static_cast<unsigned>(value);
    else 
        w << static_cast<unsigned long long>(value);
}

inline void write(Writer & w, unsigned short value) {
    w << static_cast<unsigned>(value);
}

inline void write(Writer & w, unsigned char value) {
    w << static_cast<unsigned>(value);
}

inline void write(Writer & w, long long value) { 
    if (value < 0) {
        w.putChar('-');
        value *= -1;
    }
    w << static_cast<unsigned long long>(value);
}   

inline void write(Writer & w, int value) { 
    if (value < 0) {
        w.putChar('-');
        value *= -1;
    }
    w << static_cast<unsigned>(value);
}

inline void write(Writer & w, long value) {
    if (sizeof(long) == 4)
        w << static_cast<int>(value);
    else
        w << static_cast<long long>(value);
}

inline void write(Writer & w, short value) {
    w << static_cast<int>(value);
}

inline void write(Writer & w, signed char value) {
    w << static_cast<int>(value);
}

inline void write(Writer & w, char c) {
    w.putChar(c);
}

inline void write(Writer & w, char const * str) {
    while (*str != 0)
        w.putChar( *(str++) );
}

inline void write(Writer & w, bool b) {
    w.putChar(b ? 'T' : 'F');
}

template<typename T>
std::enable_if_t<std::is_base_of<Writer::Converter, T>::value, void> write(Writer & w, T && conv) {
    conv(w);
}

template<typename T>
std::enable_if_t<std::is_base_of<Writer::Converter, T>::value, void> write(Writer & w, T & conv) {
    conv(w);
}

/** Converter that displays numbers in hexadecimal. 
    
    The numbers are aligned based on their size and by default contain the `0x` prefix, which can be disabled for more compact output
 */
template<typename T>
class hex : public Writer::Converter {
public:
    hex(T what, bool header = true): what_{what}, header_{header} {}

    void operator () (Writer & writer) const {
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

    void operator () (Writer & writer) const {
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

    void operator () (Writer & writer) const {
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

inline void write(Writer & w, void * ptr) {
    w << hex(ptr);
}
template<typename T>
class alignRight : public Writer::Converter {
public:
    alignRight(T value, size_t width, char pad = ' ') : value_{value}, width_{width}, pad_{pad} {}

    void operator () (Writer & writer) const {
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

    void operator () (Writer & writer) const {
        if (value_ != 0)
            writer << value_ << separator_;
    }

private:
    T value_;
    char const * separator_;
};
