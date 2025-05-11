#pragma once

#include "../rckid.h"
#include "stream.h"

namespace rckid::json {

    /** JSON object. 
     */
    class Object {
    public:
        enum class Kind {
            Null,
            Boolean,
            Integer, 
            Double,
            String,
            Array,
            Struct,
        }; 

        Object() : kind_{Kind::Null} {}

        ~Object() {
            if (kind_ == Kind::String) {
                string_.~String();
            } else if (kind_ == Kind::Array) {
                array_.~vector();
            } else if (kind_ == Kind::Struct) {
                struct_.~unordered_map();
            }
        }

        Kind kind() const {
            return kind_;
        }

        int32_t asInteger() const {
            ASSERT(kind_ == Kind::Integer);
            return int_;
        }

        double asDouble() const {
            ASSERT(kind_ == Kind::Double);
            return double_;
        }

        bool asBoolean() const {
            ASSERT(kind_ == Kind::Boolean);
            return boolean_;
        }

        String asString() const {
            ASSERT(kind_ == Kind::String);
            return string_;
        }

        Object & operator[](uint32_t index) {
            ASSERT(kind_ == Kind::Array);
            return array_[index];
        }

        Object & operator[](String const & key) {
            ASSERT(kind_ == Kind::Struct);
            return struct_[key];
        }

    private:
        Kind kind_;
        union {
            bool boolean_;
            int32_t int_;
            double double_;
            String string_;
            std::vector<Object> array_;
            std::unordered_map<String, Object> struct_;
        };
    }; 

    Object parse(ReadStream && stream) {
        // TODO implement the parser
        UNIMPLEMENTED;
    }

    /** Parses JSON object from given buffer. Internally wraps the buffer in a read stream and calls the streamed parser.
     */
    Object parse(char const * json) {
        return parse(std::move(MemoryReadStream(reinterpret_cast<uint8_t const *>(json), strlen(json))));
    }




} // namespace rckid::json