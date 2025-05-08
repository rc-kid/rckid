#pragma once

#include <platform/writer.h>
#include "../memory.h"
#include "buffers.h"

#define STR(...) (rckid::StringWriter{} << __VA_ARGS__).str()

namespace rckid {

    /** Simple string that can hold either const char found in ROM, or mutable string stored on heap.
     */
    class String {
    public:

        /** Creates an empty string. 
         */
        String(Allocator & a = Heap::allocator()): String{"", a} {}

        String(char c, uint32_t n, Allocator & a = Heap::allocator()): 
            str_{n + 1, a} {
            memset(str_.data(), c, n);
            str_[size()] = '\0';
        }

        /** Creates string from given literal. 
         
            If the string literal comes from heap, creates new heap allocated copy of it. Otherwise just keeps the presumed immuatble pointer. The string must be null terminated, or null.
         */
        String(char const * str, Allocator & a = Heap::allocator()) : 
            str_{str, static_cast<uint32_t>(strlen(str) + 1), a} {
        }

        String(String const & from, uint32_t start, uint32_t length):
            str_{length + 1, from.str_.allocator()} {
            memcpy(str_.data(), from.str_.data() + start, length);
            str_[length] = '\0';
            str_.grow(length + 1);
        } 

        String(char const * str, uint32_t size, Allocator & a = Heap::allocator()): 
            str_{str, size + 1, a} {
        }

        String(String const & from) = default;

        String(String && from) = default;

        String & operator = (String const &) = default;
        String & operator = (String &&) = default;

        static String withCapacity(uint32_t size, Allocator & a = Heap::allocator()) {
            String s{a};
            s.str_.grow(size + 1);
            return s;
        }

        static String withCapacity(char const * str, uint32_t size, Allocator & a = Heap::allocator()) {
            String s{str, a};
            s.str_.grow(size + 1);
            return s;
        }

        bool empty() const { return str_.size() <= 1; }

        bool immutable() const { return str_.immutable(); }

        char const * c_str() const { return str_.data(); }

        char * data() { return str_.data(); }
        char const * data() const { return str_.data(); }

        uint32_t size() const { 
            uint32_t result = str_.size();
            if (result > 0)
                --result;
            return result;
        }

        uint32_t capacity() const {             
            uint32_t result = str_.capacity();
            if (result > 0)
                --result;
            return result;
        }

        char const * begin() const { return str_.begin(); }
        char const * end() const { return str_.begin() + size(); }

        char operator[](uint32_t index) const { return str_[index]; }

        String & operator += (char x) {
            str_[size()] = x;
            str_.append('\0');
            return *this;
        }

        bool operator == (char const * other) const {
            return strcmp(str_.data(), other) == 0;
        }

        bool operator != (char const * other) const {
            return strcmp(str_.data(), other) != 0;
        }

        bool operator == (String const & other) const {
            return strcmp(str_.data(), other.str_.data()) == 0;
        }

        bool operator != (String const & other) const {
            return strcmp(str_.data(), other.str_.data()) != 0;
        }

        String substr(uint32_t start) const { return substr(start, size() - start); }
            
        String substr(uint32_t start, uint32_t length) const {
            if (length == 0)
                return String{};
            ASSERT(start + length <= size());
            return String(*this, start, length);
        }

        void shrink() { str_.shrink(); }

        void grow(uint32_t size) { str_.grow(size + 1); }

        bool endsWith(char other) const {
            if (size() == 0)
                return false;
            return c_str()[size() - 1] == other;
        }

        bool endsWith(char const * other) const {
            uint32_t len = strlen(other);
            if (len > size())
                return false;
            return strcmp(c_str() + size() - len, other) == 0;
        }

    private:
        LazyBuffer<char> str_;
    }; // rckid::String

    // support for the writer interface
    inline Writer & operator<<(Writer & w, String const & s) {
        w << s.c_str();
        return w;
    }

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
    
        String str() {
            return std::move(str_);
        }
    
    private:
        String str_;
        Writer writer_;
    }; // rckid::StringWriter

} // namespace rckid

namespace std {
    template<>
    struct hash<rckid::String> {
        size_t operator()(rckid::String const & obj) const noexcept {
            size_t result = 0;
            for (uint32_t i = 0, e = obj.size(); i != e; ++i)
                result += std::hash<char>{}(obj[i]) + 0x9e3779b9 + (result << 6) + (result >> 2);
            return result;
        }
    }; // std::hash<rckid::String>
} // namespac std
