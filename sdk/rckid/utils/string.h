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

        static constexpr uint32_t NPOS = static_cast<uint32_t>(-1);

        /** Creates an empty string. 
         */
        String(): String{""} {}

        String(char c, uint32_t n): 
            str_{n + 1} {
            str_.setSize(n + 1);
            memset(str_.data(), c, n);
            str_[size()] = '\0';
        }

        /** Creates string from given literal. 
         
            If the string literal comes from heap, creates new heap allocated copy of it. Otherwise just keeps the presumed immuatble pointer. The string must be null terminated, or null.
         */
        String(char const * str) : 
            str_{str, static_cast<uint32_t>(strlen(str) + 1)} {
        }

        String(String const & from, uint32_t start, uint32_t length):
            str_{length + 1} {
            memcpy(str_.data(), from.str_.data() + start, length);
            str_[length] = '\0';
            str_.reserve(length + 1);
        } 

        /** Creates string from a subset of char pointer. 
         
            If the char pointer is null terminated at given size, will simply wrap it in lazy buffer which might save allocations for immutable memory. Otherwise allocates new heap memory and copies the data over, appending a null terminator.
         */
        String(char const * str, uint32_t size) {
            if (str[size] == '\0') {
                str_ = LazyBuffer<char>{str, size + 1};
            } else {
                str_ = LazyBuffer<char>{size + 1};
                str_.setSize(size + 1);
                memcpy(str_.data(), str, size);
                str_[size] = '\0';
            }
        } 

        String(String const & from) = default;

        String(String && from) noexcept = default;

        String & operator = (String const &) = default;
        String & operator = (String &&) = default;

        String & operator = (char const * str) {
            str_ = LazyBuffer<char>{str, static_cast<uint32_t>(strlen(str) + 1)};
            return *this;
        }

        static String withCapacity(uint32_t size) {
            String s{};
            s.str_.reserve(size + 1);
            return s;
        }

        static String withCapacity(char const * str, uint32_t size) {
            String s{str};
            s.str_.reserve(size + 1);
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
        
        char & operator[](uint32_t index) { return str_[index]; }

        void append(char c) {
            str_.makeMutable();
            if (str_.size() == 0) {
                str_.reserve(2);
                str_.setSize(2);
                str_[0] = c;
                str_[1] = '\0';
            } else {
                str_[size()] = c;
                str_.reserve(str_.size() + 1);
                str_.setSize(str_.size() + 1);
                str_[size()] = '\0';
            }
        }

        void clear() {
            str_.setSize(1);
            str_[0] = '\0';
        }

        String & operator += (char c) {
            append(c);
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

        void reserve(uint32_t size) { str_.reserve(size + 1); }

        void erase(uint32_t index, uint32_t length) {
            ASSERT(index + length <= size());
            if (index + length == size()) {
                str_[index] = '\0';
            } else {
                memmove(str_.data() + index, str_.data() + index + length, str_.size() - index - length + 1);
            }
            str_.setSize(str_.size() - length);
        }

        void insert(uint32_t index, char c) {
            ASSERT(index <= size());
            uint32_t oldSize = str_.size();
            str_.reserve(oldSize + 1);
            memmove(str_.data() + index + 1, str_.data() + index, oldSize - index);
            str_[index] = c;
            str_.setSize(oldSize + 1);
        }

        uint32_t find(char c, uint32_t start = 0) const {
            for (uint32_t i = start; i < size(); ++i) {
                if (str_[i] == c)
                    return i;
            }
            return NPOS;
        }

        bool startsWith(char what) const {
            if (size() == 0)
                return false;
            return c_str()[0] == what;
        }

        bool startsWith(char const * what) const {
            uint32_t len = strlen(what);
            if (len > size())
                return false;
            return strncmp(c_str(), what, len) == 0;
        }

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

        Writer write() {
            return Writer([](char c, void * self) {
                reinterpret_cast<String*>(self)->append(c);
            }, this);
        }

        char const * release() {
            return str_.release();
        }

        /** Conversion operator to const char * (null terminated string). 
         */
        operator char const * () const { return str_.data(); }

    private:
        LazyBuffer<char> str_;
    }; // rckid::String

    // support for the writer interface
    inline Writer operator<<(Writer && w, String const & s) {
        w << s.c_str();
        return std::move(w);
    }

    inline Writer & operator<<(Writer & w, String const & s) {
        w << s.c_str();
        return w;
    }

    class StringWriter {
    public:
        StringWriter():
            writer_{str_.write()} {
        }

        template<typename T>
        StringWriter & operator << (T && x) {
            writer_ << x;
            return *this; 
        }

        template<typename T>
        StringWriter & operator << (T const & x) {
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
