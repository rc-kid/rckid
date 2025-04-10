#pragma once

#include <platform/writer.h>
#include "../memory.h"

#define STR(...) (rckid::StringWriter{} << __VA_ARGS__).str()

namespace rckid {

    /** Simple string that can hold either const char found in ROM, or mutable string stored on heap.
     */
    class String {
    public:

        /** Creates an empty string. 
         */
        String() = default;

        explicit String(Allocator & a): a_{a} {}

        String(char c, uint32_t n, Allocator & a = Heap::allocator()): a_{a}, data_{a.alloc<char>(n + 1)}, size_{n}, capacity_{} {
            memset(data_, c, n);
            data_[n] = '\0';
        }

        /** Creates string from given literal. 
         
            If the string literal comes from heap, creates new heap allocated copy of it. Otherwise just keeps the presumed immuatble pointer. The string must be null terminated, or null.
         */
        String(char const * str, Allocator & a = Heap::allocator()) : a_{a}, data_{const_cast<char*>(str)} {
            if (str == nullptr)
                return;
            size_ = strlen(str);
            if (Heap::contains(str))
                grow(size_);
        }

        String(char const * str, uint32_t size, Allocator & a = Heap::allocator()): a_{a}, data_{const_cast<char*>(str)}, size_{size} {
            // we have to always grow as we are not sure that the substring will end with 0
            grow(size_);
            data_[size_] = 0;
        }

        String(String const & from) : a_{from.a_}, data_{from.data_}, size_{from.size_}, capacity_{from.capacity_} {
            if (from.capacity_ > 0) {
                data_ = a_.alloc<char>(from.size_ + 1);
                memcpy(data_, from.data_, from.size_ + 1);
            }
        }

        String(String && from): a_{from.a_}, data_{from.data_}, size_{from.size_}, capacity_{from.capacity_} {
            if (capacity_ != 0) {
                from.data_ = nullptr;
                from.size_ = 0;
                from.capacity_ = 0;
            }
        }

        static String withCapacity(uint32_t size, Allocator & a = Heap::allocator()) {
            String s{a};
            s.grow(size);
            return s;
        }

        static String withCapacity(char const * str, uint32_t size, Allocator & a = Heap::allocator()) {
            String s{str, a};
            s.grow(size);
            return s;
        }

        /** Deletes the string. If the stored string literal belongs to heap, cleans it.
         */
        ~String() {
            a_.tryFree(data_);
        }

        String & operator = (String const & other) {
            if (capacity_ < other.size_) {
                a_.tryFree(data_);
                data_ = a_.alloc<char>(other.size_ + 1);
                capacity_ = other.size_;
            }
            memcpy(data_, other.data_, other.size_ + 1);
            size_ = other.size_;
            return *this;
        }

        String & operator = (String && other) {
            if (this != &other) {
                a_.tryFree(data_);
                data_ = other.data_;
                size_ = other.size_;
                capacity_ = other.capacity_;
                other.data_ = nullptr;
                other.size_ = 0;
                other.capacity_ = 0;
            }
            return *this;
        }

        bool empty() const { return size_ == 0; }

        bool immutable() const { return ! Heap::contains(data_); }

        char const * c_str() const { return data_; }

        char * data() { return data_; }
        char const * data() const { return data_; }

        uint32_t size() const { return size_; }

        char const * begin() const { return data_; }
        char const * end() const { return data_ + size_; }

        char operator[](uint32_t index) const { return data_[index]; }

        String & operator += (char x) {
            if (capacity_ < size_ + 1)
                grow(size_ == 0 ? 16 : size_ * 2);
            data_[size_] = x;
            data_[size_ + 1] = '\0';
            ++size_;
            return *this;
        }

        bool operator == (char const * other) const {
            return strcmp(data_, other) == 0;
        }

        bool operator != (char const * other) const {
            return strcmp(data_, other) != 0;
        }

        String substr(uint32_t start) const { return substr(start, size_ - start); }
            
        String substr(uint32_t start, uint32_t length) const {
            if (start >= size_)
                return String{};
            ASSERT(start + length <= size_);
            return String{data_ + start, length};
        }


        uint32_t capacity() const { return capacity_; }

        void grow(uint32_t size) {
            if (size > capacity_) {
                char * newData = a_.alloc<char>(size + 1);;
                memcpy(newData, data_, size_ + 1);
                a_.tryFree(data_);
                data_ = newData;
                capacity_ = size;
            }
        }

        void shrink() {
            if (size_ < capacity_) {
                char * newData = a_.alloc<char>(size_ + 1);
                memcpy(newData, data_, size_ + 1);
                a_.tryFree(data_);
                data_ = newData;
                capacity_ = size_;
            }
        }

    private:
        Allocator & a_ = Heap::allocator();
        // start an empty string
        char * data_ = const_cast<char *>("");
        uint32_t size_ = 0;
        uint32_t capacity_ = 0;
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