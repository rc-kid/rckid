#pragma once

#include <platform/writer.h>

#include <rckid/memory.h>

#define STR(...) [&](){ rckid::StringBuilder sb{}; sb.writer() << __VA_ARGS__; return sb.str(); }()

namespace rckid {

    /** String is a very simple view over characters. 
     
        Strings in RCKid SDK are immutable views into a string that support either flash based literals, or heap allocated values via the internal mutable_ptr<char>. This makes them very lightweight and memory efficient with simple and predictable ergonomics.

        Although operator + is provided for concatenating strings, any non-trivial String construction should be done by the StringBuilder class and the associated STR macro.

        All Strings are null terminated.
     */
    class String {
    public:

        String() = default;
        constexpr String(char const * s) : data_{s} { }

        ~String() = default;

        String(String const & other): data_{other.data_.clone()} {}

        String(String && other) noexcept : data_{std::move(other.data_)} {}
        
        String & operator = (char const * s) {
            data_ = s;
            return *this;
        }

        String & operator = (String const & other) {
            if (this == & other)
                return *this;
            data_ = other.data_.clone();
            return *this;
        }

        String & operator = (String && other) noexcept {
            if (this == & other)
                return *this;
            data_ = std::move(other.data_);
            return *this;
        }

        char const * c_str() const {
            return data_.ptr();
        }

        char const * data() const {
            return data_.ptr();
        }

        /** Returns the size of the stringm, excluding the null character at the end.
         */
        uint32_t size() const {
            return data_.count() ? data_.count() - 1 : 0;
        }

        bool operator == (String const & other) const {
            return std::strcmp(data_.ptr(), other.data_.ptr()) == 0;
        }

        bool operator != (String const & other) const {
            return std::strcmp(data_.ptr(), other.data_.ptr()) != 0;
        }

        bool operator < (String const & other) const {
            return std::strcmp(data_.ptr(), other.data_.ptr()) < 0;
        }

        bool operator > (String const & other) const {
            return std::strcmp(data_.ptr(), other.data_.ptr()) > 0;
        }

        bool operator <= (String const & other) const {
            return std::strcmp(data_.ptr(), other.data_.ptr()) <= 0;
        }

        bool operator >= (String const & other) const {
            return std::strcmp(data_.ptr(), other.data_.ptr()) >= 0;
        }

        /** Creates new string by concatenating two existing ones. 
         
            Note that this is a convenience function only - for any non-trivial string manipulations the string builder writer interface (STR macro) should be used for better ergonomics *and* performance.
         */
        String operator + (String const & other) const {
            uint32_t newSize = size() + other.size();
            char * newData = new char[newSize + 1];
            std::memcpy(newData, data_.ptr(), size());
            std::memcpy(newData + size(), other.data_.ptr(), other.size());
            newData[newSize] = '\0';
            return String{newData, newSize + 1};
        }

    private:

        friend class StringBuilder;

        // private constructor to utilize the already calculated size
        String(char * s, uint32_t count) : data_{s, count} { }
    
        // we are using mutable_ptr as it provides size tracking already
        mutable_ptr<char> data_;

    }; // rckid::String

    /** Class for creating strings from parts, to be used with the STR macro.
     
        Internally, this uses the Writer interface for formatting, accumulating the string in a buffer. When done, calling the str() method creates an exactly sized null terminated string and returns the value. 
     */
    class StringBuilder {
    public:

        /** Default constructor with capacity of 32 characters.
         */
        StringBuilder(): StringBuilder{32} {}

        /** Creates the string builder with given capacity 
         */
        static StringBuilder withCapacity(uint32_t capacity) { return StringBuilder{capacity}; }

        /** Appends a single character to the string being built. 
         
            For anything more complete, use the writer() method to get to a writer interface backed by the string builder.
         */
        void appendChar(char c) {
            if (size_ == capacity_)
                grow();
            data_.get()[size_++] = c;
        }

        /** Returns a writer for the string builder. 
         */
        Writer writer() { return Writer([this](char c) { appendChar(c); }); }

        /** Call this to get the String out of the builder. 
         */
        String str() {
            char * buffer = new char[size_ + 1]; // for /0 at the end
            memcpy(buffer, data_.get(), size_);
            buffer[size_] = 0;
            return String{buffer, size_ + 1};
        }

        /** Returns current size of the string data accumulated by the builder. 
         */
        uint32_t size() const { return size_; }

        /** Clears the accumulated data without releasing the memory.
         */
        void clear() { size_ = 0; }

    private:

        StringBuilder(uint32_t capacity):
            data_{new char[capacity]},
            capacity_{capacity} {
        }

        void grow() {
            capacity_ *= 2;
            char * newData = new char[capacity_];
            memcpy(newData, data_.get(), size_);
            data_ = unique_ptr<char>(newData);
        }

        unique_ptr<char> data_;
        uint32_t size_ = 0;
        uint32_t capacity_ = 0;
    }; // StringBuilder
} // namespace rckid