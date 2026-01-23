#pragma once

#include <platform/writer.h>

#include <rckid/memory.h>

namespace rckid {

    /** String is a very simple view over characters. 
     
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

        // private constructor to utilize the already calculated size
        String(char * s, uint32_t count) : data_{s, count} { }
    
        // we are using mutable_ptr as it provides size tracking already
        mutable_ptr<char> data_;

    }; // rckid::String
} // namespace rckid