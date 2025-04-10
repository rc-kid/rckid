#pragma once

#include <cstring>
#include <string>
#include <functional>
#include <type_traits>

/** A very simple formatter for both human-readable texts and binary representations. 
 
    To make sure the writer is available even on very low level platforms such as ATTiny chips, it does not use C++'s lambda captures, but rely on passing a void * context pointer to the PutChar function. 
 */
class Writer {
public:
    class Converter{};

#if (defined PLATFORM_NO_STDCPP)
    typedef void (*CharWriter)(char, void *);
    #define PUTCHAR(...) putChar_(__VA_ARGS__, this)
#else
    using CharWriter = std::function<void(char)>;
    #define PUTCHAR(...) putChar_(__VA_ARGS__)
#endif

    static constexpr char endl = '\n';

    explicit Writer(CharWriter putChar):putChar_{putChar} {}

    Writer & operator << (char const * str) {
        while (*str != 0)
            PUTCHAR(*(str++));
        return *this;
    }

    Writer & operator << (std::string const & str) {
        return *this << str.c_str();
    }

    Writer & operator << (char c) { 
        PUTCHAR(c); 
        return *this; 
    }

    Writer & operator << (bool b) {
        PUTCHAR(b ? 'T' : 'F');
        return *this;
    }

    Writer & operator << (uint8_t x) { return *this << (uint32_t) x; }
    Writer & operator << (uint16_t x) { return *this << (uint32_t)x; }
    Writer & operator << (uint32_t x) {
        uint32_t order = 1000000000;
        while (x < order && order > 1)
            order = order / 10;
        while (order >= 10) {
            PUTCHAR(static_cast<char>((x / order)) + '0');
            x = x % order;
            order = order / 10;
        }
        PUTCHAR(static_cast<char>(x) + '0');
        return *this;
    }
    Writer & operator << (uint64_t x) {
        uint64_t order = 10000000000000000000ul;
        while (x < order && order > 1)
            order = order / 10;
        while (order >= 10) {
            PUTCHAR(static_cast<char>((x / order)) + '0');
            x = x % order;
            order = order / 10;
        }
        PUTCHAR(static_cast<char>(x) + '0');
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
            PUTCHAR('-');
            value *= -1;
        }
        return (*this) << static_cast<uint8_t>(value);
    }

    Writer & operator << (int16_t value) { 
        if (value < 0) {
            PUTCHAR('-');
            value *= -1;
        }
        return (*this) << static_cast<uint16_t>(value);
    }

    Writer & operator << (int32_t value) { 
        if (value < 0) {
            PUTCHAR('-');
            value *= -1;
        }
        return (*this) << static_cast<uint32_t>(value);
    }

    Writer & operator << (int64_t value) { 
        if (value < 0) {
            PUTCHAR('-');
            value *= -1;
        }
        return (*this) << static_cast<uint64_t>(value);
    }

    template<typename T>
    std::enable_if_t<std::is_base_of<Converter, T>::value, Writer &> operator << (T & conv) {
        conv(*this);
        return *this;
    }

    template<typename T>
    std::enable_if_t<std::is_base_of<Converter, T>::value, Writer &> operator << (T && conv) {
        conv(*this);
        return *this;
    }

private:

    CharWriter putChar_;
}; // Writer

#undef PUTCHAR

class BufferedWriter : protected Writer {
public:
    BufferedWriter():
#if (defined PLATFORM_NO_STDCPP)
        Writer{[](char c, void * self) { ((BufferedWriter*)self)->append(c); }} {
#else
        Writer{[this](char c) { append(c); }} {
#endif
    }

    ~BufferedWriter() {
        delete[] buffer_;
    }

    template<typename T>
    BufferedWriter & operator << (T x) {
        (*(Writer*)this) << x;
        return *this; 
    }

    uint32_t size() const { return size_; }

    uint32_t capacity() const { return capacity_; }

    char const * c_str() { return buffer_; }
private:

    void append(char c) {
        if (size_ == capacity_) {
            capacity_ *= 2;
            char * newBuffer = new char[capacity_];
            std::memcpy(newBuffer, buffer_, size_);
            delete[] buffer_;
            buffer_ = newBuffer;
        }
        buffer_[size_++] = c;
        buffer_[size_] = 0;
    }

    char * buffer_ = new char[17];;
    uint32_t size_ = 0;
    uint32_t capacity_ = 16; 
}; // BufferedWriter

template<typename T>
class fillLeft : public Writer::Converter {
public:
    fillLeft(T const & what, uint32_t width, char fill = ' '):
        what_{what}, width_{width}, fill_{fill} {
    }

    void operator () (Writer & writer) {
        BufferedWriter bw;
        bw << what_;
        while (bw.size() < width_) {
            writer << fill_;
            --width_;
        }
        writer << bw.c_str();;
    }

private:
    T const & what_;
    uint32_t width_;
    char fill_;
}; // fillLeft

template<typename T>
class fillRight : public Writer::Converter {
public:
    fillRight(T const & what, uint32_t width, char fill = ' '):
        what_{what}, width_{width}, fill_{fill} {
    }

    void operator () (Writer & writer) {
        BufferedWriter bw;
        bw << what_;
        writer << bw.c_str();
        while (bw.size() < width_) {
            writer << fill_;
            --width_;
        }
    }
    
private:
    T const & what_;
    uint32_t width_;
    char fill_;
}; // fillRight

/** Converter that displays numbers in hexadecimal. 
    
    The numbers are aligned based on their size and by default contain the `0x` prefix, which can be disabled for more compact output
 */
template<typename T>
class hex : public Writer::Converter {
public:
    hex(T what, bool header = true): what_{what}, header_{header} {}

    void operator () (Writer & writer);
private:
    T what_;
    bool header_;
}; // hex


template<>
inline void hex<uint8_t>::operator()(Writer & writer) {
    if (header_)
        writer << '0' << 'x';
    writer << "0123456789abcdef"[(what_ >> 4) & 0xf];
    writer << "0123456789abcdef"[what_ & 0xf];
}

template<>
inline void hex<uint16_t>::operator()(Writer & writer) {
    if (header_)
        writer << '0' << 'x';
    writer << "0123456789abcdef"[what_ >> 12];
    writer << "0123456789abcdef"[(what_ >> 8) & 0xf];
    writer << "0123456789abcdef"[(what_ >> 4) & 0xf];
    writer << "0123456789abcdef"[what_ & 0xf];
}

template<>
inline void hex<uint32_t>::operator()(Writer & writer) {
    if (header_)
        writer << '0' << 'x';
    writer << "0123456789abcdef"[what_ >> 28];
    writer << "0123456789abcdef"[(what_ >> 24) & 0xf];
    writer << "0123456789abcdef"[(what_ >> 20) & 0xf];
    writer << "0123456789abcdef"[(what_ >> 16) & 0xf];
    writer << "0123456789abcdef"[(what_ >> 12) & 0xf];
    writer << "0123456789abcdef"[(what_ >> 8) & 0xf];
    writer << "0123456789abcdef"[(what_ >> 4) & 0xf];
    writer << "0123456789abcdef"[what_ & 0xf];
}

/** Binary converter. 
 */
template<typename T>
class bin : public Writer::Converter {
public:
    bin(T what, bool header = true): what_{what}, header_{header} {}

    void operator () (Writer & writer);
private:
    T what_;
    bool header_;
}; // bin

template<>
inline void bin<uint8_t>::operator()(Writer & writer) {
    if (header_)
        writer << '0' << 'b';
    writer << ((what_ & 128) ? '1' : '0');
    writer << ((what_ & 64) ? '1' : '0');
    writer << ((what_ & 32) ? '1' : '0');
    writer << ((what_ & 16) ? '1' : '0');
    writer << ((what_ & 8) ? '1' : '0');
    writer << ((what_ & 4) ? '1' : '0');
    writer << ((what_ & 2) ? '1' : '0');
    writer << ((what_ & 1) ? '1' : '0');
}
