#pragma once

#include <platform/writer.h>

#include <rckid/memory.h>
#include <rckid/serialization.h>

#define STR(...) [&](){ rckid::StringBuilder sb{}; sb.writer() << __VA_ARGS__; return sb.str(); }()

namespace rckid {

    /** String is a very simple view over characters. 
     
        Strings in RCKid SDK are immutable views into a string that support either flash based literals, or heap allocated values via the internal mutable_ptr<char>. This makes them very lightweight and memory efficient with simple and predictable ergonomics.

        Although operator + is provided for concatenating strings, any non-trivial String construction should be done by the StringBuilder class and the associated STR macro.

        All Strings are null terminated.
     */
    class String {
    public:

        /** Default string constructor creates an empty string. 
         
            The empty string literal does not cost us anything as it will be stored in flash memory and hence just use pointer to it w/o any copying. 
         */
        String() : String{emptyLiteral_} { }
        
        /** Creates string from given character literal. 
         
            The character literal *must* be null terminated and will be copied if not stored in immutable flash memory. To create string from owned heap data, use the constructor taking immutable_ptr<char>.
         */
        String(char const * s) : 
            size_{static_cast<uint32_t>(std::strlen(s))}
        {
            if (hal::memory::isImmutableDataPtr(s)) {
                data_ = immutable_ptr<char>{s};
            } else {
                char * data = new char[size_ + 1];
                std::memcpy(data, s, size_ + 1);
                data_ = immutable_ptr<char>{data};
            }
        }

        String(immutable_ptr<char> data) : data_{std::move(data)} {
            ASSERT(data_.get() != nullptr);
            size_ = static_cast<uint32_t>(std::strlen(data_.get()));
        }

        ~String() = default;

        String(String const & other) {
            if (hal::memory::isImmutableDataPtr(other.data_.get())) {
                data_ = immutable_ptr<char>{other.data_.get()};
            } else {
                // make a copy of the heap data
                char * newData = new char[other.size_ + 1];
                std::memcpy(newData, other.data_.get(), other.size_ + 1);
                data_ = immutable_ptr<char>{newData};
            }
            size_ = other.size_;
        }

        String(String && other) noexcept : data_{std::move(other.data_)}, size_{other.size_} {
            other.data_ = immutable_ptr<char>{emptyLiteral_};
            other.size_ = 0;
        }

        String & operator = (char const * s) {
            ASSERT(hal::memory::isImmutableDataPtr(s));
            data_ = immutable_ptr<char>{s};
            size_ = static_cast<uint32_t>(std::strlen(s));
            return *this;
        }

        String & operator = (String const & other) {
            if (this == & other)
                return *this;
            *this = String{other};
            return *this;
        }

        String & operator = (String && other) noexcept {
            if (this == & other)
                return *this;
            data_ = std::move(other.data_);
            size_ = other.size_;
            other.data_ = immutable_ptr<char>{emptyLiteral_};
            other.size_ = 0;
            return *this;
        }

        char const * c_str() const {
            return data_.get();
        }

        char const * data() const {
            return data_.get();
        }

        /** Returns true if the string is empty. 
         */
        bool empty() const { return size_ == 0; }

        /** Returns the size of the stringm, excluding the null character at the end.
         */
        uint32_t size() const { return size_; }

        bool startsWith(String const & prefix) const {
            uint32_t prefixSize = prefix.size();
            if (size() < prefixSize)
                return false;
            return std::strncmp(data_.get(), prefix.data_.get(), prefixSize) == 0;
        }

        bool endsWith(String const & suffix) const {
            uint32_t suffixSize = suffix.size();
            if (size() < suffixSize)
                return false;
            return std::strncmp(data_.get() + size() - suffixSize, suffix.data_.get(), suffixSize) == 0;
        }

        bool endsWith(char c) const {
            if (size() == 0)
                return false;
            return data_.get()[size_ - 1] == c;
        }

        String substr(uint32_t pos, uint32_t count = UINT32_MAX) const {
            if (pos >= size())
                return String{};
            uint32_t available = size() - pos;
            if (count > available)
                count = available;
            char * newData = new char[count + 1];
            std::memcpy(newData, data_.get() + pos, count);
            newData[count] = '\0';
            return String{newData, count};
        }

        bool operator == (String const & other) const {
            return std::strcmp(data_.get(), other.data_.get()) == 0;
        }

        bool operator != (String const & other) const {
            return std::strcmp(data_.get(), other.data_.get()) != 0;
        }

        bool operator < (String const & other) const {
            return std::strcmp(data_.get(), other.data_.get()) < 0;
        }

        bool operator > (String const & other) const {
            return std::strcmp(data_.get(), other.data_.get()) > 0;
        }

        bool operator <= (String const & other) const {
            return std::strcmp(data_.get(), other.data_.get()) <= 0;
        }

        bool operator >= (String const & other) const {
            return std::strcmp(data_.get(), other.data_.get()) >= 0;
        }

        char operator [] (uint32_t index) const {
            if (index >= size())
                return 0;
            return data_.get()[index];
        }

        /** Creates new string by concatenating two existing ones. 
         
            Note that this is a convenience function only - for any non-trivial string manipulations the string builder writer interface (STR macro) should be used for better ergonomics *and* performance.
         */
        String operator + (String const & other) const {
            uint32_t newSize = size() + other.size();
            char * newData = new char[newSize + 1];
            std::memcpy(newData, data_.get(), size());
            std::memcpy(newData + size(), other.data_.get(), other.size());
            newData[newSize] = '\0';
            return String{newData, newSize};
        }

        /** Returns reader constructed from the string.
         
            Optionally the reader can start at a specific position, which defaults to the beginning of the string.
         */
        Reader reader(uint32_t pos = 0) const {
            return Reader([this, pos] (bool advance) mutable -> int32_t {
                if (pos >= size())
                    return Reader::EOFMarker;
                char c = data_.get()[pos];
                if (advance)
                    ++pos;
                return static_cast<int32_t>(c);
            });
        }

        /** Releases the  stored pointer as const. 
         */
        char const * release() { 
            char const * result = data_.release();
            data_ = immutable_ptr<char>{emptyLiteral_};
            return result;
        }

    private:

        friend class StringBuilder;

        static constexpr char const * emptyLiteral_ = "";

        // private constructor to utilize the already calculated size
        String(char * s, uint32_t size) : data_{s}, size_{size} { }
    
        // pointer to the string data
        immutable_ptr<char> data_;
        // cached size of the string data
        uint32_t size_ = 0;

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
            return String{buffer, size_};
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

    inline Writer operator << (Writer w, String const & s) {
        for (uint32_t i = 0; i < s.size(); ++i) {
            w.putChar(s.c_str()[i]);
        }
        return w;
    }

    inline BinaryWriter operator << (BinaryWriter w, String const & s) {
        w << static_cast<uint32_t>(s.size());
        for (uint32_t i = 0; i < s.size(); ++i)
            w.putByte(s.c_str()[i]);
        return w;
    }

} // namespace rckid

namespace std {
    template<>
    struct hash<rckid::String> {
        size_t operator () (rckid::String const & s) const {
            return hash<std::string_view>()(std::string_view{s.c_str(), s.size()});
        }
    };
}
