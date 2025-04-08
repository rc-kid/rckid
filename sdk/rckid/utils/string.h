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

        String(String && from): data_{from.data_}, size_{from.size_}, capacity_{from.capacity_} {
            if (capacity_ != 0) {
                from.data_ = nullptr;
                from.size_ = 0;
                from.capacity_ = 0;
            }
        }

        /** Deletes the string. If the stored string literal belongs to heap, cleans it.
         */
        ~String() {
            Heap::tryFree(data_);
        }

        bool empty() const { return size_ == 0; }

        bool immutable() const { return ! Heap::contains(data_); }

        char const * c_str() const {
            return data_;
        }

        uint32_t size() const {
            return size_;
        }

        char operator[](uint32_t index) const {
            return data_[index];
        }

        uint32_t capacity() const {
            return capacity_;
        }

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
        char * data_ = nullptr;
        uint32_t size_ = 0;
        uint32_t capacity_ = 0;
    }; // rckid::String

    // support for the writer interface
    inline Writer & operator<<(Writer & w, String const & s) {
        w << s.c_str();
        return w;
    }

} // namespace rckid