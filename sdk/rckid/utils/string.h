#pragma once

#include <platform/writer.h>
#include "../memory.h"

namespace rckid {

    /** Simple string that can hold either const char found in ROM, or mutable string stored on heap.
     */
    class String {
    public:

        /** Creates an empty string. 
         */
        String() = default;

        /** Creates string from given literal. 
         
            If the string literal comes from heap, creates new heap allocated copy of it. Otherwise just keeps the presumed immuatble pointer. The string must be null terminated, or null.
         */
        String(char const * str) : data_{const_cast<char*>(str)} {
            if (str == nullptr)
                return;
            size_ = strlen(str);
            if (Heap::contains(str))
                grow(size_);
        }

        String(char const * str, uint32_t capacity): data_{const_cast<char*>(str)} {
            if (str != nullptr)
                size_ = strlen(str);
            grow(capacity);
        }

        String(String const & from) : data_{from.data_}, size_{from.size_}, capacity_{from.capacity_} {
            if (from.capacity_ > 0) {
                data_ = (char*)Heap::allocBytes(from.size_ + 1);
                memcpy(data_, from.data_, from.size_ + 1);
            }
        }

        String(String && from): data_{from.data_}, size_{from.size_}, capacity_{from.capacity_} {
            if (capacity_ != 0) {
                from.data_ = nullptr;
                from.size_ = 0;
                from.capacity_ = 0;
            }
        }

        // TODO constructor that takes std::string
        // TO BE DELETED WHEN WE SWITCH TO STRING COMPLETELY
        String(std::string const & str): data_{nullptr}, size_{0}, capacity_{0} {
            if (str.size() > 0) {
                data_ = (char*)Heap::allocBytes(str.size() + 1);
                memcpy(data_, str.c_str(), str.size() + 1);
                size_ = str.size();
                capacity_ = str.size();
            }
        }

        /** Deletes the string. If the stored string literal belongs to heap, cleans it.
         */
        ~String() {
            Heap::tryFree(data_);
        }

        String & operator = (String const & other) {
            if (capacity_ < other.size_) {
                Heap::tryFree(data_);
                data_ = (char*)Heap::allocBytes(other.size_ + 1);
                capacity_ = other.size_;
            }
            memcpy(data_, other.data_, other.size_ + 1);
            size_ = other.size_;
            return *this;
        }

        String & operator = (String && other) {
            if (this != &other) {
                Heap::tryFree(data_);
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

        uint32_t size() const { return size_; }

        char const * begin() const { return data_; }
        char const * end() const { return data_ + size_; }

        char operator[](uint32_t index) const { return data_[index]; }

        uint32_t capacity() const { return capacity_; }

        void grow(uint32_t size) {
            if (size > capacity_) {
                char * newData = (char*)Heap::allocBytes(size + 1);
                memcpy(newData, data_, size_);
                Heap::tryFree(data_);
                data_ = newData;
                capacity_ = size;
            }
        }

        void shrink() {
            if (size_ < capacity_) {
                char * newData = (char*)Heap::allocBytes(size_ + 1);
                memcpy(newData, data_, size_);
                Heap::tryFree(data_);
                data_ = newData;
                capacity_ = size_;
            }
        }

    private:
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

} // namespace rckid